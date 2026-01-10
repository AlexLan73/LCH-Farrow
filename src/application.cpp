#include "application.h"

#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <cstring>

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
#include "validator.h"
#include "reporter.h"

namespace radar {

Application::Application(const Config& cfg)
    : cfg_(cfg),
      signal_buffer_(cfg_.num_beams, static_cast<size_t>(cfg_.duration * cfg_.sample_rate)),
      cpu_signal_buffer_(cfg_.num_beams, static_cast<size_t>(cfg_.duration * cfg_.sample_rate)),
      gpu_signal_buffer_(cfg_.num_beams, static_cast<size_t>(cfg_.duration * cfg_.sample_rate)),
      delay_coeffs_(cfg_.num_beams)
{
}

Application::~Application() = default;

int Application::Run() {
    std::cout << "========================================\n";
    std::cout << "LCH-Farrow OpenCL Benchmark (OOP)\n";
    std::cout << "========================================\n\n";

    if (!GenerateSignal()) return 1;
    if (!LoadLagrangeMatrix()) return 1;
    if (!RunCpuFractionalDelay()) return 1;
    if (!RunGpuFractionalDelay()) return 1;
    if (!CompareAndReport()) return 1;

    std::cout << "Готово!\n";
    return 0;
}

bool Application::GenerateSignal() {
    std::cout << "Генерация ЛЧМ сигнала...\n";
    radar::LFMParameters lfm_params;
    lfm_params.f_start = cfg_.f_start;
    lfm_params.f_stop = cfg_.f_stop;
    lfm_params.sample_rate = cfg_.sample_rate;
    lfm_params.duration = cfg_.duration;
    lfm_params.num_beams = cfg_.num_beams;
    lfm_params.steering_angle = cfg_.steering_angle;

    radar::LFMSignalGenerator lfm_generator(lfm_params);

    size_t num_samples = static_cast<size_t>(cfg_.duration * cfg_.sample_rate);
    for (size_t beam = 0; beam < cfg_.num_beams; ++beam) {
        auto* beam_data = signal_buffer_.GetBeamData(beam);
        float delay_samples = beam * 0.125f;
        lfm_generator.GenerateBeam(beam_data, num_samples,
                                   radar::LFMVariant::DELAY, delay_samples);
        delay_coeffs_[beam] = delay_samples;
    }

    printf("✅ ЛЧМ сигнал сгенерирован для %zu лучей\n", cfg_.num_beams);
    printf("   Частота: %.0f - %.0f Гц\n", cfg_.f_start, cfg_.f_stop);
    printf("   Длительность: %.2f сек\n", cfg_.duration);
    printf("   Отсчётов: %zu\n", num_samples);

    return true;
}

bool Application::LoadLagrangeMatrix() {
    std::cout << "Загрузка матрицы Лагранжа...\n";
    LagrangeMatrix lagrange_matrix;
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
        return false;
    }

    // Запоминаем матрицу в GPU backend при выполнении GPU шага
    // Чтобы не держать её тут как член класса, загрузка на GPU выполняется в соответствующем шаге
    return true;
}

bool Application::RunCpuFractionalDelay() {
    std::cout << "\n=== CPU ВЕРСИЯ (дробная задержка) ===\n";
    size_t num_samples = static_cast<size_t>(cfg_.duration * cfg_.sample_rate);

    // Копия данных для CPU
    for (size_t beam = 0; beam < cfg_.num_beams; ++beam) {
        const auto* src = signal_buffer_.GetBeamData(beam);
        auto* dst = cpu_signal_buffer_.GetBeamData(beam);
        if (src && dst) {
            std::memcpy(dst, src, num_samples * sizeof(SignalBuffer::ComplexType));
        }
    }

    profiler_.StartTimer("FractionalDelay_CPU");

    LagrangeMatrix lagrange_matrix;
    // reuse default loading paths from main implementation
    std::vector<std::string> possible_paths = {
        "Doc/Example/lagrange_matrix.json",
        "../Doc/Example/lagrange_matrix.json",
        "../../Doc/Example/lagrange_matrix.json",
        "/home/alex/C++/LCH-Farrow/Doc/Example/lagrange_matrix.json"
    };
    bool loaded = false;
    for (const auto& path : possible_paths) {
        if (lagrange_matrix.LoadFromJson(path)) { loaded = true; break; }
    }
    if (!loaded) {
        std::cerr << "Ошибка: не удалось загрузить матрицу Лагранжа для CPU шага\n";
        return false;
    }

    if (!ExecuteFractionalDelayCPU(&cpu_signal_buffer_, &lagrange_matrix,
                                   delay_coeffs_.data(), cfg_.num_beams, num_samples)) {
        std::cerr << "Ошибка при выполнении CPU версии дробной задержки\n";
        return false;
    }

    profiler_.StopTimer("FractionalDelay_CPU");
    std::cout << "✅ CPU версия выполнена\n";
    return true;
}

bool Application::RunGpuFractionalDelay() {
    std::cout << "Инициализация GPU backend...\n";
    auto gpu_backend = GPUFactory::CreateBackend();
    if (!gpu_backend) {
        std::cerr << "Ошибка: не удалось создать GPU backend\n";
        return false;
    }

    std::cout << "Backend: " << gpu_backend->GetBackendName() << "\n";
    std::cout << "Устройство: " << gpu_backend->GetDeviceName() << "\n";
    std::cout << "Память: " << (gpu_backend->GetDeviceMemorySize() / (1024 * 1024)) << " MB\n\n";

    // Загружаем матрицу Лагранжа на GPU
    LagrangeMatrix lagrange_matrix;
    std::vector<std::string> possible_paths = {
        "Doc/Example/lagrange_matrix.json",
        "../Doc/Example/lagrange_matrix.json",
        "../../Doc/Example/lagrange_matrix.json",
        "/home/alex/C++/LCH-Farrow/Doc/Example/lagrange_matrix.json"
    };
    bool loaded = false;
    for (const auto& path : possible_paths) {
        if (lagrange_matrix.LoadFromJson(path)) { loaded = true; break; }
    }
    if (!loaded) {
        std::cerr << "Ошибка: не удалось загрузить матрицу Лагранжа для GPU шага\n";
        return false;
    }

    if (!gpu_backend->UploadLagrangeMatrix(lagrange_matrix.GetData())) {
        std::cerr << "Ошибка: не удалось загрузить матрицу Лагранжа на GPU\n";
        return false;
    }

    std::cout << "Матрица Лагранжа загружена на GPU\n\n";

    // Подготовка буферов
    size_t num_samples = static_cast<size_t>(cfg_.duration * cfg_.sample_rate);
    size_t buffer_size = cfg_.num_beams * num_samples * sizeof(SignalBuffer::ComplexType);
    void* gpu_buffer = gpu_backend->AllocateDeviceMemory(buffer_size);
    if (!gpu_buffer) {
        std::cerr << "Ошибка: не удалось выделить память на GPU\n";
        return false;
    }

    std::vector<SignalBuffer::ComplexType> host_buffer(cfg_.num_beams * num_samples);
    for (size_t beam = 0; beam < cfg_.num_beams; ++beam) {
        const auto* beam_data = signal_buffer_.GetBeamData(beam);
        if (!beam_data) {
            std::cerr << "Ошибка: не удалось получить данные для луча " << beam << "\n";
            gpu_backend->FreeDeviceMemory(gpu_buffer);
            return false;
        }
        std::memcpy(host_buffer.data() + beam * num_samples,
                    beam_data,
                    num_samples * sizeof(SignalBuffer::ComplexType));
    }

    DetailedGPUProfiling gpu_profiling;
    gpu_profiling.system_info = GetSystemInfo(gpu_backend.get());

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
        return false;
    }

    cl::Event kernel_event;
    if (opencl_backend && opencl_backend->ExecuteFractionalDelayWithProfiling(
            gpu_buffer, delay_coeffs_.data(), cfg_.num_beams, num_samples, kernel_event)) {
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
        return false;
    }

    std::vector<SignalBuffer::ComplexType> gpu_result_buffer(cfg_.num_beams * num_samples);
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
        return false;
    }

    for (const auto& event : gpu_profiling.gpu_events) {
        gpu_profiling.total_gpu_time_ms += event.total_time_ms;
    }

    for (size_t beam = 0; beam < cfg_.num_beams; ++beam) {
        auto* beam_data = gpu_signal_buffer_.GetBeamData(beam);
        if (!beam_data) {
            std::cerr << "Ошибка: не удалось получить выходной буфер для луча " << beam << "\n";
            gpu_backend->FreeDeviceMemory(gpu_buffer);
            return false;
        }
        std::memcpy(beam_data,
                    gpu_result_buffer.data() + beam * num_samples,
                    num_samples * sizeof(SignalBuffer::ComplexType));
    }

    gpu_backend->FreeDeviceMemory(gpu_buffer);
    std::cout << "✅ GPU версия выполнена\n";

    // Сохраняем расширенное профилирование
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    char date_str[32];
    std::strftime(date_str, sizeof(date_str), "%Y-%m-%d", timeinfo);
    char time_str[32];
    std::strftime(time_str, sizeof(time_str), "%H-%M-%S", timeinfo);

    std::stringstream extended_json_filename;
    extended_json_filename << "Results/JSON/profile_report_" << date_str << "_" << time_str << ".json";
    std::map<std::string, std::string> signal_params;
    {
        std::stringstream ss;
        ss << cfg_.f_start << " - " << cfg_.f_stop << " Гц";
        signal_params["Частота"] = ss.str();
        ss.str(""); ss.clear();
        ss << cfg_.sample_rate << " Гц"; signal_params["Частота дискретизации"] = ss.str(); ss.str(""); ss.clear();
        ss << cfg_.duration << " сек"; signal_params["Длительность"] = ss.str(); ss.str(""); ss.clear();
        ss << cfg_.num_beams; signal_params["Количество лучей"] = ss.str(); ss.str(""); ss.clear();
    }
    reporter_.SaveDetailedGPU(gpu_profiling, signal_params, extended_json_filename.str(), "Results/rezult_test_gpu.md");

    return true;
}

bool Application::CompareAndReport() {
    std::cout << "\n=== СРАВНЕНИЕ РЕЗУЛЬТАТОВ (CPU vs GPU) ===\n";
    ComparisonMetrics metrics;
    if (!validator_.Validate(cpu_signal_buffer_, gpu_signal_buffer_, cfg_.tolerance, &metrics)) {
        std::cerr << "Ошибка при сравнении результатов\n";
        return false;
    }

    std::cout << "Метрики сравнения:\n";
    std::cout << "  Максимальная разница (real): " << metrics.max_diff_real << "\n";
    std::cout << "  Максимальная разница (imag): " << metrics.max_diff_imag << "\n";
    std::cout << "  Максимальная разница (модуль): " << metrics.max_diff_magnitude << "\n";
    std::cout << "  Средняя разница (модуль): " << metrics.avg_diff_magnitude << "\n";
    std::cout << "  Максимальная относительная ошибка: " << metrics.max_relative_error << "\n";
    std::cout << "  Точки с превышением tolerance (" << cfg_.tolerance << "): "
              << metrics.errors_above_tolerance << " / " << metrics.total_points << "\n";

    if (metrics.errors_above_tolerance == 0) {
        std::cout << "✅ Результаты CPU и GPU идентичны (в пределах tolerance)\n";
    } else {
        std::cout << "⚠️  Обнаружены различия между CPU и GPU результатами\n";
    }

    profiler_.ReportMetrics();
    reporter_.SaveProfiling(profiler_, "Results/JSON/profile_report.json");

    return true;
}

} // namespace radar
