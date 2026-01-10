#include <iostream>
#include <memory>
#include <cstring>
#include <vector>
#include "signal_buffer.h"
#include "filter_bank.h"
#include "lagrange_matrix.h"
#include "gpu_backend/gpu_factory.h"
#include "profiling_engine.h"
#include "processing_pipeline.h"
#include "lfm_signal_generator.h"
#include "fractional_delay_cpu.h"
#include "result_comparator.h"
#include "gpu_profiling.h"
#include "gpu_backend/opencl_backend.h"
#include <iomanip>
#include <ctime>
#include <sstream>
#include <map>

using namespace radar;

int main(int argc, char* argv[]) {
    const float f_start = 100.0f;
    const float f_stop = 500.0f;
    const float sample_rate = 500000.0f;
    const float duration = 1.0f;

    // Параметры для теста
    const size_t num_samples = static_cast<size_t>(duration * sample_rate);
    const size_t num_beams = 128; // Для теста используем небольшое количество
    
    std::cout << "========================================\n";
    std::cout << "LCH-Farrow OpenCL Benchmark\n";
    std::cout << "========================================\n\n";
    
    // Создаём компоненты
    SignalBuffer signal_buffer(num_beams, num_samples);
    FilterBank filter_bank;
    ProfilingEngine profiler;
    
    // Генерируем ЛЧМ сигнал через новый генератор
    std::cout << "Генерация ЛЧМ сигнала...\n";
    radar::LFMParameters lfm_params;
    lfm_params.f_start = f_start;
    lfm_params.f_stop = f_stop;
    lfm_params.sample_rate = sample_rate;
    lfm_params.duration = duration;
    lfm_params.num_beams = num_beams;
    lfm_params.steering_angle = 30.0f;
    
    radar::LFMSignalGenerator lfm_generator(lfm_params);
    
    // Генерируем сигналы с дробными задержками для каждого луча
    for (size_t beam = 0; beam < num_beams; ++beam) {
        auto* beam_data = signal_buffer.GetBeamData(beam);
        float delay_samples = beam * 0.125f;  // 0.0, 0.125, 0.25, 0.375
        
        lfm_generator.GenerateBeam(beam_data, num_samples,
                                  radar::LFMVariant::DELAY, delay_samples);
    }
    
    printf("✅ ЛЧМ сигнал сгенерирован для %zu лучей\n", num_beams);
    printf("   Частота: %.0f - %.0f Гц\n", f_start, f_stop);
    printf("   Длительность: %.2f сек\n", duration);
    printf("   Отсчётов: %zu\n", num_samples);
    printf("   Все лучи хранятся в одном векторе (линейно)\n\n");

    // Загружаем матрицу Лагранжа (нужно для CPU и GPU обработки)
    std::cout << "Загрузка матрицы Лагранжа...\n";
    LagrangeMatrix lagrange_matrix;
    // Пробуем несколько путей
    std::vector<std::string> possible_paths = {
        "Doc/Example/lagrange_matrix.json",
        "../Doc/Example/lagrange_matrix.json",
        "../../Doc/Example/lagrange_matrix.json",
        "/home/alex/C++/LCH-Farrow/Doc/Example/lagrange_matrix.json"
    };
    
    bool loaded = false;
    for (const auto& path : possible_paths) {
        if (lagrange_matrix.LoadFromJson(path)) {
            std::cout << "Матрица загружена из: " << path << "\n";
            loaded = true;
            break;
        }
    }
    
    if (!loaded) {
        std::cerr << "Ошибка: не удалось загрузить матрицу Лагранжа\n";
        std::cerr << "Пробовались пути:\n";
        for (const auto& path : possible_paths) {
            std::cerr << "  - " << path << "\n";
        }
        return 1;
    }
    
    // Коэффициенты задержки для каждого луча (используем те же, что для генерации)
    std::vector<float> delay_coeffs(num_beams);
    for (size_t beam = 0; beam < num_beams; ++beam) {
        delay_coeffs[beam] = beam * 0.125f;  // 0.0, 0.125, 0.25, 0.375
    }
    
    // Копия данных для CPU обработки
    std::cout << "\n=== CPU ВЕРСИЯ (дробная задержка) ===\n";
    SignalBuffer cpu_signal_buffer(num_beams, num_samples);
    for (size_t beam = 0; beam < num_beams; ++beam) {
        const auto* src = signal_buffer.GetBeamData(beam);
        auto* dst = cpu_signal_buffer.GetBeamData(beam);
        if (src && dst) {
            std::memcpy(dst, src, num_samples * sizeof(SignalBuffer::ComplexType));
        }
    }
    
    // Выполнение CPU версии дробной задержки
    profiler.StartTimer("FractionalDelay_CPU");
    if (!ExecuteFractionalDelayCPU(&cpu_signal_buffer, &lagrange_matrix, 
                                    delay_coeffs.data(), num_beams, num_samples)) {
        std::cerr << "Ошибка при выполнении CPU версии дробной задержки\n";
        return 1;
    }
    profiler.StopTimer("FractionalDelay_CPU");
    std::cout << "✅ CPU версия выполнена\n";

    // Создаём GPU backend
    std::cout << "Инициализация GPU backend...\n";
    auto gpu_backend = GPUFactory::CreateBackend();
    if (!gpu_backend) {
        std::cerr << "Ошибка: не удалось создать GPU backend\n";
        return 1;
    }
    
    std::cout << "Backend: " << gpu_backend->GetBackendName() << "\n";
    std::cout << "Устройство: " << gpu_backend->GetDeviceName() << "\n";
    std::cout << "Память: " << (gpu_backend->GetDeviceMemorySize() / (1024 * 1024)) << " MB\n\n";
    
    // Загружаем матрицу на GPU
    if (!gpu_backend->UploadLagrangeMatrix(lagrange_matrix.GetData())) {
        std::cerr << "Ошибка: не удалось загрузить матрицу Лагранжа на GPU\n";
        return 1;
    }
    std::cout << "Матрица Лагранжа загружена на GPU\n\n";
    
    // Выполнение GPU версии дробной задержки (прямой вызов)
    std::cout << "\n=== GPU ВЕРСИЯ (дробная задержка, прямой вызов) ===\n";
    
    // Выделить память на GPU
    size_t buffer_size = num_beams * num_samples * sizeof(SignalBuffer::ComplexType);
    void* gpu_buffer = gpu_backend->AllocateDeviceMemory(buffer_size);
    if (!gpu_buffer) {
        std::cerr << "Ошибка: не удалось выделить память на GPU\n";
        return 1;
    }
    
    // Подготовить данные для H2D
    std::vector<SignalBuffer::ComplexType> host_buffer(num_beams * num_samples);
    for (size_t beam = 0; beam < num_beams; ++beam) {
        const auto* beam_data = signal_buffer.GetBeamData(beam);
        if (!beam_data) {
            std::cerr << "Ошибка: не удалось получить данные для луча " << beam << "\n";
            return 1;
        }
        std::memcpy(host_buffer.data() + beam * num_samples, 
                    beam_data, 
                    num_samples * sizeof(SignalBuffer::ComplexType));
    }
    
    // GPU профилирование через OpenCL Events
    DetailedGPUProfiling gpu_profiling;
    gpu_profiling.system_info = GetSystemInfo(gpu_backend.get());
    
    // H2D Transfer с GPU профилированием
    cl::Event h2d_event;
    OpenCLBackend* opencl_backend = dynamic_cast<OpenCLBackend*>(gpu_backend.get());
    if (opencl_backend && opencl_backend->CopyHostToDeviceWithProfiling(
            gpu_buffer, host_buffer.data(), buffer_size, h2d_event)) {
        h2d_event.wait();
        cl_ulong queued, submitted, started, ended;
        h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
        h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submitted);
        h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &started);
        h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &ended);
        gpu_profiling.gpu_events.push_back(
            CalculateEventMetrics("H2D_Transfer", queued, submitted, started, ended)
        );
    } else {
        std::cerr << "Ошибка при копировании данных на GPU с профилированием\n";
        gpu_backend->FreeDeviceMemory(gpu_buffer);
        return 1;
    }
    
    // Выполнить дробную задержку с GPU профилированием
    cl::Event kernel_event;
    if (opencl_backend && opencl_backend->ExecuteFractionalDelayWithProfiling(
            gpu_buffer, delay_coeffs.data(), num_beams, num_samples, kernel_event)) {
        kernel_event.wait();
        cl_ulong queued, submitted, started, ended;
        kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
        kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submitted);
        kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &started);
        kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &ended);
        gpu_profiling.gpu_events.push_back(
            CalculateEventMetrics("FractionalDelay_Kernel", queued, submitted, started, ended)
        );
    } else {
        std::cerr << "Ошибка при выполнении GPU версии дробной задержки с профилированием\n";
        gpu_backend->FreeDeviceMemory(gpu_buffer);
        return 1;
    }
    
    // D2H Transfer с GPU профилированием
    std::vector<SignalBuffer::ComplexType> gpu_result_buffer(num_beams * num_samples);
    cl::Event d2h_event;
    if (opencl_backend && opencl_backend->CopyDeviceToHostWithProfiling(
            gpu_result_buffer.data(), gpu_buffer, buffer_size, d2h_event)) {
        d2h_event.wait();
        cl_ulong queued, submitted, started, ended;
        d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
        d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submitted);
        d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &started);
        d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &ended);
        gpu_profiling.gpu_events.push_back(
            CalculateEventMetrics("D2H_Transfer", queued, submitted, started, ended)
        );
    } else {
        std::cerr << "Ошибка при копировании результатов с GPU с профилированием\n";
        gpu_backend->FreeDeviceMemory(gpu_buffer);
        return 1;
    }
    
    // Вычисляем общее время GPU
    for (const auto& event : gpu_profiling.gpu_events) {
        gpu_profiling.total_gpu_time_ms += event.total_time_ms;
    }
    
    // Скопировать результаты в SignalBuffer для сравнения
    SignalBuffer gpu_signal_buffer(num_beams, num_samples);
    for (size_t beam = 0; beam < num_beams; ++beam) {
        auto* beam_data = gpu_signal_buffer.GetBeamData(beam);
        if (!beam_data) {
            std::cerr << "Ошибка: не удалось получить выходной буфер для луча " << beam << "\n";
            gpu_backend->FreeDeviceMemory(gpu_buffer);
            return 1;
        }
        std::memcpy(beam_data, 
                    gpu_result_buffer.data() + beam * num_samples,
                    num_samples * sizeof(SignalBuffer::ComplexType));
    }
    
    // Освободить память на GPU
    gpu_backend->FreeDeviceMemory(gpu_buffer);
    
    std::cout << "✅ GPU версия выполнена\n";
    
    // Сравнение результатов CPU и GPU
    std::cout << "\n=== СРАВНЕНИЕ РЕЗУЛЬТАТОВ (CPU vs GPU) ===\n";
    ComparisonMetrics metrics;
    float tolerance = 1e-5f;  // Допустимая погрешность
    
    if (!CompareResults(&cpu_signal_buffer, &gpu_signal_buffer, tolerance, &metrics)) {
        std::cerr << "Ошибка при сравнении результатов\n";
        return 1;
    }
    
    // Вывести метрики
    std::cout << "Метрики сравнения:\n";
    std::cout << "  Максимальная разница (real): " << metrics.max_diff_real << "\n";
    std::cout << "  Максимальная разница (imag): " << metrics.max_diff_imag << "\n";
    std::cout << "  Максимальная разница (модуль): " << metrics.max_diff_magnitude << "\n";
    std::cout << "  Средняя разница (модуль): " << metrics.avg_diff_magnitude << "\n";
    std::cout << "  Максимальная относительная ошибка: " << metrics.max_relative_error << "\n";
    std::cout << "  Точки с превышением tolerance (" << tolerance << "): " 
              << metrics.errors_above_tolerance << " / " << metrics.total_points << "\n";
    
    if (metrics.errors_above_tolerance == 0) {
        std::cout << "✅ Результаты CPU и GPU идентичны (в пределах tolerance)\n";
    } else {
        std::cout << "⚠️  Обнаружены различия между CPU и GPU результатами\n";
    }
    
    // Выводим отчёт о производительности
    std::cout << "\n";
    profiler.ReportMetrics();
    
    // Сохраняем отчёт в JSON
    std::string json_filename = "Results/JSON/profile_report.json";
    
    // Создать директорию если её нет (Windows)
    #ifdef _WIN32
    std::string dir = "Results\\JSON";
    system(("if not exist \"" + dir + "\" mkdir \"" + dir + "\"").c_str());
    #else
    std::string dir = "Results/JSON";
    system(("mkdir -p \"" + dir + "\"").c_str());
    #endif
    
    if (profiler.SaveReportToJson(json_filename)) {
        std::cout << "Отчёт сохранён в: " << json_filename << "\n";
    } else {
        std::cerr << "⚠️  Не удалось сохранить отчёт в: " << json_filename << "\n";
    }
    
    // Генерация расширенного GPU профилирования
    std::cout << "\n=== ГЕНЕРАЦИЯ РАСШИРЕННЫХ ОТЧЁТОВ ===\n";
    
    // Параметры сигнала для отчета
    std::map<std::string, std::string> signal_params;
    std::stringstream ss;
    ss << f_start << " - " << f_stop << " Гц";
    signal_params["Частота"] = ss.str();
    ss.str("");
    ss << sample_rate << " Гц";
    signal_params["Частота дискретизации"] = ss.str();
    ss.str("");
    ss << duration << " сек";
    signal_params["Длительность"] = ss.str();
    ss.str("");
    ss << num_beams;
    signal_params["Количество лучей"] = ss.str();
    ss.str("");
    ss << num_samples;
    signal_params["Количество отсчётов"] = ss.str();
    
    // Получаем текущую дату для имени файла
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    char date_str[32];
    std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);
    char time_str[32];
    std::strftime(time_str, sizeof(time_str), "%H-%M-%S", timeinfo);
    
    // Сохраняем расширенный JSON отчет
    std::stringstream extended_json_filename;
    extended_json_filename << "Results/JSON/profile_report_" << date_str << "_" << time_str << ".json";
    if (SaveDetailedGPUProfilingToJson(gpu_profiling, extended_json_filename.str())) {
        std::cout << "✅ Расширенный JSON отчёт сохранён в: " << extended_json_filename.str() << "\n";
    } else {
        std::cerr << "⚠️  Не удалось сохранить расширенный JSON отчёт\n";
    }
    
    // Сохраняем Markdown отчет
    std::string md_filename = "Results/rezult_test_gpu.md";
    if (SaveDetailedGPUProfilingToMarkdown(gpu_profiling, signal_params, md_filename)) {
        std::cout << "✅ Markdown отчёт сохранён в: " << md_filename << "\n";
    } else {
        std::cerr << "⚠️  Не удалось сохранить Markdown отчёт\n";
    }
    
    // Вывод детальных GPU метрик
    std::cout << "\n=== ДЕТАЛЬНЫЕ GPU МЕТРИКИ ===\n";
    for (const auto& event : gpu_profiling.gpu_events) {
        std::cout << "Событие: " << event.event_name << "\n";
        std::cout << "  Постановка в очередь: " << event.queue_time_ms << " мс\n";
        std::cout << "  Ожидание очереди: " << event.wait_time_ms << " мс\n";
        std::cout << "  Выполнение: " << event.execution_time_ms << " мс\n";
        std::cout << "  Всего: " << event.total_time_ms << " мс\n\n";
    }
    std::cout << "Общее время GPU: " << gpu_profiling.total_gpu_time_ms << " мс\n";
    
    std::cout << "\nГотово!\n";
    return 0;
}
