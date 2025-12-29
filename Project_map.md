# 🗺️ PROJECT MAP - LCH-Farrow OpenCL Benchmark

## 📋 ОБЗОР ПРОЕКТА

**LCH-Farrow** - Multi-GPU Benchmark на OpenCL для Ubuntu с поддержкой NVIDIA и AMD GPU.

**Основная задача**: Реализация дробной задержки сигнала с интерполяцией Лагранжа до формирования матрицы с задержанными сигналами. Поддержка опционального вывода результата с GPU для анализа.

**Целевая платформа**: Ubuntu Linux, RTX 3060 (NVIDIA), поддержка AMD GPU

**Технологии**: C++17, OpenCL 2.0+, CMake 3.20+

---

## 📁 СТРУКТУРА КАТАЛОГОВ

```
LCH-Farrow/
├── CMakeLists.txt              # Главный файл сборки
├── CMakePresets.json           # CMake пресеты
├── CMakePresets.ubuntu         # Пресеты для Ubuntu
├── CMakePresets.windows        # Пресеты для Windows
├── CLAUDE.md                   # Конфигурация AI ассистента
├── Project_map.md              # Этот файл - карта проекта
├── README.md                   # README для GitHub
│
├── include/                    # Заголовочные файлы
│   ├── mylib.h                 # Старый тестовый файл
│   ├── signal_buffer.h         # Класс для управления сигнальными данными
│   ├── filter_bank.h           # Класс для FIR коэффициентов и ЛЧМ сигнала
│   ├── processing_pipeline.h   # Класс для координации обработки
│   ├── profiling_engine.h     # Класс для профилирования производительности
│   ├── lagrange_matrix.h       # Класс для работы с матрицей Лагранжа
│   └── gpu_backend/
│       ├── igpu_backend.h      # Абстрактный интерфейс GPU backend
│       ├── opencl_backend.h    # Реализация OpenCL backend
│       └── gpu_factory.h       # Фабрика для создания GPU backend
│
├── src/                        # Исходный код
│   ├── main.cpp                # Точка входа программы
│   ├── mylib.cpp               # Старый тестовый файл
│   ├── signal_buffer.cpp       # Реализация SignalBuffer
│   ├── filter_bank.cpp         # Реализация FilterBank
│   ├── processing_pipeline.cpp # Реализация ProcessingPipeline
│   ├── profiling_engine.cpp   # Реализация ProfilingEngine
│   ├── lagrange_matrix.cpp     # Реализация LagrangeMatrix
│   ├── gpu_backend/
│   │   ├── opencl_backend.cpp  # Реализация OpenCLBackend
│   │   └── gpu_factory.cpp     # Реализация GPUFactory
│   └── Production/
│       └── CMakeLists.txt      # CMake для production кода
│
├── kernels/                    # OpenCL kernel файлы
│   ├── kernel_fractional_delay.cl  # Kernel дробной задержки (Лагранж)
│   └── kernel_hadamard.cl          # Kernel поэлементного умножения (не используется в текущей версии)
│
├── data/                       # Тестовые данные
│   └── (тестовые файлы сигналов)
│
├── build/                      # Директория сборки (gitignore)
│   └── (сгенерированные файлы)
│
├── Results/                    # Результаты тестов
│   ├── JSON/                   # JSON отчёты профилирования
│   └── Profiler/               # Детальные отчёты профилирования
│
├── tests/                      # Тесты
│   ├── CMakeLists.txt
│   └── test_main.cpp
│
├── Doc/                        # Документация
│   ├── Plan/                   # Планы работ
│   │   ├── 00_ОБЩИЙ_ПЛАН.md
│   │   ├── 01_ДЕТАЛЬНЫЙ_ПЛАН.md
│   │   ├── DEBUG_FRACTIONAL_DELAY.md  # Маршрут отладки
│   │   ├── FRACTIONAL_DELAY_ROUTE.md   # Краткая справка
│   │   └── Start/              # Исходная документация
│   ├── Example/                # Примеры и эталоны
│   │   ├── lagrange_matrix.json        # Матрица Лагранжа 48×5
│   │   ├── lagrange_matrix_48x5.md     # Описание матрицы
│   │   └── opencl_fractional_delay_256beams.cpp  # Эталонный код
│   └── (другая документация)
│
└── MemoryBank/                 # Память проекта
    ├── key_findings.md         # Ключевые результаты исследований
    └── AI_SESSION_MEMORY.md    # Память AI сессий
```

---

## 🏗️ АРХИТЕКТУРА ПРОЕКТА

### Трёхслойная архитектура:

```
┌─────────────────────────────────────────────┐
│  СЛОЙ 1: C++ Приложение (платформа-независимо) │
│  - SignalBuffer                              │
│  - FilterBank                                │
│  - ProcessingPipeline                        │
│  - ProfilingEngine                           │
│  - LagrangeMatrix                            │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  СЛОЙ 2: GPU Абстракция (виртуальный интерфейс)│
│  - IGPUBackend (abstract)                   │
│  - OpenCLBackend (реализация)                │
│  - GPUFactory                                │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  СЛОЙ 3: OpenCL Код (специфичный для платформы)│
│  - kernel_fractional_delay.cl                │
│  - kernel_hadamard.cl                        │
│  - clFFT библиотека                          │
└─────────────────────────────────────────────┘
```

---

## 📦 КЛАССЫ И КОМПОНЕНТЫ

### 1. SignalBuffer
**Файлы**: `include/signal_buffer.h`, `src/signal_buffer.cpp`

**Назначение**: Управление сигнальными данными (лучами)

**Основные методы**:
- `LoadFromFile(filename)` - загрузка из бинарного файла
- `SaveToFile(filename)` - сохранение в бинарный файл
- `GetBeamData(beam_id)` - доступ к данным луча
- `Resize(num_beams, num_samples)` - изменение размера

**Данные**:
- 1-256 лучей
- 100-1300000 комплексных отсчётов на луч
- Тип: `vector<vector<complex<float>>>`

---

### 2. FilterBank
**Файлы**: `include/filter_bank.h`, `src/filter_bank.cpp`

**Назначение**: Управление FIR коэффициентами и опорным ЛЧМ сигналом

**Основные методы**:
- `LoadCoefficients(coeffs)` - загрузка FIR коэффициентов
- `GenerateLFMReference(...)` - генерация ЛЧМ сигнала
- `PrecomputeReferenceFft()` - предвычисление FFT опорного сигнала
- `GetReferenceFft()` - получение предвычисленной FFT

**Данные**:
- FIR коэффициенты: `vector<float>`
- Опорный ЛЧМ сигнал: `vector<complex<float>>`
- Предвычисленная FFT: `vector<complex<float>>`

---

### 3. LagrangeMatrix
**Файлы**: `include/lagrange_matrix.h`, `src/lagrange_matrix.cpp`

**Назначение**: Работа с матрицей коэффициентов Лагранжа для дробной задержки

**Основные методы**:
- `LoadFromJson(filename)` - загрузка из JSON файла
- `GetCoefficient(row, col)` - получение коэффициента
- `GetRowIndex(delay_fraction)` - получение индекса строки для дробной задержки

**Данные**:
- Матрица 48×5 (48 дробных задержек × 5 коэффициентов)
- Формат: `float[48 * 5]` (плоский массив)

---

### 4. ProcessingPipeline
**Файлы**: `include/processing_pipeline.h`, `src/processing_pipeline.cpp`

**Назначение**: Координация полного pipeline обработки сигнала

**Основные методы**:
- `ExecuteFull()` - выполнение полного pipeline
- `ExecuteStepByStep()` - пошаговое выполнение (для отладки)
- `ValidateResults(tolerance)` - валидация результатов

**Pipeline этапы**:
1. H2D Transfer (Host → Device) - загрузка данных на GPU
2. Дробная задержка (GPU Kernel) - формирование матрицы с задержанными сигналами
3. Опционально: D2H Transfer (Device → Host) - вывод с GPU для анализа

**Режимы работы**:
- `copy_to_host = false`: результат остаётся на GPU для дальнейшей обработки
- `copy_to_host = true`: результат копируется на хост для анализа

---

### 5. ProfilingEngine
**Файлы**: `include/profiling_engine.h`, `src/profiling_engine.cpp`

**Назначение**: Профилирование производительности всех этапов

**Основные методы**:
- `StartTimer(name)` - начать измерение (CPU)
- `StopTimer(name)` - остановить измерение (CPU)
- `RecordGpuEvent(name, time_ms)` - записать GPU событие
- `ReportMetrics()` - вывести отчёт в консоль
- `SaveReportToJson(filename)` - сохранить отчёт в JSON

**Метрики**:
- Время выполнения каждого этапа
- Минимальное/максимальное/среднее время
- Количество вызовов
- Общее время

---

### 6. IGPUBackend (Интерфейс)
**Файлы**: `include/gpu_backend/igpu_backend.h`

**Назначение**: Абстрактный интерфейс для GPU backend

**Виртуальные методы**:
- `Initialize()` - инициализация
- `AllocateDeviceMemory(size)` - выделение памяти
- `ExecuteFractionalDelay(...)` - дробная задержка
- `UploadLagrangeMatrix(data)` - загрузка матрицы Лагранжа
- `ExecuteFFT(...)` - FFT/IFFT (опционально, не используется в текущей версии)
- `ExecuteHadamardMultiply(...)` - поэлементное умножение (опционально, не используется в текущей версии)

---

### 7. OpenCLBackend
**Файлы**: `include/gpu_backend/opencl_backend.h`, `src/gpu_backend/opencl_backend.cpp`

**Назначение**: Реализация GPU backend через OpenCL

**Основные компоненты**:
- `cl::Platform`, `cl::Device`, `cl::Context` - OpenCL контекст
- `cl::CommandQueue` - очередь команд
- `cl::Program`, `cl::Kernel` - скомпилированные kernel'ы
- `cl::Buffer` - буферы на GPU

**Kernels**:
- `kernel_fractional_delay_` - дробная задержка (используется)
- `kernel_hadamard_` - поэлементное умножение (не используется в текущей версии)

**Буферы**:
- `lagrange_matrix_buffer_` - матрица Лагранжа на GPU

---

### 8. GPUFactory
**Файлы**: `include/gpu_backend/gpu_factory.h`, `src/gpu_backend/gpu_factory.cpp`

**Назначение**: Фабрика для создания GPU backend

**Методы**:
- `CreateBackend()` - создать оптимальный backend
- `CreateOpenCLBackend()` - создать OpenCL backend
- `IsOpenCLAvailable()` - проверить доступность OpenCL

**Приоритет выбора**:
1. OpenCL (NVIDIA RTX3060)
2. OpenCL (AMD GPU)
3. Другие OpenCL устройства

---

## 🔧 OPENCL KERNELS

### kernel_fractional_delay.cl
**Файл**: `kernels/kernel_fractional_delay.cl`

**Назначение**: Дробная задержка сигнала с интерполяцией Лагранжа 5-го порядка

**Параметры**:
- `input` - входной буфер сигналов
- `output` - выходной буфер (in-place)
- `lagrange_matrix` - матрица коэффициентов [48×5]
- `delay_params` - параметры задержки для каждого луча
- `num_beams`, `num_samples` - размеры

**Алгоритм**:
1. Определение луча и отсчёта по `global_id`
2. Получение параметров задержки (delay_integer, lagrange_row)
3. Интерполяция Лагранжа по 5 точкам
4. Запись результата

---

### kernel_hadamard.cl
**Файл**: `kernels/kernel_hadamard.cl`

**Назначение**: Поэлементное умножение (Hadamard product) для свёртки в частотной области

**Статус**: Не используется в текущей версии (отложено до будущих этапов)

**Примечание**: Kernel сохранён для будущего использования при добавлении FFT/IFFT свёртки

---

## 📊 ДАННЫЕ И ФОРМАТЫ

### Формат бинарного файла сигналов:
```
[Header: 2 × uint32_t]
  - num_beams (uint32_t)
  - num_samples (uint32_t)

[Data: num_beams × num_samples × 2 × float]
  - Для каждого комплексного числа:
    - real (float)
    - imag (float)
```

### Формат матрицы Лагранжа (JSON):
```json
[
  [L0, L1, L2, L3, L4],  // Строка 0 (delay_fraction = 0/48)
  [L0, L1, L2, L3, L4],  // Строка 1 (delay_fraction = 1/48)
  ...
  [L0, L1, L2, L3, L4]   // Строка 47 (delay_fraction = 47/48)
]
```

### Формат JSON отчёта профилирования:
```json
{
  "metrics": [
    {
      "name": "FractionalDelay",
      "time_ms": 0.034,
      "call_count": 1,
      "min_time_ms": 0.034,
      "max_time_ms": 0.034,
      "avg_time_ms": 0.034
    },
    ...
  ],
  "total_time_ms": 13.950
}
```

---

## 🔄 ПРОЦЕСС ОБРАБОТКИ

### Упрощённый pipeline (до формирования матрицы с задержанными сигналами):

```
1. Инициализация
   ├─ Загрузка матрицы Лагранжа из JSON
   ├─ Инициализация GPU backend
   └─ Загрузка матрицы на GPU

2. Подготовка данных
   └─ Загрузка сигналов (SignalBuffer)

3. GPU обработка
   ├─ H2D Transfer (загрузка данных на GPU)
   ├─ Дробная задержка (kernel_fractional_delay.cl)
   │  └─ Формирование матрицы с задержанными сигналами
   └─ Опционально: D2H Transfer (вывод с GPU для анализа)

4. Результаты
   ├─ Матрица с задержанными сигналами на GPU (или на хосте)
   ├─ Генерация отчёта профилирования
   └─ Сохранение JSON отчёта
```

---

## 🎯 КЛЮЧЕВЫЕ ОСОБЕННОСТИ

### 1. Интерполяция Лагранжа
- **Точность**: 5-й порядок полинома
- **Разрешение**: 1/48 шага дискретизации (2.083%)
- **Матрица**: 48 строк × 5 столбцов
- **Преимущество**: Высокая точность по сравнению с линейной интерполяцией

### 2. In-place обработка
- Все операции выполняются in-place для экономии памяти
- Один буфер используется для всех этапов
- Экономия памяти: 50% (2.66 ГБ вместо 5.3 ГБ)

### 3. Batch FFT
- Все лучи обрабатываются одновременно
- Один вызов clFFT для всех 256 лучей
- Ускорение: ~256× по сравнению с последовательной обработкой

### 4. Предвычисленная опорная FFT
- Опорная FFT вычисляется один раз при инициализации
- Используется для всех лучей
- Ускорение: ~256× по сравнению с вычислением в цикле

### 5. Кроссплатформенность
- OpenCL работает на NVIDIA и AMD GPU
- Единый код для разных платформ
- Автоматический выбор оптимального GPU

---

## 📈 ПРОИЗВОДИТЕЛЬНОСТЬ

### Целевые метрики (RTX 3060):

| Операция | Время (мс) | Описание |
|----------|------------|----------|
| H2D Transfer | < 200 | Копирование на GPU |
| FractionalDelay | ~4000-6000 | Дробная задержка (основной bottleneck) |
| FFT Forward | < 100 | Прямое FFT преобразование |
| HadamardMultiply | < 10 | Поэлементное умножение |
| IFFT Inverse | < 100 | Обратное FFT преобразование |
| D2H Transfer | < 200 | Копирование с GPU |
| **ИТОГО** | **< 10000** | Общее время для 256×1.3M |

### Текущие результаты (тестовый запуск):
- 4 луча × 1024 отсчёта: **~14 мс**
- FractionalDelay: **0.034 мс**
- FFT Forward: **4.965 мс**

---

## 🔍 ОТЛАДКА

### Ключевые файлы для отладки:

1. **Дробная задержка**:
   - `src/processing_pipeline.cpp:54` - точка входа
   - `src/gpu_backend/opencl_backend.cpp:159` - реализация
   - `kernels/kernel_fractional_delay.cl:29` - GPU kernel

2. **FFT**:
   - `src/gpu_backend/opencl_backend.cpp:250` - ExecuteFFT
   - clFFT библиотека

3. **Профилирование**:
   - `src/profiling_engine.cpp` - все методы профилирования

### Документация по отладке:
- `Doc/DEBUG_FRACTIONAL_DELAY.md` - детальный маршрут отладки
- `Doc/FRACTIONAL_DELAY_ROUTE.md` - краткая справка

---

## 🛠️ СБОРКА И ЗАПУСК

### Требования:
- Ubuntu Linux
- GCC/Clang компилятор
- CMake 3.20+
- OpenCL SDK (NVIDIA или AMD)
- clFFT библиотека (опционально)

### Сборка:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENCL=ON
cmake --build . -j$(nproc)
```

### Запуск:
```bash
./LCH-Farrow
```

---

## 📝 ЗАВИСИМОСТИ

### Внешние библиотеки:
- **OpenCL**: GPU вычисления
- **clFFT**: FFT преобразования (опционально)
- **Стандартная библиотека C++**: STL контейнеры

### Внутренние зависимости:
- Все классы независимы друг от друга (кроме интерфейсов)
- ProcessingPipeline координирует все компоненты
- GPUFactory создаёт backend

---

## 🎓 ОБУЧАЮЩИЕ МАТЕРИАЛЫ

### Документация:
- `Doc/Plan/00_ОБЩИЙ_ПЛАН.md` - общий план проекта
- `Doc/Plan/01_ДЕТАЛЬНЫЙ_ПЛАН.md` - детальный план
- `Doc/Example/` - примеры и эталоны

### Примеры кода:
- `Doc/Example/opencl_fractional_delay_256beams.cpp` - эталонная реализация
- `kernels/kernel_fractional_delay.cl` - рабочий kernel

---

## 🔮 БУДУЩИЕ УЛУЧШЕНИЯ

1. **Оптимизация kernel'ов**:
   - Использование local memory
   - Векторизация операций
   - Оптимизация work group размеров

2. **Поддержка больших данных**:
   - 256 лучей × 1.3M отсчётов
   - Оптимизация памяти
   - Асинхронные transfer'ы

3. **Дополнительные функции**:
   - Валидация результатов
   - Сравнение с CPU версией
   - Детальное профилирование GPU

---

*Обновлено: 2025-01-28*  
*Версия: 1.0*

