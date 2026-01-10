# План: Добавление CPU и GPU вызовов дробной задержки в main.cpp

## Цель

Дополнить `main.cpp` вызовами CPU и GPU реализаций дробной задержки для сравнения результатов. Последовательность: CPU → GPU → сравнение результатов.

## Текущее состояние

- ✅ GPU реализация уже существует: `OpenCLBackend::ExecuteFractionalDelay()`
- ✅ Класс `LagrangeMatrix` реализован и работает с матрицей 48×5
- ✅ В `main.cpp` уже загружается матрица Лагранжа
- ❌ CPU реализация дробной задержки отсутствует
- ❌ Функция сравнения результатов отсутствует

## Важные детали

### Использование матрицы Лагранжа 48×5

- **Размер матрицы**: 48 строк × 5 столбцов
- **ROWS = 48**: количество возможных дробных задержек (разрешение 1/48 = 2.083%)
- **COLS = 5**: количество коэффициентов полинома Лагранжа 5-го порядка
- **Использование**: `LagrangeMatrix::GetCoefficient(row, col)` для доступа к коэффициентам
- **Выбор строки**: `lagrange_row = (int)(delay_fraction * 48)` из `LagrangeMatrix::GetRowIndex(delay_fraction)`

### Алгоритм дробной задержки

Алгоритм должен быть идентичен GPU kernel (`kernels/kernel_fractional_delay.cl`):

1. Для каждого луча (beam):
   - Вычислить `delay_integer = floor(delay_coefficients[beam])`
   - Вычислить `delay_fraction = delay_coefficients[beam] - delay_integer`
   - Найти `lagrange_row = (int)(delay_fraction * 48)` через `LagrangeMatrix::GetRowIndex(delay_fraction)`

2. Для каждого отсчёта (sample) в луче:
   - Вычислить `interp_idx = sample - delay_integer - 2`
   - Интерполяция Лагранжа по 5 точкам [interp_idx, interp_idx+4]:
     ```cpp
     result = 0.0f + 0.0fi
     for i in [0, 4]:
         idx = interp_idx + i
         // Обработка граничных условий (отражение, как в GPU kernel)
         if (idx < 0) idx = -idx
         if (idx >= num_samples) idx = 2 * num_samples - idx - 2
         if (idx >= 0 && idx < num_samples):
             coeff = lagrange_matrix.GetCoefficient(lagrange_row, i)
             sample_data = input[beam][idx]
             result += coeff * sample_data  // комплексное умножение
     output[beam][sample] = result
     ```

## Задачи

### 1. Создать CPU реализацию дробной задержки

**Файл**: [`include/fractional_delay_cpu.h`](include/fractional_delay_cpu.h) (новый)

**Интерфейс**:
```cpp
bool ExecuteFractionalDelayCPU(
    SignalBuffer* input_output,      // In-place обработка
    const LagrangeMatrix* lagrange_matrix,
    const float* delay_coefficients,  // Массив для каждого луча
    size_t num_beams,
    size_t num_samples
);
```

**Особенности**:
- Использует существующий класс `LagrangeMatrix` (48×5)
- Использует тот же алгоритм, что и GPU kernel
- Обработка граничных условий идентична GPU (отражение)
- In-place обработка (результат записывается в тот же буфер)

**Реализация**: [`src/fractional_delay_cpu.cpp`](src/fractional_delay_cpu.cpp) (новый)

### 2. Создать функцию сравнения результатов

**Файл**: [`include/result_comparator.h`](include/result_comparator.h) (новый)

**Интерфейс**:
```cpp
struct ComparisonMetrics {
    float max_diff_real;      // Максимальная разница (real)
    float max_diff_imag;      // Максимальная разница (imag)
    float max_diff_magnitude; // Максимальная разница по модулю
    float avg_diff_magnitude; // Средняя разница по модулю
    float max_relative_error; // Максимальная относительная ошибка
    size_t errors_above_tolerance; // Количество точек с превышением tolerance
    size_t total_points;      // Всего точек для сравнения
};

bool CompareResults(
    const SignalBuffer* cpu_results,
    const SignalBuffer* gpu_results,
    float tolerance,
    ComparisonMetrics* metrics
);
```

**Реализация**: [`src/result_comparator.cpp`](src/result_comparator.cpp) (новый)

### 3. Добавить вызов CPU версии в main.cpp

**Файл**: [`src/main.cpp`](src/main.cpp)

**Местоположение**: После генерации сигналов (после строки 59), перед инициализацией GPU

**Изменения**:
1. Создать копию исходных данных для CPU обработки
2. Подготовить коэффициенты задержки (можно использовать те же, что для генерации сигналов)
3. Вызвать CPU версию с профилированием
4. Сохранить результаты CPU обработки

**Пример кода**:
```cpp
// Копия данных для CPU обработки
SignalBuffer cpu_signal_buffer(num_beams, num_samples);
for (size_t beam = 0; beam < num_beams; ++beam) {
    const auto* src = signal_buffer.GetBeamData(beam);
    auto* dst = cpu_signal_buffer.GetBeamData(beam);
    std::memcpy(dst, src, num_samples * sizeof(SignalBuffer::ComplexType));
}

// Коэффициенты задержки (используем те же, что для генерации)
std::vector<float> delay_coeffs(num_beams);
for (size_t beam = 0; beam < num_beams; ++beam) {
    delay_coeffs[beam] = beam * 0.125f;  // 0.0, 0.125, 0.25, 0.375
}

// Выполнение CPU версии дробной задержки
std::cout << "\n=== CPU ВЕРСИЯ (дробная задержка) ===\n";
profiler.StartTimer("FractionalDelay_CPU");
if (!ExecuteFractionalDelayCPU(&cpu_signal_buffer, &lagrange_matrix, 
                                delay_coeffs.data(), num_beams, num_samples)) {
    std::cerr << "Ошибка при выполнении CPU версии дробной задержки\n";
    return 1;
}
profiler.StopTimer("FractionalDelay_CPU");
std::cout << "✅ CPU версия выполнена\n";
```

### 4. Добавить прямой вызов GPU версии в main.cpp

**Файл**: [`src/main.cpp`](src/main.cpp)

**Местоположение**: После CPU версии, перед или вместо `pipeline.ExecuteFull()`

**Изменения**:
1. Выделить память на GPU для буфера сигналов
2. Скопировать исходные данные на GPU (H2D)
3. Вызвать `gpu_backend->ExecuteFractionalDelay()` напрямую
4. Скопировать результаты с GPU (D2H) для сравнения
5. Профилировать все этапы

**Пример кода**:
```cpp
// Выполнение GPU версии дробной задержки (прямой вызов)
std::cout << "\n=== GPU ВЕРСИЯ (дробная задержка, прямой вызов) ===\n";

// Выделить память на GPU
size_t buffer_size = num_beams * num_samples * sizeof(SignalBuffer::ComplexType);
void* gpu_buffer = gpu_backend->AllocateDeviceMemory(buffer_size);
if (!gpu_buffer) {
    std::cerr << "Ошибка: не удалось выделить память на GPU\n";
    return 1;
}

// Подготовить данные для H2D
std::vector<SignalBuffer::ComplexType> host_buffer(num_beams * num_samples);
for (size_t beam = 0; beam < num_beams; ++beam) {
    const auto* beam_data = signal_buffer.GetBeamData(beam);
    std::memcpy(host_buffer.data() + beam * num_samples, 
                beam_data, num_samples * sizeof(SignalBuffer::ComplexType));
}

// H2D Transfer
profiler.StartTimer("H2D_Transfer_Direct");
if (!gpu_backend->CopyHostToDevice(gpu_buffer, host_buffer.data(), buffer_size)) {
    std::cerr << "Ошибка при копировании данных на GPU\n";
    return 1;
}
profiler.StopTimer("H2D_Transfer_Direct");

// Выполнить дробную задержку
profiler.StartTimer("FractionalDelay_GPU");
if (!gpu_backend->ExecuteFractionalDelay(gpu_buffer, delay_coeffs.data(), 
                                         num_beams, num_samples)) {
    std::cerr << "Ошибка при выполнении GPU версии дробной задержки\n";
    return 1;
}
profiler.StopTimer("FractionalDelay_GPU");

// D2H Transfer (для сравнения)
profiler.StartTimer("D2H_Transfer_Direct");
std::vector<SignalBuffer::ComplexType> gpu_result_buffer(num_beams * num_samples);
if (!gpu_backend->CopyDeviceToHost(gpu_result_buffer.data(), gpu_buffer, buffer_size)) {
    std::cerr << "Ошибка при копировании результатов с GPU\n";
    return 1;
}
profiler.StopTimer("D2H_Transfer_Direct");

// Скопировать результаты в SignalBuffer для сравнения
SignalBuffer gpu_signal_buffer(num_beams, num_samples);
for (size_t beam = 0; beam < num_beams; ++beam) {
    auto* beam_data = gpu_signal_buffer.GetBeamData(beam);
    std::memcpy(beam_data, gpu_result_buffer.data() + beam * num_samples,
                num_samples * sizeof(SignalBuffer::ComplexType));
}

// Освободить память на GPU
gpu_backend->FreeDeviceMemory(gpu_buffer);

std::cout << "✅ GPU версия выполнена\n";
```

### 5. Добавить сравнение результатов в main.cpp

**Файл**: [`src/main.cpp`](src/main.cpp)

**Местоположение**: После выполнения GPU версии

**Изменения**:
1. Вызвать `CompareResults()` для сравнения CPU и GPU результатов
2. Вывести метрики сравнения
3. Предупредить о различиях, если они превышают tolerance

**Пример кода**:
```cpp
// Сравнение результатов CPU и GPU
std::cout << "\n=== СРАВНЕНИЕ РЕЗУЛЬТАТОВ (CPU vs GPU) ===\n";
ComparisonMetrics metrics;
float tolerance = 1e-5f;  // Допустимая погрешность

if (!CompareResults(&cpu_signal_buffer, &gpu_signal_buffer, tolerance, &metrics)) {
    std::cerr << "Ошибка при сравнении результатов\n";
    return 1;
}

// Вывести метрики
std::cout << "Метрики сравнения:\n";
std::cout << "  Максимальная разница (real): " << metrics.max_diff_real << "\n";
std::cout << "  Максимальная разница (imag): " << metrics.max_diff_imag << "\n";
std::cout << "  Максимальная разница (модуль): " << metrics.max_diff_magnitude << "\n";
std::cout << "  Средняя разница (модуль): " << metrics.avg_diff_magnitude << "\n";
std::cout << "  Максимальная относительная ошибка: " << metrics.max_relative_error << "\n";
std::cout << "  Точки с превышением tolerance (" << tolerance << "): " 
          << metrics.errors_above_tolerance << " / " << metrics.total_points << "\n";

if (metrics.errors_above_tolerance == 0) {
    std::cout << "✅ Результаты CPU и GPU идентичны (в пределах tolerance)\n";
} else {
    std::cout << "⚠️  Обнаружены различия между CPU и GPU результатами\n";
}
```

### 6. Обновить CMakeLists.txt

**Файл**: [`src/CMakeLists.txt`](src/CMakeLists.txt)

**Изменения**:
- Добавить `fractional_delay_cpu.cpp` в список источников
- Добавить `result_comparator.cpp` в список источников

### 7. Обновить Project_map.md

**Файл**: [`Project_map.md`](Project_map.md)

**Изменения**:
- Добавить описание новых классов/функций:
  - `ExecuteFractionalDelayCPU()` - CPU реализация дробной задержки
  - `CompareResults()` - функция сравнения результатов
- Обновить структуру проекта
- Добавить информацию о сравнении CPU/GPU результатов

## Последовательность реализации

1. ✅ Изучить существующий код (GPU реализацию и алгоритм)
2. Создать CPU реализацию (`fractional_delay_cpu.h/cpp`) - использует `LagrangeMatrix` 48×5
3. Создать функцию сравнения (`result_comparator.h/cpp`)
4. Обновить `CMakeLists.txt`
5. Добавить CPU вызов в `main.cpp`
6. Добавить прямой GPU вызов в `main.cpp`
7. Добавить сравнение результатов в `main.cpp`
8. Обновить `Project_map.md`

## Критерии готовности

- [ ] CPU версия использует класс `LagrangeMatrix` 48×5
- [ ] CPU версия использует тот же алгоритм, что и GPU kernel
- [ ] CPU версия компилируется и работает
- [ ] GPU версия вызывается напрямую через `gpu_backend->ExecuteFractionalDelay()`
- [ ] Результаты CPU и GPU сравниваются
- [ ] Метрики сравнения выводятся в консоль
- [ ] Профилирование работает для обеих версий
- [ ] Код компилируется без ошибок
- [ ] Проект собирается на clean clone

## Зависимости

- `SignalBuffer` - для работы с данными
- `LagrangeMatrix` - для коэффициентов интерполяции 48×5 (существующий класс)
- `IGPUBackend` - для GPU вызова (существующий интерфейс)
- `ProfilingEngine` - для профилирования (существующий класс)

## Примечания

- **Матрица Лагранжа 48×5**: Обязательно использовать существующий класс `LagrangeMatrix` с методами `GetCoefficient(row, col)` и `GetRowIndex(delay_fraction)`
- **Идентичность алгоритмов**: CPU версия должна использовать тот же алгоритм, что и GPU kernel для корректного сравнения
- **Граничные условия**: Обработка границ (отражение) должна быть идентична GPU kernel
- **In-place обработка**: CPU версия должна работать in-place, как и GPU версия
- **Точность сравнения**: По умолчанию `tolerance = 1e-5f`, но можно настроить

---

*Дата создания: 2025-01-28*  
*Версия: 1.0*  
*Автор: Кодо (AI Assistant)*

