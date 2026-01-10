# 🎮 МНОГОПЛАТФОРМЕННАЯ СТРАТЕГИЯ

## ВИРТУАЛЬНЫЙ BACKEND ПАТТЕРН

### Абстрактный интерфейс (один для всех!)
```cpp
class IGPUBackend {
public:
    virtual void ExecuteConvolution(
        complex<float>* buffer,
        const float* coefficients,
        int num_beams,
        int num_samples
    ) = 0;
    
    virtual string GetBackendName() = 0;
    virtual ~IGPUBackend() = default;
};
```

### CUDA Реализация (NVIDIA)
```cpp
class CUDABackend : public IGPUBackend {
public:
    void ExecuteConvolution(...) override {
        // H2D
        // kernel_fractional_delay
        // cuFFT forward
        // kernel_hadamard
        // cuFFT inverse
        // D2H
    }
    
    string GetBackendName() override {
        return "CUDA (NVIDIA)";
    }
};
```

### HIP Реализация (AMD) — будущее
```cpp
class HIPBackend : public IGPUBackend {
public:
    void ExecuteConvolution(...) override {
        // Аналогично CUDA, но с HIP вызовами
        // hipMemcpy вместо cudaMemcpy
        // hipLaunchKernelGGL вместо <<<>>>
        // rocfft вместо cuFFT
    }
    
    string GetBackendName() override {
        return "HIP (AMD)";
    }
};
```

---

## GPU АВТООПРЕДЕЛЕНИЕ

```cpp
class GPUFactory {
public:
    static IGPUBackend* CreateBackend() {
        int num_devices = 0;
        
        // Check CUDA
        #ifdef CUDA_AVAILABLE
        cudaGetDeviceCount(&num_devices);
        if (num_devices > 0) {
            // Выбираем лучший CUDA GPU
            return new CUDABackend();
        }
        #endif
        
        // Check HIP
        #ifdef HIP_AVAILABLE
        hipGetDeviceCount(&num_devices);
        if (num_devices > 0) {
            // Выбираем лучший HIP GPU
            return new HIPBackend();
        }
        #endif
        
        throw runtime_error("No GPU found!");
    }
};
```

---

## ПРИОРИТЕТ GPU

1. **RTX 2080 Ti** (если на Windows дома) → CUDA
2. **MI300X** (если на Linux с AMD) → HIP, 1.5-2 сек
3. **RX 6900 XT** (если есть) → HIP, 6-8 сек
4. **RTX 3060** (если на Linux на работе) → CUDA, 7.3 сек

---

## БУДУЩИЕ ПОРТЫ

### Добавить RX 6900 XT
```bash
git checkout -b feature/hip-backend
# 1. Создайте hip_backend.h/cpp
# 2. Скопируйте логику из cuda_backend.cpp
# 3. Замените CUDA вызовы на HIP
# 4. Тестируйте на RX 6900 XT
git commit -m "HIP backend: RX 6900 XT support"
git push origin feature/hip-backend
```

### Добавить MI300X
```bash
git checkout -b feature/mi300x-tensor
# 1. Используйте HIP backend как базу
# 2. Используйте tensor operations для ускорения
# 3. Оптимизируйте для CDNA3 архитектуры
# 4. Ожидайте 1.5-2 сек
git commit -m "MI300X: tensor-optimized convolution"
```

---

## CMAKE ДЛЯ МНОГОПЛАТФОРМЫ

```cmake
option(ENABLE_CUDA "Enable CUDA support" ON)
option(ENABLE_HIP "Enable HIP support" OFF)

if(ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    add_definitions(-DCUDA_AVAILABLE)
endif()

if(ENABLE_HIP)
    find_package(HIP REQUIRED)
    add_definitions(-DHIP_AVAILABLE)
endif()

if(NOT ENABLE_CUDA AND NOT ENABLE_HIP)
    message(FATAL_ERROR "At least one GPU backend required!")
endif()
```

---

## STATUS: ГОТОВО ДЛЯ РАСШИРЕНИЯ ✅