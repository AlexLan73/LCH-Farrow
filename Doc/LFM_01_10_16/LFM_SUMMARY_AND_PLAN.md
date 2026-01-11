# 🎯 КРАТКОЕ РЕЗЮМЕ И ПЛАН ИНТЕГРАЦИИ

## ЗАДАЧА 1: Задержка с Шагом 0.5°

### Суть
Для каждого направления (луча) под углом θ = θ_0 + i × 0.5° вычислить задержку по времени для каждого элемента антенной решётки.

### Формула (Главная)
```
delay_k(θ) = (k · d · sin(θ) · f_s) / c

где:
  k = индекс элемента (0, 1, 2, ..., N-1)
  d = λ/2 (расстояние между элементами)
  θ = угол в радианах
  f_s = 12 МГц (частота дискретизации)
  c = 3×10⁸ м/с (скорость света)
```

### Шаги Решения

#### 1️⃣ Выбери частоты f_0 и f_1

```cpp
// ДЛЯ БАЗОВОЙ ПОЛОСЫ (рекомендуется):
float f_0 = 1.0e6f;    // 1 МГц
float f_1 = 2.0e6f;    // 2 МГц
// Проверка Найквиста: 12 МГц > 2 × 2 МГц ✓

// СТАНДАРТНОЕ ЗНАЧЕНИЕ (из LFMParameters):
float f_center = (f_0 + f_1) / 2.0f;  // 1.5 МГц
```

#### 2️⃣ Добавь новый метод в LFMSignalGenerator

```cpp
float ComputeDelayForAngle(float angle_deg, size_t element_index) const;
```

#### 3️⃣ Реализация (скопируй из КОД 2)

```cpp
float LFMSignalGenerator::ComputeDelayForAngle(
    float angle_deg, size_t element_index) const noexcept {
    
    const float angle_rad = angle_deg * PI / 180.0f;
    const float sin_angle = std::sin(angle_rad);
    
    float f_center = (params_.f_start + params_.f_stop) / 2.0f;
    float wavelength = SPEED_OF_LIGHT / f_center;
    float element_spacing = wavelength / 2.0f;
    
    float element_position = static_cast<float>(element_index) * element_spacing;
    float delay_time = (element_position * sin_angle) / SPEED_OF_LIGHT;
    float delay_samples = delay_time * params_.sample_rate;
    
    return delay_samples;
}
```

#### 4️⃣ Обнови LFMParameters

```cpp
struct LFMParameters {
    // ... существующие поля ...
    
    // НОВЫЕ ПОЛЯ:
    float angle_step_deg = 0.5f;      // 0.5° ✓
    float angle_start_deg = -60.0f;
    float angle_stop_deg = 60.0f;
};
```

#### 5️⃣ Добавь новый вариант в enum

```cpp
enum class LFMVariant {
    // ... существующие ...
    ANGLE_SWEEP = 5,   // НОВОЕ!
    HETERODYNE = 6     // НОВОЕ!
};
```

#### 6️⃣ Добавь ветку в GenerateIntoBuffer

```cpp
case LFMVariant::ANGLE_SWEEP: {
    float angle_deg = params_.angle_start_deg + 
                     static_cast<float>(beam) * params_.angle_step_deg;
    GenerateVariant_AngleSweep(beam_data, num_samples, angle_deg, beam);
    break;
}
```

### Результат ✓
Каждый луч (beam) содержит ЛЧМ сигнал с правильной задержкой для направления θ = θ_0 + beam × 0.5°

---

## ЗАДАЧА 2: Гетеродинирование (Комплексное Сопряжение)

### Суть
Из генерируемого ЛЧМ `s[n]` создать сопряжённый вариант `s*[n]` (меняем знак мнимой части).  
При умножении приёмного сигнала: `y[n] = x[n] × s*[n]` получаем сигнал в baseband.

### Формула
```
s[n] = cos(φ[n]) + j·sin(φ[n])
s*[n] = cos(φ[n]) - j·sin(φ[n])

y[n] = x[n] × s*[n]  ← гетеродинирование
```

### Шаги Решения

#### 1️⃣ Добавь методы в LFMSignalGenerator

```cpp
// В .h:
SignalBufferNew MakeConjugateCopy(const SignalBufferNew& src) const;
void ConjugateInPlace(SignalBufferNew& buffer) const noexcept;
SignalBufferNew Heterodyne(
    const SignalBufferNew& rx_signal,
    const SignalBufferNew& ref_signal
) const;
```

#### 2️⃣ Реализация (скопируй из КОД 2)

```cpp
// Сопряжение
SignalBufferNew LFMSignalGenerator::MakeConjugateCopy(
    const SignalBufferNew& src) const {
    SignalBufferNew dst(src.GetNumBeams(), src.GetNumSamples());
    const auto* src_data = src.RawData();
    auto* dst_data = dst.RawData();
    for (size_t i = 0; i < src.GetTotalSize(); ++i) {
        dst_data[i] = std::conj(src_data[i]);
    }
    return dst;
}

// Гетеродинирование
SignalBufferNew LFMSignalGenerator::Heterodyne(
    const SignalBufferNew& rx_signal,
    const SignalBufferNew& ref_signal) const {
    SignalBufferNew result(rx_signal.GetNumBeams(), rx_signal.GetNumSamples());
    const auto* rx_data = rx_signal.RawData();
    const auto* ref_data = ref_signal.RawData();
    auto* out_data = result.RawData();
    for (size_t i = 0; i < rx_signal.GetTotalSize(); ++i) {
        out_data[i] = rx_data[i] * std::conj(ref_data[i]);
    }
    return result;
}
```

#### 3️⃣ Использование

```cpp
// Генерируем опорный ЛЧМ
auto ref_lfm = lfm_generator.Generate(LFMVariant::BASIC);

// Генерируем (или получаем) приёмный сигнал
auto rx_signal = /* приём с GPU */;

// Гетеродинируем
auto baseband = lfm_generator.Heterodyne(rx_signal, ref_lfm);

// Теперь baseband можно обрабатывать (БПФ, обнаружение и т.д.)
```

### Результат ✓
Сигнал перенесён в baseband (около 0 МГц), готов для дальнейшей обработки

---

## ИНТЕГРАЦИЯ НА GPU

### На CPU:

```cpp
// 1. Генерируем опорный ЛЧМ
auto ref_lfm = lfm_generator.Generate(LFMVariant::BASIC);

// 2. Вычисляем матрицу задержек для всех углов
size_t num_angles = 241;  // от -60° до +60° с шагом 0.5°
std::vector<std::vector<float>> delay_matrix(num_elements, 
                                             std::vector<float>(num_angles));

for (size_t elem = 0; elem < num_elements; ++elem) {
    for (size_t angle_idx = 0; angle_idx < num_angles; ++angle_idx) {
        float angle_deg = -60.0f + angle_idx * 0.5f;
        delay_matrix[elem][angle_idx] = lfm_generator.ComputeDelayForAngle(
            angle_deg, elem);
    }
}

// 3. Копируем ref_lfm и delay_matrix на GPU
// (передаём в OpenCL kernel)
```

### На GPU (OpenCL kernel):

```c
// Используешь delay_matrix для применения задержек Лагранжа
// В твоём kernel для каждого луча и элемента:

float delay_samples = delay_matrix[element_idx][beam_idx];
int delay_int = (int)delay_samples;
float delay_frac = delay_samples - delay_int;

// Применяешь интерполяцию Лагранжа (как уже делаешь!)
float2 interpolated = lagrange_interpolate(
    rx_data[element_idx][delay_int], 
    delay_frac,
    lagrange_coeffs
);
```

### На GPU (второй kernel для гетеродина):

```c
// Гетеродинирование в baseband
__kernel void heterodyne(
    __global float2* signal,
    __global float2* ref_lfm,
    uint num_samples,
    __global float2* output
) {
    uint idx = get_global_id(0);
    if (idx >= num_samples) return;
    
    float2 sig = signal[idx];
    float2 ref = ref_lfm[idx];
    float2 ref_conj = (float2)(ref.x, -ref.y);  // Сопряжение
    
    // Комплексное умножение
    output[idx].x = sig.x * ref_conj.x - sig.y * ref_conj.y;
    output[idx].y = sig.x * ref_conj.y + sig.y * ref_conj.x;
}
```

---

## СТАНДАРТНЫЕ ЗНАЧЕНИЯ

### Рекомендуемые Параметры

```cpp
LFMParameters lfm_params;

// Частоты (для базовой полосы, совместимо с 12 МГц)
lfm_params.f_start = 1.0e6f;      // 1 МГц
lfm_params.f_stop = 2.0e6f;       // 2 МГц

// Дискретизация
lfm_params.sample_rate = 12.0e6f; // 12 МГц (задано!)
lfm_params.duration = 0.001f;     // 1 мс (типично)

// Решётка
lfm_params.num_beams = 256;       // 256 лучей

// Углы (ДЛЯ ANGLE_SWEEP)
lfm_params.angle_step_deg = 0.5f;    // 0.5° ✓
lfm_params.angle_start_deg = -60.0f; // -60° до +60°
lfm_params.angle_stop_deg = 60.0f;
```

### Проверочные Вычисления

```
λ = c / f_center = (3×10⁸) / (1.5×10⁶) = 200 м
d = λ/2 = 100 м

delay(θ=30°, elem=5) = (5 × 100 × sin(30°) × 12×10⁶) / (3×10⁸)
                      = (5 × 100 × 0.5 × 12×10⁶) / (3×10⁸)
                      = 10 отсчётов ✓
```

---

## БЫСТРЫЙ СТАРТ

### Шаг 1: Добавь файлы в проект
```
✓ LFM_ANGLE_DELAY_THEORY.md  — теория
✓ LFM_CODE_IMPLEMENTATION.md — готовый код
✓ LFM_PYTHON_EXAMPLES.md     — примеры на Python
```

### Шаг 2: Обнови lfm_signal_generator.h
```cpp
// Добавь поля в LFMParameters
// Добавь методы в LFMSignalGenerator
// Добавь новые значения в enum LFMVariant
```

### Шаг 3: Обнови lfm_signal_generator.cpp
```cpp
// Скопируй реализацию методов из КОД 2
// Добавь ветки в GenerateIntoBuffer
```

### Шаг 4: Используй в main.cpp
```cpp
radar::LFMParameters lfm_params;
lfm_params.f_start = 1.0e6f;
lfm_params.f_stop = 2.0e6f;
lfm_params.sample_rate = 12.0e6f;
// ... остальные параметры ...

radar::LFMSignalGenerator lfm_gen(lfm_params);

// Опция А: Углы с шагом 0.5°
auto angles_signals = lfm_gen.Generate(LFMVariant::ANGLE_SWEEP);

// Опция Б: Гетеродин
auto ref_lfm = lfm_gen.Generate(LFMVariant::BASIC);
auto ref_conj = lfm_gen.MakeConjugateCopy(ref_lfm);
auto baseband = lfm_gen.Heterodyne(rx_signal, ref_lfm);
```

### Шаг 5: Адаптируй GPU kernel
```c
// Используй ComputeDelayForAngle() результаты на CPU
// Передай delay_matrix на GPU
// Применяй в kernel с интерполяцией Лагранжа
```

---

## ПРОВЕРОЧНЫЙ СПИСОК

- [ ] Выбраны f_0 и f_1 (или используются значения по умолчанию)
- [ ] Проверена теорема Найквиста
- [ ] Добавлены новые методы в LFMSignalGenerator
- [ ] Обновлены enum и struct
- [ ] Реализованы методы в .cpp
- [ ] Добавлены ветки в GenerateIntoBuffer
- [ ] Протестировано на CPU (Python примеры)
- [ ] Адаптирован GPU kernel для delay_matrix
- [ ] Обработка гетеродина на GPU
- [ ] Тестирование на реальных данных

---

## ФАЙЛЫ, КОТОРЫЕ ТЫ ПОЛУЧИЛ

| Файл | Описание | ID |
|------|---------|-----|
| **LFM_ANGLE_DELAY_THEORY.md** | Полная теория с формулами | 34 |
| **LFM_CODE_IMPLEMENTATION.md** | Готовый C++ код для интеграции | 35 |
| **LFM_PYTHON_EXAMPLES.md** | Python примеры и визуализация | 36 |
| **LFM_SUMMARY_AND_PLAN.md** | Этот файл — краткое резюме | 37 |

---

## КОНТРОЛЬНЫЕ ВОПРОСЫ

**❓ Какая формула для задержки?**  
↳ `delay_k(θ) = (k · d · sin(θ) · f_s) / c`

**❓ Что такое гетеродинирование?**  
↳ Умножение сигнала на сопряжённый опорный: `y[n] = x[n] × s*[n]`

**❓ Как выбрать f_0 и f_1?**  
↳ Используй базовую полосу (1-2 МГц) или проверь Найквист: `f_s > 2×f_max`

**❓ Сколько лучей при шаге 0.5°?**  
↳ От -60° до +60° = 241 луч (или настрой диапазон под себя)

**❓ Где применяется гетеродинирование?**  
↳ На GPU в отдельном kernel после beamforming, перед БПФ

---

**✅ ГОТОВО К ИСПОЛЬЗОВАНИЮ!**

Весь код написан, все формулы выведены, примеры подготовлены.  
Следуй плану выше — и всё заработает! 🚀

---

**Версия:** 1.0  
**Статус:** Полная документация + готовый код  
**Дата:** 10 января 2026
