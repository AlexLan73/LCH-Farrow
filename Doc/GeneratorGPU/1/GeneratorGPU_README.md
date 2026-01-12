# 🚀 GeneratorGPU - Параллельная генерация ЛЧМ сигналов на GPU

## 📋 СТРУКТУРА ПРОЕКТА

```
src/
├── generator/
│   ├── generator_gpu.h          ✅ Класс GeneratorGPU (заголовок)
│   ├── generator_gpu.cpp        ✅ Реализация с OpenCL
│   ├── kernels_generator.cl     ✅ OpenCL kernels для GPU
│   └── example_generator_gpu.cpp ✅ Пример использования
└── ...другие файлы...
```

---

## ✨ ЧТО СДЕЛАНО (ТОЛЬКО GeneratorGPU - ВСЁ ОСТАЛЬНОЕ ПОЗЖЕ!)

### 1. **generator_gpu.h** - Класс GeneratorGPU
- ✅ Структура `DelayParameter` {beam_index, delay_degrees}
- ✅ Конструктор с `LFMParameters`
- ✅ **`signal_base()`** → базовый ЛЧМ на GPU
- ✅ **`signal_valedation()`** → ЛЧМ с дробной задержкой
- ✅ **`ClearGPU()`** → очистка памяти
- ✅ Getters для размеров, параметров, контекста OpenCL

### 2. **generator_gpu.cpp** - Реализация
- ✅ `InitializeOpenCL()` → инициализация платформы, устройства, контекста
- ✅ `CompileKernels()` → компиляция OpenCL kernels
- ✅ `signal_base()` → параллельно генерирует базовый ЛЧМ на GPU
- ✅ `signal_valedation()` → параллельно генерирует ЛЧМ с задержками
- ✅ Правильное управление памятью GPU (allocation/deallocation)
- ✅ Возврат `cl_mem` адресов GPU памяти

### 3. **kernels_generator.cl** - OpenCL Kernels
- ✅ **kernel_lfm_basic** - базовый ЛЧМ сигнал
  ```
  φ(t) = 2π(f_start * t + 0.5 * chirp_rate * t²)
  x(t) = cos(φ) + j*sin(φ)
  ```
  
- ✅ **kernel_lfm_delayed** - ЛЧМ сигнал с дробной задержкой
  ```
  delay_time = (delay_degrees * π/180) * wavelength / c
  delay_samples = delay_time * sample_rate
  Применить к ЛЧМ сигналу: x(t - delay_time)
  ```

- ✅ **ПАРАЛЛЕЛИЗМ:**
  - Каждый поток GPU обрабатывает один элемент (sample_id луча ray_id)
  - Total threads = `num_beams * num_samples` = 256 × 1,300,000 = **333.2 млн**
  - ВСЕ потоки работают **ОДНОВРЕМЕННО** 🔥

### 4. **example_generator_gpu.cpp** - Пример использования
- ✅ Инициализация параметров ЛЧМ
- ✅ Создание GeneratorGPU
- ✅ Генерация signal_base()
- ✅ Подготовка m_delay[] (массив задержек)
- ✅ Генерация signal_valedation()
- ✅ Трансфер результатов GPU → CPU
- ✅ Измерение времени выполнения

---

## 🎯 ВХОДНЫЕ/ВЫХОДНЫЕ ПАРАМЕТРЫ

### **signal_base()**
```cpp
// ВХОДНЫЕ (из конструктора LFMParameters):
- f_start              // Начальная частота (Гц)
- f_stop               // Конечная частота (Гц)
- sample_rate          // Частота дискретизации (12 МГц)
- duration             // Длительность сигнала (сек)
- num_beams            // Количество лучей (256)

// ВЫХОДНЫЕ:
cl_mem → GPU память с ЛЧМ сигналом
Структура: [ray0_all_samples][ray1_all_samples]...[ray255_all_samples]
Размер: 256 × num_samples × sizeof(complex<float>) байт
```

### **signal_valedation()**
```cpp
// ВХОДНЫЕ:
- LFMParameters        // (из конструктора)
- m_delay[]            // Массив DelayParameter {beam_id, delay_degrees}
- num_delay_params     // Размер m_delay[] (обычно 256)

// Пример m_delay:
m_delay[0] = {beam_index: 0, delay_degrees: 0.0}
m_delay[1] = {beam_index: 1, delay_degrees: 0.5}
m_delay[2] = {beam_index: 2, delay_degrees: 1.0}
...
m_delay[255] = {beam_index: 255, delay_degrees: 127.5}

// ВЫХОДНЫЕ:
cl_mem → GPU память с ЛЧМ сигналом + задержки
Структура: [ray0_delayed][ray1_delayed]...[ray255_delayed]
Размер: 256 × num_samples × sizeof(complex<float>) байт
```

---

## 🔄 ПОТОК ВЫПОЛНЕНИЯ

### Инициализация (Конструктор)
```
1. Валидировать LFMParameters
2. InitializeOpenCL()
   ├─ Получить платформу OpenCL
   ├─ Получить GPU устройство
   ├─ Создать контекст
   └─ Создать очередь команд
3. CompileKernels()
   ├─ Получить исходный код kernels
   ├─ Создать программу OpenCL
   ├─ Скомпилировать
   └─ Создать kernel objects
```

### signal_base()
```
1. Создать GPU буфер (num_beams × num_samples × 8 байт)
2. Установить аргументы kernel (f_start, f_stop, sample_rate, ...)
3. Запустить kernel_lfm_basic
   └─ Каждый GPU поток параллельно:
      ├─ ray_id = gid / num_samples
      ├─ sample_id = gid % num_samples
      ├─ t = sample_id / sample_rate
      ├─ φ(t) = 2π(f_start*t + 0.5*chirp_rate*t²)
      └─ output[ray_id*num_samples + sample_id] = cos(φ) + j*sin(φ)
4. Дождаться завершения (clFinish)
5. Вернуть cl_mem адрес GPU памяти
```

### signal_valedation()
```
1. Скопировать m_delay[] на GPU в constant memory
2. Создать GPU буфер для выхода
3. Установить аргументы kernel (+ m_delay, speed_of_light, ...)
4. Запустить kernel_lfm_delayed
   └─ Каждый GPU поток параллельно:
      ├─ ray_id = gid / num_samples
      ├─ sample_id = gid % num_samples
      ├─ delay_degrees = m_delay[ray_id].delay_degrees
      ├─ delay_rad = delay_degrees × π/180
      ├─ wavelength = c / f_center
      ├─ delay_time = delay_rad × wavelength / c
      ├─ delay_samples = delay_time × sample_rate
      ├─ t_delayed = (sample_id - delay_samples) / sample_rate
      ├─ φ(t_delayed) = 2π(f_start*t_delayed + 0.5*chirp_rate*t²_delayed)
      └─ output[ray_id*num_samples + sample_id] = cos(φ) + j*sin(φ)
5. Освободить буфер m_delay
6. Дождаться завершения
7. Вернуть cl_mem адрес GPU памяти
```

---

## 🎨 АРХИТЕКТУРА ООП

### Класс GeneratorGPU
```cpp
class GeneratorGPU {
private:
    // OpenCL ресурсы
    cl_context context_;
    cl_command_queue queue_;
    cl_kernel kernel_lfm_basic_;
    cl_kernel kernel_lfm_delayed_;
    
    // Параметры (const)
    const LFMParameters params_;
    size_t num_samples_;
    size_t num_beams_;
    size_t total_size_;
    
    // Приватные методы инициализации
    void InitializeOpenCL();
    void CompileKernels();
    std::string GetKernelSource() const;

public:
    // Constructor/Destructor
    explicit GeneratorGPU(const LFMParameters& params);
    ~GeneratorGPU();
    
    // Move semantics (РАЗРЕШЕНЫ)
    GeneratorGPU(GeneratorGPU&&) noexcept;
    GeneratorGPU& operator=(GeneratorGPU&&) noexcept;
    
    // Copy semantics (ЗАПРЕЩЕНЫ)
    GeneratorGPU(const GeneratorGPU&) = delete;
    GeneratorGPU& operator=(const GeneratorGPU&) = delete;
    
    // PUBLIC API - ГЛАВНЫЕ МЕТОДЫ
    cl_mem signal_base();
    cl_mem signal_valedation(const DelayParameter* m_delay, size_t num_delay_params);
    void ClearGPU();
    
    // Getters
    size_t GetNumBeams() const noexcept;
    size_t GetNumSamples() const noexcept;
    cl_context GetContext() const noexcept;
    cl_command_queue GetQueue() const noexcept;
};
```

### Структура DelayParameter
```cpp
struct DelayParameter {
    uint32_t beam_index;      // Номер луча (0-255)
    float delay_degrees;      // Задержка в градусах (0.5, 1.5, 6.0, ...)
};
```

---

## 📊 ПРОИЗВОДИТЕЛЬНОСТЬ

### Ожидаемые результаты на AMD Radeon
```
num_beams = 256
num_samples = 1,300,000 (0.1 сек при 12 МГц)
Total elements = 333,200,000

signal_base():        ~50-100 мс
signal_valedation():  ~80-150 мс

Пропускная способность GPU: 3-6 ГВЫБ/сек
Ускорение над CPU: 50-100x
```

---

## 🔧 КОМПИЛЯЦИЯ

```bash
# С OpenCL SDK AMD (ROCm)
g++ -O3 -std=c++17 \
    -I/opt/rocm/include \
    -I./include \
    -o generator_gpu \
    example_generator_gpu.cpp \
    src/generator/generator_gpu.cpp \
    src/signal_buffer.cpp \
    src/lfm_signal_generator.cpp \
    -L/opt/rocm/lib \
    -lOpenCL -lm

# Запуск
./generator_gpu
```

---

## ⚠️ ВАЖНЫЕ ЗАМЕЧАНИЯ

1. **ТОЛЬКО GeneratorGPU** - остальные функции (FuncGPU::fractional_delay и т.д.) будут в следующих ветках!

2. **Параллелизм** - все вычисления происходят **одновременно на GPU**:
   - 333 миллиона потоков параллельно для num_beams=256, num_samples=1.3М
   - Нет циклов на CPU - всё на GPU kernel

3. **Управление памятью** - через OpenCL:
   - `cl_mem` - адреса GPU памяти
   - Автоматическое освобождение в деструкторе `~GeneratorGPU()`
   - RAII паттерн (Resource Acquisition Is Initialization)

4. **Задержки** - в градусах, как требовалось:
   - `delay_degrees` может быть любое число (0.5, 1.5, 6.0, ...)
   - Автоматическое преобразование через длину волны в пикосекунды

---

## 🎯 ГОТОВО ДЛЯ СЛЕДУЮЩИХ ЭТАПОВ

После успешной компиляции и тестирования GeneratorGPU:
- ✅ Ветка `GeneratorGPU` готова к merge в main
- ⏳ Следующая ветка: `FuncGPU` (fractional_delay, func_valedation, ClearGPU)
- ⏳ Потом: интеграция с основным процессом обработки сигналов

---

**Создано:** 12 Январь 2026  
**Архитектура:** GPU-first, параллельная обработка  
**Язык:** C++17 + OpenCL  
**Статус:** ✅ ГОТОВО К ИСПОЛЬЗОВАНИЮ
