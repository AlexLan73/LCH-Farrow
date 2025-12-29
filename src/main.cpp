#include <iostream>
#include <memory>
#include "signal_buffer.h"
#include "filter_bank.h"
#include "lagrange_matrix.h"
#include "gpu_backend/gpu_factory.h"
#include "profiling_engine.h"
#include "processing_pipeline.h"

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "LCH-Farrow OpenCL Benchmark\n";
    std::cout << "========================================\n\n";
    
    // Создаём компоненты
    SignalBuffer signal_buffer;
    FilterBank filter_bank;
    ProfilingEngine profiler;
    
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
    
    // Загружаем матрицу Лагранжа
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
    
    // Загружаем матрицу на GPU
    if (!gpu_backend->UploadLagrangeMatrix(lagrange_matrix.GetData())) {
        std::cerr << "Ошибка: не удалось загрузить матрицу Лагранжа на GPU\n";
        return 1;
    }
    std::cout << "Матрица Лагранжа загружена на GPU\n\n";
    
    // Создаём тестовые данные
    std::cout << "Создание тестовых данных...\n";
    size_t num_beams = 4;  // Для теста используем небольшое количество
    size_t num_samples = 1024;  // Для теста используем небольшой размер
    
    signal_buffer.Resize(num_beams, num_samples);
    
    // Заполняем тестовыми данными
    for (size_t beam = 0; beam < num_beams; ++beam) {
        auto* beam_data = signal_buffer.GetBeamData(beam);
        for (size_t sample = 0; sample < num_samples; ++sample) {
            float phase = 2.0f * 3.14159f * static_cast<float>(sample) / static_cast<float>(num_samples);
            beam_data[sample] = std::complex<float>(std::cos(phase), std::sin(phase));
        }
    }
    
    // Генерируем опорный ЛЧМ сигнал
    std::cout << "Генерация опорного ЛЧМ сигнала...\n";
    filter_bank.GenerateLFMReference(num_samples, 1.0f, 1.0f, 1.0f);
    
    // Создаём pipeline
    std::cout << "Создание processing pipeline...\n";
    ProcessingPipeline pipeline(
        &signal_buffer,
        &filter_bank,
        gpu_backend.get(),
        &profiler
    );
    
    // Выполняем обработку
    std::cout << "\nВыполнение pipeline...\n";
    if (!pipeline.ExecuteFull()) {
        std::cerr << "Ошибка при выполнении pipeline\n";
        return 1;
    }
    
    // Выводим отчёт о производительности
    std::cout << "\n";
    profiler.ReportMetrics();
    
    // Сохраняем отчёт в JSON
    std::string json_filename = "Results/JSON/profile_report.json";
    if (profiler.SaveReportToJson(json_filename)) {
        std::cout << "Отчёт сохранён в: " << json_filename << "\n";
    }
    
    std::cout << "\nГотово!\n";
    return 0;
}
