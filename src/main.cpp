#include <iostream>
#include <memory>
#include "signal_buffer.h"
#include "filter_bank.h"
#include "lagrange_matrix.h"
#include "gpu_backend/gpu_factory.h"
#include "profiling_engine.h"
#include "processing_pipeline.h"
#include "lfm_signal_generator.h"


int main(int argc, char* argv[]) {
  const float f_start = 100.0f;
  const float f_stop = 500.0f;
  const float sample_rate = 8000.0f;
  const float duration = 1.0f;

  // Создать генератор
  LFMSignalGenerator lfm(f_start, f_stop, sample_rate, duration);

  // Получить буфер данных
  // size_t num_samples = 1024;  // Для теста используем небольшой размер

  const size_t num_samples = static_cast<size_t>(duration * sample_rate);  // Для теста используем небольшой размер
  const size_t num_beams = 4; // Для теста используем небольшое количество
  
  // Создаём компоненты
  SignalBuffer signal_buffer(num_beams, num_samples);

  std::vector<std::complex<float>*> beam_ptrs(num_beams);
  for (size_t b = 0; b < num_beams; ++b) {
      beam_ptrs[b] = signal_buffer.GetBeamData(b);
  }

  // Задержки для каждого луча (имитация DOA)
  std::vector<float> delays(num_beams);
  for (size_t b = 0; b < num_beams; ++b) {
      delays[b] = b * 0.1f;  // 0, 0.1, 0.2, ... задержки в отсчётах
  }

  // Генерировать все лучи
  lfm.GenerateAllBeams(beam_ptrs, num_samples, num_beams, delays);

  printf("✅ ЛЧМ сигнал сгенерирован для %zu лучей\n", num_beams);
  printf("   Частота: %.0f - %.0f Гц\n", f_start, f_stop);
  printf("   Длительность: %.2f сек\n", duration);
  printf("   Отсчётов: %zu\n", num_samples);

  //--------------------------------------------------------------------


  std::cout << "========================================\n";
  std::cout << "LCH-Farrow OpenCL Benchmark\n";
  std::cout << "========================================\n\n";
    
    // Создаём компоненты
//    SignalBuffer signal_buffer;
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
    // Данные уже сгенерированы через LFMSignalGenerator выше
    
    // Генерируем опорный ЛЧМ сигнал (опционально, не используется в упрощённом pipeline)
    // std::cout << "Генерация опорного ЛЧМ сигнала...\n";
    // filter_bank.GenerateLFMReference(num_samples, 1.0f, 1.0f, 1.0f);
    
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
    
    // Опционально: копировать результат с GPU на хост для анализа
    // true = скопировать на хост, false = оставить на GPU
    bool copy_to_host = false;  // По умолчанию оставляем на GPU
    
    // Можно задать через аргументы командной строки
    if (argc > 1 && std::string(argv[1]) == "--copy-to-host") {
        copy_to_host = true;
        std::cout << "Режим: копирование результата с GPU на хост для анализа\n";
    } else {
        std::cout << "Режим: результат остаётся на GPU для дальнейшей обработки\n";
    }
    
    if (!pipeline.ExecuteFull(copy_to_host)) {
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
