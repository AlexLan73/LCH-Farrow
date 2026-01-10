# 🗺️ PROJECT MAP - LCH-Farrow OpenCL Benchmark

## 🤖 AI АССИСТЕНТ

**Имя**: Кодо (Codo)
**Роль**: Code assistant and helper
**Помощники**: 5 синьоров (мастера/помощники)
**MCP-server**: sequential-thinking
**Конфигурация**: [`CLAUDE.md`](CLAUDE.md)

**Правила работы**:
- Всегда проверять [`MemoryBank`](MemoryBank/) перед началом работы
- Использовать sequential-thinking-mcp для сложных проблем
- Задавать уточняющие вопросы при неясностях
- Обновлять память сессий после важных разговоров
- Использовать 5 помощников (синьоров) при необходимости

---

## 📋 ОБЗОР ПРОЕКТА

**LCH-Farrow** - Multi-GPU Benchmark на OpenCL для Ubuntu с поддержкой NVIDIA и AMD GPU.

**Основная задача**: Реализация дробной задержки сигнала с интерполяцией Лагранжа до формирования матрицы с задержанными сигналами. Поддержка опционального вывода результата с GPU для анализа.

**Целевая платформа 1**: Windows, RTX 2080ti (NVIDIA), поддержка AMD GPU
**Целевая платформа 2**: Ubuntu Linux, RTX 3060 (NVIDIA), поддержка AMD GPU

**Технологии**: C++17, OpenCL 2.0+, CMake 3.20+

- сборка регулируется файлом CMakePresets.json

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
**Файлы**: [`include/signal_buffer.h`](include/signal_buffer.h), [`src/signal_buffer.cpp`](src/signal_buffer.cpp)

**Назначение**: Управление сигнальными данными (лучами)

**Основные методы**:
- `LoadFromFile(filename)` - загрузка из бинарного файла
- `SaveToFile(filename)` - сохранение в бинарный файл
- `GetBeamData(beam_id)` - доступ к данным луча
- `Resize(num_beams, num_samples)` - изменение размера

**Данные**:
- 1-256 лучей
- 100-1300000 комплексных отсчётов на луч
- Тип: `vector<complex<float>>`

---

### 2. FilterBank
**Файлы**: [`include/filter_bank.h`](include/filter_bank.h), [`src/filter_bank.cpp`](src/filter_bank.cpp)

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
**Файлы**: [`include/lagrange_matrix.h`](include/lagrange_matrix.h), [`src/lagrange_matrix.cpp`](src/lagrange_matrix.cpp)

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
**Файлы**: [`include/processing_pipeline.h`](include/processing_pipeline.h), [`src/processing_pipeline.cpp`](src/processing_pipeline.cpp)

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
**Файлы**: [`include/profiling_engine.h`](include/profiling_engine.h), [`src/profiling_engine.cpp`](src/profiling_engine.cpp)

**Назначение**: Профилирование производительности всех этапов

**Основные методы**:
- `StartTimer(name)` - начать измерение (CPU)
- `StopTimer(name)` - остановить измерение (CPU)
- `RecordGpuEvent(name, time_ms)` - записать GPU событие !! Должны быть времена!
- `ReportMetrics()` - вывести отчёт в консоль
- `SaveReportToJson(filename)` - сохранить отчёт в JSON

**Метрики**:
- Время выполнения каждого этапа
- Минимальное/максимальное/среднее время
- Количество вызовов
- Общее время

---

### 6. IGPUBackend (Интерфейс)
**Файлы**: [`include/gpu_backend/igpu_backend.h`](include/gpu_backend/igpu_backend.h)

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
**Файлы**: [`include/gpu_backend/opencl_backend.h`](include/gpu_backend/opencl_backend.h), [`src/gpu_backend/opencl_backend.cpp`](src/gpu_backend/opencl_backend.cpp)

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
**Файлы**: [`include/gpu_backend/gpu_factory.h`](include/gpu_backend/gpu_factory.h), [`src/gpu_backend/gpu_factory.cpp`](src/gpu_backend/gpu_factory.cpp)

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

### 9. ExecuteFractionalDelayCPU (CPU реализация)
**Файлы**: [`include/fractional_delay_cpu.h`](include/fractional_delay_cpu.h), [`src/fractional_delay_cpu.cpp`](src/fractional_delay_cpu.cpp)

**Назначение**: CPU реализация дробной задержки с интерполяцией Лагранжа 5-го порядка

**Основные методы**:
- `ExecuteFractionalDelayCPU(...)` - выполнение дробной задержки на CPU

**Особенности**:
- Использует тот же алгоритм, что и GPU kernel
- Использует матрицу Лагранжа 48×5 через класс `LagrangeMatrix`
- In-place обработка (результат записывается в тот же буфер)
- Обработка граничных условий идентична GPU (отражение)

**Алгоритм**:
1. Вычисление `delay_integer = floor(delay_coefficients[beam])`
2. Вычисление `delay_fraction = delay_coefficients[beam] - delay_integer`
3. Выбор строки матрицы: `lagrange_row = (int)(delay_fraction * 48)`
4. Интерполяция Лагранжа по 5 точкам [interp_idx, interp_idx+4]
5. Обработка граничных условий (отражение)

---

### 10. CompareResults (Сравнение результатов)
**Файлы**: [`include/result_comparator.h`](include/result_comparator.h), [`src/result_comparator.cpp`](src/result_comparator.cpp)

**Назначение**: Сравнение результатов CPU и GPU обработки

**Основные методы**:
- `CompareResults(...)` - сравнение результатов CPU и GPU

**Метрики сравнения**:
- `max_diff_real` - максимальная разница (real часть)
- `max_diff_imag` - максимальная разница (imag часть)
- `max_diff_magnitude` - максимальная разница по модулю
- `avg_diff_magnitude` - средняя разница по модулю
- `max_relative_error` - максимальная относительная ошибка
- `errors_above_tolerance` - количество точек с превышением tolerance

**Использование**:
- Сравнение результатов CPU и GPU версий дробной задержки
- Валидация корректности GPU реализации
- Анализ точности вычислений

---

## 🔧 OPENCL KERNELS

### kernel_fractional_delay.cl
**Файл**: [`kernels/kernel_fractional_delay.cl`](kernels/kernel_fractional_delay.cl)

**Версия OpenCL C**: Оптимизировано для OpenCL C 3.0 с обратной совместимостью с OpenCL C 1.2

**Компиляция**: 
- Автоматическое определение поддержки OpenCL C 3.0
- Если поддерживается: `-cl-std=CL3.0 -cl-fast-relaxed-math -cl-mad-enable`
- Если нет: `-cl-std=CL1.2 -cl-fast-relaxed-math -cl-mad-enable`
- **Примечание**: NVIDIA RTX 2080 Ti поддерживает только OpenCL C 1.2 (это нормально для NVIDIA)

**Назначение**: Дробная задержка сигнала с интерполяцией Лагранжа 5-го порядка

**Параметры**:
- `input` - входной буфер сигналов
- `output` - выходной буфер (in-place)
- `lagrange_matrix` - матрица коэффициентов [48×5]
- `delay_params` - параметры задержки для каждого луча
- `num_beams`, `num_samples` - размеры

**Оптимизации**:
- ✅ Использование `mad()` функции для быстрого умножения-сложения
- ✅ Полная развёртка цикла интерполяции (5 итераций → 5 отдельных блоков)
- ✅ Предвычисление индексов и коэффициентов Лагранжа для лучшего кэширования
- ✅ Улучшенная обработка граничных условий
- ✅ Векторизация через `float2` для комплексных чисел

**Алгоритм**:
1. Определение луча и отсчёта по `global_id`
2. Получение параметров задержки (delay_integer, lagrange_row)
3. Предвычисление всех 5 индексов интерполяции с граничными условиями
4. Загрузка коэффициентов Лагранжа в локальные переменные
5. Интерполяция Лагранжа по 5 точкам с использованием `mad()`
6. Запись результата (векторная запись `float2`)

---

### kernel_hadamard.cl
**Файл**: [`kernels/kernel_hadamard.cl`](kernels/kernel_hadamard.cl)

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
   ├─ Сохранение JSON отчёта
   └─ Сравнение результатов CPU и GPU (опционально)

### Расширенный pipeline (с сравнением CPU/GPU):

```
1. Инициализация
   ├─ Загрузка матрицы Лагранжа из JSON
   ├─ Инициализация GPU backend
   └─ Загрузка матрицы на GPU

2. Подготовка данных
   └─ Загрузка сигналов (SignalBuffer)

3. CPU обработка
   ├─ Копия исходных данных
   └─ ExecuteFractionalDelayCPU() - дробная задержка на CPU

4. GPU обработка (прямой вызов)
   ├─ H2D Transfer (загрузка данных на GPU)
   ├─ Дробная задержка (gpu_backend->ExecuteFractionalDelay())
   └─ D2H Transfer (вывод с GPU для анализа)

5. Сравнение результатов
   ├─ CompareResults() - сравнение CPU и GPU результатов
   └─ Вывод метрик сравнения

6. Результаты
   ├─ Матрица с задержанными сигналами (CPU и GPU версии)
   ├─ Метрики сравнения
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


### 3. Кроссплатформенность
- OpenCL работает на NVIDIA и AMD GPU
- Единый код для разных платформ
- Автоматический выбор оптимального GPU

---




---

## 🔍 ОТЛАДКА
###  на Windows RTX 2080Ti:

### Ключевые файлы для отладки:

1. **Дробная задержка**:
   - [`src/processing_pipeline.cpp:54`](src/processing_pipeline.cpp:54) - точка входа
   - [`src/gpu_backend/opencl_backend.cpp:159`](src/gpu_backend/opencl_backend.cpp:159) - реализация
   - [`kernels/kernel_fractional_delay.cl:29`](kernels/kernel_fractional_delay.cl:29) - GPU kernel

2. **Профилирование**:
   - [`src/profiling_engine.cpp`](src/profiling_engine.cpp) - все методы профилирования

### Документация по отладке:
- [`Doc/DEBUG_FRACTIONAL_DELAY.md`](Doc/DEBUG_FRACTIONAL_DELAY.md) - детальный маршрут отладки
- [`Doc/FRACTIONAL_DELAY_ROUTE.md`](Doc/FRACTIONAL_DELAY_ROUTE.md) - краткая справка

---

## 🛠️ СБОРКА И ЗАПУСК

### Требования:
- регулируется файлом CMakePresets.json
- Дома сейчас Windows. ( потом работа Ubuntu Linux)
- GCC/Clang компилятор
- CMake 3.20+
- OpenCL SDK (NVIDIA или AMD)
- clFFT библиотека (опционально)

### Сборка:
Существуе F5 (VSCode) & run.bat
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENCL=ON
cmake --build . -j$(nproc)
```
Просьба не переписывать файлы структура должна остаться одна на Windows и Ubuntu:    CMakeList.txt, launch.json & task.json

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
- [`Doc/Plan/00_ОБЩИЙ_ПЛАН.md`](Doc/Plan/00_ОБЩИЙ_ПЛАН.md) - общий план проекта
- [`Doc/Plan/01_ДЕТАЛЬНЫЙ_ПЛАН.md`](Doc/Plan/01_ДЕТАЛЬНЫЙ_ПЛАН.md) - детальный план
- [`Doc/Example/`](Doc/Example/) - примеры и эталоны

### Примеры кода:
- [`Doc/Example/opencl_fractional_delay_256beams.cpp`](Doc/Example/opencl_fractional_delay_256beams.cpp) - эталонная реализация
- [`kernels/kernel_fractional_delay.cl`](kernels/kernel_fractional_delay.cl) - рабочий kernel

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

## 📚 ПАМЯТЬ ПРОЕКТА

### MemoryBank
- [`MemoryBank/key_findings.md`](MemoryBank/key_findings.md) - Ключевые результаты исследований
- [`MemoryBank/AI_SESSION_MEMORY.md`](MemoryBank/AI_SESSION_MEMORY.md) - Память AI сессий

**Правила использования**:
- Всегда проверять MemoryBank перед началом работы
- Обновлять память сессий после важных разговоров
- Использовать sequential-thinking-mcp для сложных проблем

---

*Обновлено: 2025-01-28*
*Версия: 1.2*
*AI Assistant: Кодо (Codo)*

## 📝 ИЗМЕНЕНИЯ В ВЕРСИИ 1.2

### Добавлено:
- CPU реализация дробной задержки (`ExecuteFractionalDelayCPU`)
- Функция сравнения результатов CPU и GPU (`CompareResults`)
- Прямой вызов GPU версии в `main.cpp`
- Сравнение результатов CPU и GPU с метриками

### Обновлено:
- `main.cpp` - добавлены вызовы CPU и GPU версий с сравнением результатов
- `CMakeLists.txt` - добавлены новые файлы в список источников
- `Project_map.md` - добавлены описания новых функций и процесс сравнения

