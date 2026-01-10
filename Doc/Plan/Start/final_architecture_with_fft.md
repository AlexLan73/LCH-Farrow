# 🏛️ ПОЛНАЯ АРХИТЕКТУРА + FFT ПАЙПЛАЙН

## ОБЩАЯ АРХИТЕКТУРА

### 3 слоя:

**Слой 1: C++ Приложение (платформа-независимо)**
- SignalBuffer: загружает 2.66 ГБ
- FilterBank: 240 FIR коэффициентов
- ProcessingPipeline: координирует всё
- ProfilingEngine: измеряет время

**Слой 2: GPU Абстракция (виртуальный интерфейс)**
- IGPUBackend: abstract class
- CUDABackend: реализация для NVIDIA
- HIPBackend: реализация для AMD (будущее)

**Слой 3: GPU Код (специфичный для платформы)**
- CUDA kernels (.cu файлы)
- cuFFT вызовы
- HIP kernels (будущее)

---

## ПАМЯТЬ LAYOUT

### Входные данные
```
Input buffer (host): 256 × 1.3M × 8 bytes = 2.66 GB
  ├─ Complex = 2 floats (real + imag)
  ├─ 8 bytes per complex
  └─ 1.3M samples × 256 beams

Pinned host memory: 2.66 GB
  └─ Для быстрого H2D transfer

Device (GPU) buffer: 2.66 GB (in-place!)
  └─ Переписываем туда же на каждом этапе
```

### GPU память бюджет
```
RTX 2080 Ti: 11 GB total
├─ Input/work buffer: 2.66 GB ✅
├─ cuFFT workspace: ~0.5 GB
├─ Reference FFT: 0.01 GB
└─ Free: ~7.8 GB (запас!)
```

---

## GPU ПАЙПЛАЙН (4.65 сек)

```
ЭТАП 1: H2D Transfer (170 мс)
├─ Pinned host → Device
└─ Используем async если можем

ЭТАП 2: Fractional Delay Kernel (4200 мс) — 90% времени!
├─ 256 × 1.3M потоков
├─ Каждый поток: one sample
├─ In-place обработка (переписываем в тот же буфер)
└─ Синхронизация после

ЭТАП 3: FFT Forward (60 мс)
├─ cuFFT batch plan: size=1.3M, batch=256
├─ In-place: input=output
└─ Все 256 лучей за одно преобразование

ЭТАП 4: Hadamard Multiply (5 мс)
├─ Element-wise умножение
├─ 256 лучей × 1.3M samples
├─ In-place
└─ С предвычисленной опорной FFT

ЭТАП 5: FFT Inverse (60 мс)
├─ cuFFT batch IFFT
├─ In-place
└─ Все 256 лучей за раз

ЭТАП 6: D2H Transfer (170 мс)
├─ Device → Pinned host
└─ Async если возможно

ИТОГО: 4.65 сек ✅
```

---

## СТРУКТУРА ПРОЕКТА

```
RadarConvolver/
├─ .gitignore
├─ README.md
├─ CMakeLists.txt              ← ГЛАВНЫЙ файл сборки
├─ .vscode/
│  ├─ settings.json
│  ├─ launch.json
│  └─ tasks.json
├─ src/
│  ├─ CMakeLists.txt
│  ├─ main.cpp
│  ├─ signal_buffer.h/cpp
│  ├─ filter_bank.h/cpp
│  ├─ gpu_factory.h/cpp
│  ├─ processing_pipeline.h/cpp
│  ├─ profiling_engine.h/cpp
│  └─ gpu_backend/
│     ├─ igpu_backend.h
│     └─ cuda/
│        ├─ CMakeLists.txt
│        ├─ cuda_backend.h/cpp
│        ├─ kernel_fractional_delay.cu
│        ├─ kernel_hadamard.cu
│        └─ cufft_wrapper.h/cpp
├─ data/
│  └─ lfm_signal.bin           ← Тестовые данные
├─ build/                      ← Создаётся при сборке
└─ Doc/
   └─ (эта документация)
```

---

## CMAKELISTS.TXT ШАБЛОН

```cmake
cmake_minimum_required(VERSION 3.20)
project(RadarConvolver LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

# Find CUDA
find_package(CUDAToolkit REQUIRED)

# Find cuFFT
find_package(CUDAToolkit REQUIRED COMPONENTS cufft)

# Main executable
add_executable(radar_convolver
    src/main.cpp
    src/signal_buffer.cpp
    src/filter_bank.cpp
    src/gpu_factory.cpp
    src/processing_pipeline.cpp
    src/profiling_engine.cpp
    src/gpu_backend/cuda/cuda_backend.cpp
    src/gpu_backend/cuda/kernel_fractional_delay.cu
    src/gpu_backend/cuda/kernel_hadamard.cu
    src/gpu_backend/cuda/cufft_wrapper.cpp
)

# Link libraries
target_link_libraries(radar_convolver
    CUDA::cufft
    CUDA::cudart
)

# CUDA architecture
set_property(TARGET radar_convolver PROPERTY CUDA_ARCHITECTURES 75 86)

# Include dirs
target_include_directories(radar_convolver PRIVATE src)
```

---

## GPU ЯДРА СПЕЦИФИКАЦИЯ

### kernel_fractional_delay.cu
```
Input: d_input[256 * 1.3M complex]
Output: d_output (same buffer, in-place)
Parameters:
  - num_beams = 256
  - num_samples = 1.3M
  - delay_samples = fractional delay amount

Grid: 256 blocks × 256 threads
Block: 256 threads
  → 256 × 256 = 65536 parallel threads
  → Process 256 beams × 256 samples per block iteration

Time: 4.2 seconds on RTX 2080 Ti
```

### kernel_hadamard.cu
```
Input: d_beam[1.3M complex], d_reference_fft[1.3M complex]
Output: d_beam (same buffer, in-place)

Операция: d_beam[i] *= d_reference_fft[i]

Grid: 256 blocks × 512 threads
Time: 5 ms on RTX 2080 Ti
```

---

## ВАЖНЫЕ ДЕТАЛИ

### In-place Memory
```
WRONG:
  kernel<<<>>>(d_in, d_out);  // Uses 2× memory!

RIGHT:
  kernel<<<>>>(d_buffer, d_buffer);  // Same buffer!
```

### Batch FFT
```
SLOW (256 individual calls):
  for (b = 0; b < 256; b++) {
      cufftExec();  // 60ms × 256 = 15 sec ❌
  }

FAST (batch of 256):
  cufftPlan1d(..., batch=256);
  cufftExec();  // 60 ms ✅
```

### Reference FFT Caching
```
SLOW:
  for (b = 0; b < 256; b++) {
      reference_fft = fft(reference);  // 60ms × 256 ❌
  }

FAST:
  reference_fft = fft(reference);  // 60 ms, once
  for (b = 0; b < 256; b++) {
      multiply(beam[b], reference_fft);  // 5ms × 256 ✅
  }
```

---

## СБОРКА И ЗАПУСК

### Windows
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -DCUDA_TOOLKIT_ROOT_DIR="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.0"
cmake --build . --config Release
.\Release\radar_convolver.exe
```

### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
./radar_convolver
```

---

## STATUS: ПОЛНАЯ АРХИТЕКТУРА ГОТОВА ✅