# 🔧 ИСПРАВЛЕНИЕ ОШИБКИ -11 (CL_BUILD_PROGRAM_FAILURE)

## ❌ ПРОБЛЕМА
```
err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
if (err != CL_SUCCESS) {  // ❌ ошибка -11
```

Ошибка **-11 = `CL_BUILD_PROGRAM_FAILURE`** означает, что компилятор OpenCL не смог скомпилировать код kernel.

---

## 🎯 ПРИЧИНЫ

### 1. **typedef ПОСЛЕ kernel определений** ❌
В исходном коде `kernels_generator.cl` структура `DelayParam` определена **ПОСЛЕ** kernel функций.

**Неправильно:**
```c
__kernel void kernel_lfm_basic(...) {
    // использование структур, но они еще не определены!
}

typedef struct {
    uint beam_index;
    float delay_degrees;
} DelayParam;  // ❌ Слишком поздно!
```

### 2. **float2 вместо __float2** (в некоторых компиляторах) ⚠️
`float2` в встроенных строках может не работать, нужны точные типы OpenCL.

### 3. **Синтаксис встроенных строк** 
Символы вроде `─`, `═`, `│` могут вызвать проблемы кодировки.

---

## ✅ РЕШЕНИЕ

### **Исправление 1: Переместить typedef в начало**
```c
// ✅ Правильно: typedef в начале файла
typedef struct {
    uint beam_index;
    float delay_degrees;
} DelayParam;

__kernel void kernel_lfm_basic(...) {
    // Теперь структура известна!
}

__kernel void kernel_lfm_delayed(...) {
    // Используем DelayParam спокойно
    float delay = m_delay[ray_id].delay_degrees;
}
```

### **Исправление 2: Использовать только ASCII символы**
```c
// ❌ ПЛОХО (в встроенной строке):
return R"(
    // ═════════════════════════════════════════════════════
    // KERNEL CODE
    // ═════════════════════════════════════════════════════
)";

// ✅ ХОРОШО:
return R"(
    // Kernel code here
    // No fancy Unicode characters
)";
```

### **Исправление 3: Явно типизировать uint вместо uint32_t**
```c
// ❌ Может не работать в OpenCL:
uint32_t beam_index;

// ✅ Правильно в OpenCL C:
uint beam_index;  // OpenCL встроенный тип
```

---

## 📋 ИСПРАВЛЕННЫЙ КОД (generator_gpu.cpp)

В методе `GetKernelSource()`:

```cpp
std::string GeneratorGPU::GetKernelSource() const {
    return R"(
// ✅ typedef в НАЧАЛЕ файла
typedef struct {
    uint beam_index;
    float delay_degrees;
} DelayParam;

// ✅ Без unicode символов в комментариях
__kernel void kernel_lfm_basic(
    __global float2 *output,
    float f_start,
    float f_stop,
    float sample_rate,
    float duration,
    uint num_samples,
    uint num_beams
) {
    uint gid = get_global_id(0);
    
    if (gid >= num_samples * num_beams) return;
    
    uint ray_id = gid / num_samples;
    uint sample_id = gid % num_samples;
    
    // ... код ...
}

__kernel void kernel_lfm_delayed(
    __global float2 *output,
    __constant DelayParam *m_delay,  // ✅ Структура уже известна!
    float f_start,
    float f_stop,
    float sample_rate,
    float duration,
    float speed_of_light,
    uint num_samples,
    uint num_beams,
    uint num_delays
) {
    // ... код ...
}
)";
}
```

---

## 🚀 ШАГ ЗА ШАГОМ

### 1. **Замени generator_gpu.cpp** на `generator_gpu_FIXED.cpp`
```bash
cp generator_gpu_FIXED.cpp src/generator/generator_gpu.cpp
```

### 2. **Проверь изменения:**
- typedef **в начале** R"(...)"
- **Нет unicode** символов в комментариях
- **Все kernel функции** ПОСЛЕ typedef

### 3. **Перекомпилируй:**
```bash
cd build
cmake ..
make -j8
```

### 4. **Проверь вывод при компиляции:**
```
[ 50%] Building CXX object src/generator/CMakeFiles/generator.dir/generator_gpu.cpp.o
[100%] Linking CXX executable test_generator_gpu
[100%] Built target test_generator_gpu
✓ Успешно!
```

---

## 🧪 ОТЛАДКА (если ошибка все еще есть)

### **Получить полный лог ошибки:**

Добавь в CompileKernels():
```cpp
err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
if (err != CL_SUCCESS) {
    size_t log_size = 0;
    clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 
                         0, nullptr, &log_size);
    
    std::string log(log_size, '\0');
    clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 
                         log_size, &log[0], nullptr);
    
    std::cerr << "✗ OPENCL COMPILATION ERROR:\n"
              << log << std::endl;  // ✅ Выведет реальную ошибку!
    
    throw std::runtime_error("OpenCL compilation failed");
}
```

### **Возможные ошибки:**
```
error: undeclared identifier 'DelayParam'
  → typedef должен быть ДО использования

error: syntax error in kernel
  → Проверь кодировку (нет unicode!)

error: invalid conversion from 'float' to '__global float2 *'
  → Проверь типы аргументов kernel
```

---

## 📦 ФАЙЛЫ ДЛЯ ЗАМЕНЫ

| Что | Старый файл | Новый файл | Статус |
|-----|-----------|-----------|--------|
| Заголовок | generator_gpu.h | generator_gpu_FIXED.h | ✅ |
| Реализация | generator_gpu.cpp | generator_gpu_FIXED.cpp | ✅ |
| Kernels | kernels_generator.cl | (встроено в .cpp) | ✅ |

---

## ✨ КАК ИСПОЛЬЗОВАТЬ ИСПРАВЛЕННЫЕ ФАЙЛЫ

### **Вариант 1: Заменить существующие файлы**
```bash
# Backup старых файлов
cp src/generator/generator_gpu.h src/generator/generator_gpu.h.bak
cp src/generator/generator_gpu.cpp src/generator/generator_gpu.cpp.bak

# Скопировать новые
cp generator_gpu_FIXED.h src/generator/generator_gpu.h
cp generator_gpu_FIXED.cpp src/generator/generator_gpu.cpp

# Перекомпилировать
cd build && cmake .. && make
```

### **Вариант 2: Обновить только GetKernelSource()**
Если у тебя уже есть работающий .h файл, просто замени функцию `GetKernelSource()` на версию из `generator_gpu_FIXED.cpp`.

---

## ✅ ПРОВЕРКА УСПЕХА

После исправления должно работать:
```
✓ GPU инициализирована за X мс
✓ signal_base() завершена за X мс
✓ signal_valedation() завершена за X мс
✓ Первый луч, первые 10 отсчётов:
  [0] = 0.123456 + 0.654321j
  [1] = 0.234567 + 0.543210j
  ...
```

---

## 📚 ДОПОЛНИТЕЛЬНО

**Почитай:**
- [OpenCL Specification (Khronos)](https://www.khronos.org/opencl/)
- [OpenCL C Language Reference](https://www.khronos.org/registry/OpenCL/specs/)

**Если все еще не работает:**
1. Скопируй полный лог ошибки из `clGetProgramBuildInfo()`
2. Проверь версию OpenCL (у тебя AMD ROCm?)
3. Убедись, что GPU поддерживает нужные features

---

**Статус:** ✅ ГОТОВО К ИСПОЛЬЗОВАНИЮ  
**Дата:** 12 Января 2026  
