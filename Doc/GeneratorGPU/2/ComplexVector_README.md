# 🚀 ComplexVector - GPU вектор для комплексных чисел

## 📋 АРХИТЕКТУРА

### Цель
Создать удобный RAII-обертку над GPU памятью для комплексных данных с целью:
1. ✅ Двусторонний трансфер CPU↔GPU
2. ✅ Автоматическое управление памятью
3. ✅ FFT-ready интерфейс
4. ✅ Интеграция с GeneratorGPU

---

## 📁 СТРУКТУРА ФАЙЛОВ

```
src/
├── gpu/
│   ├── complex_vector.h       ✅ Заголовок (шаблон класса)
│   ├── complex_vector.cpp     ✅ Реализация с явной инстанциацией
│   └── example_complex_vector.cpp ✅ Пример использования
└── ...
```

---

## 🎯 КОМПОНЕНТЫ

### 1. **ComplexVector<T> - Шаблонный класс**

```cpp
template<typename T>
class ComplexVector {
private:
    cl_context context_;           // OpenCL контекст
    cl_command_queue queue_;       // OpenCL очередь
    cl_mem gpu_buffer_;            // GPU память (std::complex<T>)
    
    size_t num_elements_;          // Количество элементов
    size_t buffer_size_bytes_;     // Размер в байтах
    bool is_allocated_;            // Флаг выделения памяти
};
```

### 2. **Типы данных**

```cpp
using ComplexVectorF = ComplexVector<float>;   // float-based
using ComplexVectorD = ComplexVector<double>;  // double-based

// std::complex<T> хранится как:
// [real0, imag0, real1, imag1, real2, imag2, ...]
// └─── float2 (8 байт) ───┘
```

### 3. **GPU MEMORY LAYOUT**

```
GPU MEMORY (Линейная раскладка):
┌─────────────────────────────────────────┐
│ ComplexVector<float> (256 лучей × 1.3M) │
├─────────────────────────────────────────┤
│ ray0:                                    │
│ [real0, imag0, real1, imag1, ...]      │ 1.3M элементов
│                                         │
│ ray1:                                    │
│ [real0, imag0, real1, imag1, ...]      │ 1.3M элементов
│                                         │
│ ...                                      │
│ ray255:                                  │
│ [real0, imag0, real1, imag1, ...]      │ 1.3M элементов
└─────────────────────────────────────────┘
Всего: 256 × 1.3M × 8 = 3.3 GB
```

---

## 🔄 ИНТЕГРАЦИЯ С GeneratorGPU

### Поток данных:

```
1. GeneratorGPU генерирует сигнал
   └─ signal_base() → cl_mem (GPU адрес)

2. ComplexVector оборачивает cl_mem
   └─ ComplexVector(context, queue, num_elements)

3. Двусторонний трансфер
   ├─ SetData(cpu_data) → CPU → GPU
   └─ GetData() → GPU → CPU

4. FFT kernel может работать напрямую
   └─ clSetKernelArg(kernel, arg, sizeof(cl_mem), &complex_vector.GetMemObject())
```

### Код интеграции:

```cpp
// 1. Создать GeneratorGPU
GeneratorGPU gen(params);
cl_mem signal_gpu = gen.signal_base();

// 2. Создать ComplexVector
ComplexVectorF vector(
    gen.GetContext(),
    gen.GetQueue(),
    num_total_elements
);

// 3. Загрузить данные на GPU
std::vector<std::complex<float>> cpu_data = ...;
vector.SetData(cpu_data);

// 4. Скачать результаты с GPU
std::vector<std::complex<float>> result = vector.GetData();

// 5. Использовать в FFT kernel
cl_mem gpu_mem = vector.GetMemObject();  // Передать в FFT kernel
```

---

## 📚 PUBLIC API

### Constructor

```cpp
ComplexVector<T> vec(context, queue, num_elements);
```

### Data Transfer

```cpp
// CPU → GPU
void SetData(const std::vector<std::complex<T>>& cpu_data);
void SetData(const std::complex<T>* cpu_data, size_t count);

// GPU → CPU
std::vector<std::complex<T>> GetData();                    // Все
std::vector<std::complex<T>> GetData(offset, count);       // С смещением
std::vector<std::complex<T>> GetDataFirst(count);          // Первые N
std::vector<std::complex<T>> GetDataLast(count);           // Последние N
```

### GPU Access

```cpp
// Для kernel
cl_mem GetMemObject() const;
cl_context GetContext() const;
cl_command_queue GetQueue() const;

// Метаданные
size_t Size() const;                    // Количество элементов
size_t SizeBytes() const;               // Размер в байтах
static constexpr size_t ElementSize();  // sizeof(std::complex<T>)
bool IsAllocated() const;
```

### Utility

```cpp
void Flush() const;                     // clFlush()
void Finish() const;                    // clFinish()
std::string GetInfo() const;            // Информация о буфере
```

---

## 💡 ОСОБЕННОСТИ

### 1. **RAII (Resource Acquisition Is Initialization)**

```cpp
{
    ComplexVectorF vec(context, queue, 1000);
    // Память выделена на GPU
    vec.SetData(data);
    vec.GetData();
} // ← Деструктор автоматически освобождает GPU память
```

### 2. **Move semantics**

```cpp
ComplexVectorF vec1(context, queue, 1000);
ComplexVectorF vec2 = std::move(vec1);  // ✅ Разрешено
// vec1 теперь невалиден (moved-from state)
```

### 3. **No Copy (Delete copy)**

```cpp
ComplexVectorF vec1(context, queue, 1000);
ComplexVectorF vec2 = vec1;  // ❌ Ошибка компиляции!
```

### 4. **Type Safety (шаблон)**

```cpp
ComplexVectorF float_vec(context, queue, 1000);  // std::complex<float>
ComplexVectorD double_vec(context, queue, 1000); // std::complex<double>

// Разные типы → разные инстанции
// Проверка типов на compile-time
```

### 5. **Error Handling**

```cpp
try {
    ComplexVectorF vec(nullptr, queue, 1000);  // ❌ Exception!
} catch (const std::invalid_argument& e) {
    // context is nullptr
}

try {
    vec.SetData(nullptr, 100);  // ❌ Exception!
} catch (const std::invalid_argument& e) {
    // cpu_data is nullptr
}
```

---

## 🧪 ПРИМЕР ИСПОЛЬЗОВАНИЯ

```cpp
#include "complex_vector.h"
#include "generator_gpu.h"

int main() {
    // 1. Создать GeneratorGPU
    LFMParameters params = ...;
    GeneratorGPU gen(params);
    
    // 2. Создать ComplexVector
    size_t total = params.num_beams * params.GetNumSamples();
    ComplexVectorF signal(
        gen.GetContext(),
        gen.GetQueue(),
        total
    );
    
    // 3. Загрузить тестовые данные на GPU
    std::vector<std::complex<float>> test_data(total);
    for (size_t i = 0; i < total; ++i) {
        test_data[i] = std::complex<float>(cos(i*0.1), sin(i*0.1));
    }
    signal.SetData(test_data);
    
    // 4. Скачать первые 10 элементов
    auto first10 = signal.GetDataFirst(10);
    for (const auto& val : first10) {
        std::cout << val << "\n";
    }
    
    // 5. Использовать в FFT (позже)
    cl_mem gpu_mem = signal.GetMemObject();
    // clSetKernelArg(kernel_fft, 0, sizeof(cl_mem), &gpu_mem);
    // clEnqueueNDRangeKernel(...);
}
```

---

## 📊 ПРОИЗВОДИТЕЛЬНОСТЬ

### Ожидаемые скорости (AMD Radeon)

```
Размер данных: 333 млн элементов (3.3 GB)

Загрузка на GPU (SetData):     ~1000 мс  → 3.3 Гб/сек
Скачивание с GPU (GetData):    ~1000 мс  → 3.3 Гб/сек

Для сравнения:
- PCIe 3.0: до 16 Гб/сек (теоретический максимум)
- Типичная реальная скорость: 3-5 Гб/сек
```

---

## 🔮 БУДУЩИЕ РАСШИРЕНИЯ

### FFT-специфичные методы

```cpp
// Для будущей FFT реализации:
template<typename T>
class ComplexVectorFFT : public ComplexVector<T> {
    // FFT-специфичные методы
    ComplexVectorFFT& InPlaceFFT();
    ComplexVectorFFT ConvolveWith(const ComplexVector<T>& other);
};
```

### Batch операции

```cpp
// Несколько векторов одновременно
std::vector<ComplexVectorF> batch;
for (size_t i = 0; i < num_beams; ++i) {
    batch.emplace_back(context, queue, num_samples);
    batch[i].SetData(...);
}
```

---

## 🎯 СТАТУС

| Компонент | Статус |
|-----------|--------|
| ComplexVector<T> шаблон | ✅ Done |
| Constructor/Destructor | ✅ Done |
| SetData (CPU→GPU) | ✅ Done |
| GetData (GPU→CPU) | ✅ Done |
| RAII управление памятью | ✅ Done |
| Move semantics | ✅ Done |
| Интеграция с GeneratorGPU | ✅ Done |
| Явная инстанциация (float/double) | ✅ Done |
| Пример использования | ✅ Done |
| FFT-ready API | ✅ Done |

---

## 🚀 ГОТОВО К:

1. **FFT реализации** - ComplexVector готов предоставить GPU данные
2. **Batch обработке** - Несколько векторов одновременно
3. **Интеграции** - Полный pipeline GeneratorGPU → ComplexVector → FFT
4. **Оптимизации** - Можно добавить pinned memory для еще большей скорости

---

**Создано:** 12 Январь 2026  
**Архитектура:** RAII + шаблоны + OpenCL  
**Язык:** C++17  
**Статус:** ✅ ГОТОВО К ИСПОЛЬЗОВАНИЮ
