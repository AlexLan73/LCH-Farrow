# 💻 Готовый Код: Расширение LFMSignalGenerator

## КОД 1: Добавить в lfm_signal_generator.h

```cpp
// ═══════════════════════════════════════════════════════════════════════════
// В enum class LFMVariant добавить новый тип:
// ═══════════════════════════════════════════════════════════════════════════

enum class LFMVariant : uint8_t {
    BASIC = 0,           // Базовый ЛЧМ для всех лучей одинаково
    PHASE_OFFSET = 1,    // С фазовыми сдвигами (array steering)
    DELAY = 2,           // С временными задержками
    BEAMFORMING = 3,     // С фазовым фокусированием
    WINDOWED = 4,        // С Hamming окном
    ANGLE_SWEEP = 5,     // 🆕 По углам с шагом 0.5° (НОВОЕ!)
    HETERODYNE = 6       // 🆕 Для гетеродина (сопряжённый сигнал)
};

// ═══════════════════════════════════════════════════════════════════════════
// В struct LFMParameters добавить поля:
// ═══════════════════════════════════════════════════════════════════════════

struct LFMParameters {
    float f_start = 100.0f;           // Начальная частота (Гц)
    float f_stop = 500.0f;            // Конечная частота (Гц)
    float sample_rate = 12.0e6f;      // Частота дискретизации (12 МГц)
    float duration = 1.0f;            // Длительность сигнала (сек)
    size_t num_beams = 256;           // Количество лучей
    float steering_angle = 30.0f;     // Базовый угол (градусы)
    
    // 🆕 НОВЫЕ ПОЛЯ для задержки с шагом угла:
    float angle_step_deg = 0.5f;      // Шаг по углу (градусы) - СТАНДАРТ 0.5°
    float angle_start_deg = -60.0f;   // Начальный угол (градусы)
    float angle_stop_deg = 60.0f;     // Конечный угол (градусы)
    
    // ДЛЯ ГЕТЕРОДИНА:
    bool apply_heterodyne = false;    // Применять ли сопряжение
    
    // ВАЛИДАЦИЯ (обновлена)
    bool IsValid() const noexcept {
        return f_start > 0.0f && f_stop > f_start &&
               sample_rate > 2.0f * f_stop &&
               duration > 0.0f && num_beams > 0 &&
               angle_step_deg > 0.0f;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// В класс LFMSignalGenerator добавить эти методы (публичные):
// ═══════════════════════════════════════════════════════════════════════════

class LFMSignalGenerator {
    // ... существующие методы ...
    
public:
    
    // 🆕 НОВЫЙ МЕТОД 1: Генерация с задержкой по углам (0.5° шаг)
    // Входные параметры:
    //   - angle_deg: центральный угол в градусах
    //   - num_angles: количество лучей (каждый на 0.5° от предыдущего)
    //   - element_index: индекс элемента антенной решётки
    // Возвращает: задержку в отсчётах для этого элемента и угла
    float ComputeDelayForAngle(
        float angle_deg,        // Угол в градусах
        size_t element_index    // Индекс элемента (0, 1, 2, ...)
    ) const noexcept;
    
    // 🆕 НОВЫЙ МЕТОД 2: Создать сопряжённую копию буфера (гетеродин)
    SignalBufferNew MakeConjugateCopy(const SignalBufferNew& src) const;
    
    // 🆕 НОВЫЙ МЕТОД 3: In-place сопряжение (экономит память)
    void ConjugateInPlace(SignalBufferNew& buffer) const noexcept;
    
    // 🆕 НОВЫЙ МЕТОД 4: Гетеродинирование (умножение двух сигналов)
    // Результат: y[n] = x[n] * h[n], где h[n] = сопряжённый опорный сигнал
    SignalBufferNew Heterodyne(
        const SignalBufferNew& rx_signal,      // Принятый сигнал
        const SignalBufferNew& ref_signal      // Опорный сигнал (ЛЧМ)
    ) const;

private:
    
    // 🆕 ПРИВАТНЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ:
    
    // Генерация варианта с задержкой по углам
    void GenerateVariant_AngleSweep(
        std::complex<float>* beam_data,
        size_t num_samples,
        float angle_deg,
        size_t element_index
    ) const noexcept;
    
    // Генерация варианта гетеродина (сопряжённый сигнал)
    void GenerateVariant_Heterodyne(
        std::complex<float>* beam_data,
        size_t num_samples
    ) const noexcept;
};
```

---

## КОД 2: Добавить в lfm_signal_generator.cpp

```cpp
// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 1: Вычисление задержки для угла (формула 5 из теории)
// ═══════════════════════════════════════════════════════════════════════════

float LFMSignalGenerator::ComputeDelayForAngle(
    float angle_deg,
    size_t element_index
) const noexcept {
    
    // Константы
    const float angle_rad = angle_deg * PI / 180.0f;  // Перевод в радианы
    const float sin_angle = std::sin(angle_rad);
    
    // Вычисляем длину волны (для центральной частоты)
    float f_center = (params_.f_start + params_.f_stop) / 2.0f;
    float wavelength = SPEED_OF_LIGHT / f_center;
    
    // Расстояние между элементами (стандартно λ/2)
    float element_spacing = wavelength / 2.0f;
    
    // Геометрическая задержка по времени (формула 4)
    float element_position = static_cast<float>(element_index) * element_spacing;
    float delay_time = (element_position * sin_angle) / SPEED_OF_LIGHT;
    
    // Переводим в отсчёты (формула 5)
    float delay_samples = delay_time * params_.sample_rate;
    
    return delay_samples;
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 2: Создание сопряжённой копии
// ═══════════════════════════════════════════════════════════════════════════

SignalBufferNew LFMSignalGenerator::MakeConjugateCopy(
    const SignalBufferNew& src
) const {
    
    SignalBufferNew dst(src.GetNumBeams(), src.GetNumSamples());
    
    const std::complex<float>* src_data = src.RawData();
    std::complex<float>* dst_data = dst.RawData();
    
    size_t total_size = src.GetTotalSize();
    
    // Стандартная операция сопряжения
    for (size_t i = 0; i < total_size; ++i) {
        dst_data[i] = std::conj(src_data[i]);  // Встроенная функция C++
    }
    
    return dst;
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 3: Сопряжение на месте (экономит память)
// ═══════════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::ConjugateInPlace(SignalBufferNew& buffer) const noexcept {
    
    std::complex<float>* data = buffer.RawData();
    size_t total_size = buffer.GetTotalSize();
    
    for (size_t i = 0; i < total_size; ++i) {
        data[i] = std::conj(data[i]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 4: Гетеродинирование (комплексное умножение)
// ═══════════════════════════════════════════════════════════════════════════

SignalBufferNew LFMSignalGenerator::Heterodyne(
    const SignalBufferNew& rx_signal,
    const SignalBufferNew& ref_signal
) const {
    
    // Проверка размерности
    if (rx_signal.GetTotalSize() != ref_signal.GetTotalSize()) {
        throw std::invalid_argument(
            "Signals must have same size for heterodyning"
        );
    }
    
    // Создаём буфер результата
    SignalBufferNew result(rx_signal.GetNumBeams(), rx_signal.GetNumSamples());
    
    const std::complex<float>* rx_data = rx_signal.RawData();
    const std::complex<float>* ref_data = ref_signal.RawData();
    std::complex<float>* out_data = result.RawData();
    
    size_t total_size = rx_signal.GetTotalSize();
    
    // Перемножение с сопряжением на лету (без создания второго буфера)
    for (size_t i = 0; i < total_size; ++i) {
        // y[i] = rx[i] * conj(ref[i])
        out_data[i] = rx_data[i] * std::conj(ref_data[i]);
    }
    
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// ПРИВАТНЫЙ МЕТОД: Генерация варианта с задержкой по углам
// ═══════════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::GenerateVariant_AngleSweep(
    std::complex<float>* beam_data,
    size_t num_samples,
    float angle_deg,
    size_t element_index
) const noexcept {
    
    // Вычисляем задержку для этого элемента и угла
    float delay_samples = ComputeDelayForAngle(angle_deg, element_index);
    
    // Используем уже существующий метод для применения задержки
    GenerateVariant_Delay(beam_data, num_samples, delay_samples);
}

// ═══════════════════════════════════════════════════════════════════════════
// ПРИВАТНЫЙ МЕТОД: Генерация варианта гетеродина (сопряжённый сигнал)
// ═══════════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::GenerateVariant_Heterodyne(
    std::complex<float>* beam_data,
    size_t num_samples
) const noexcept {
    
    // Генерируем обычный ЛЧМ
    GenerateVariant_Basic(beam_data, num_samples);
    
    // Применяем сопряжение (меняем знак мнимой части)
    for (size_t i = 0; i < num_samples; ++i) {
        beam_data[i] = std::conj(beam_data[i]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ОБНОВЛЕНИЕ: Метод GenerateIntoBuffer (добавить эти ветки в switch)
// ═══════════════════════════════════════════════════════════════════════════

// В функции GenerateIntoBuffer, в switch(variant):

        case LFMVariant::ANGLE_SWEEP: {
            // Вычисляем угол для этого луча (шаг 0.5°)
            float angle_deg = params_.angle_start_deg + 
                            static_cast<float>(beam) * params_.angle_step_deg;
            
            // Генерируем сигнал с задержкой для этого угла
            GenerateVariant_AngleSweep(beam_data, num_samples, angle_deg, beam);
            break;
        }
        
        case LFMVariant::HETERODYNE: {
            // Генерируем сопряжённый сигнал (для гетеродина)
            GenerateVariant_Heterodyne(beam_data, num_samples);
            break;
        }
```

---

## КОД 3: ПРИМЕР ИСПОЛЬЗОВАНИЯ

### Вариант A: Базовая генерация ЛЧМ

```cpp
// Создаём параметры
radar::LFMParameters lfm_params;
lfm_params.f_start = 1.0e6f;         // 1 МГц
lfm_params.f_stop = 2.0e6f;          // 2 МГц
lfm_params.sample_rate = 12.0e6f;    // 12 МГц ✓
lfm_params.duration = 0.001f;        // 1 мс
lfm_params.num_beams = 256;          // 256 направлений (256 × 0.5° = 128°)

// Создаём генератор
radar::LFMSignalGenerator lfm_generator(lfm_params);

// Генерируем базовый ЛЧМ
auto signal = lfm_generator.Generate(radar::LFMVariant::BASIC);
```

### Вариант B: Генерация с задержкой по углам (0.5° шаг)

```cpp
// Параметры (добавляем углы)
radar::LFMParameters lfm_params;
lfm_params.f_start = 1.0e6f;
lfm_params.f_stop = 2.0e6f;
lfm_params.sample_rate = 12.0e6f;
lfm_params.duration = 0.001f;
lfm_params.num_beams = 256;          // 256 направлений
lfm_params.angle_start_deg = -60.0f; // Начало сканирования
lfm_params.angle_stop_deg = 60.0f;   // Конец сканирования
lfm_params.angle_step_deg = 0.5f;    // Шаг 0.5° ✓

radar::LFMSignalGenerator lfm_generator(lfm_params);

// Генерируем сигналы для всех углов (каждый луч = один угол)
auto angles_signal = lfm_generator.Generate(radar::LFMVariant::ANGLE_SWEEP);

// Результат:
// angles_signal.GetBeamData(0)   → сигнал для θ = -60.0°
// angles_signal.GetBeamData(1)   → сигнал для θ = -59.5°
// angles_signal.GetBeamData(2)   → сигнал для θ = -59.0°
// ...
// angles_signal.GetBeamData(240) → сигнал для θ = +60.0°
```

### Вариант C: Гетеродинирование

```cpp
// Генерируем два сигнала: приёмный и опорный
auto rx_signal = lfm_generator.Generate(radar::LFMVariant::BASIC);
auto ref_signal = lfm_generator.Generate(radar::LFMVariant::BASIC);

// Способ 1: Явное сопряжение + умножение
auto ref_conj = lfm_generator.MakeConjugateCopy(ref_signal);
auto heterodyned = lfm_generator.Heterodyne(rx_signal, ref_signal);

// Способ 2: Сопряжение на месте (экономнее)
lfm_generator.ConjugateInPlace(ref_signal);
auto heterodyned = lfm_generator.Heterodyne(rx_signal, ref_signal);

// Способ 3: Генерируем сопряжённый сигнал сразу
auto ref_conjugate = lfm_generator.Generate(radar::LFMVariant::HETERODYNE);
```

### Вариант D: Полный цикл (передача + приём + обработка)

```cpp
// 1. Генерируем передающий ЛЧМ (базовый)
auto tx_signal = lfm_generator.Generate(radar::LFMVariant::BASIC);

// 2. Имитируем приём (в реальности это будет с GPU)
// Здесь просто копируем, но на практике это будет приёмный тракт
auto rx_signal = lfm_generator.MakeConjugateCopy(tx_signal);

// 3. Создаём сопряжённый опорный сигнал (гетеродин)
auto tx_conjugate = lfm_generator.MakeConjugateCopy(tx_signal);

// 4. Гетеродинируем
auto baseband_signal = lfm_generator.Heterodyne(rx_signal, tx_signal);

// 5. Результат в baseband готов для дальнейшей обработки
// (БПФ, корреляция, обнаружение целей и т.д.)
```

---

## КОД 4: РАСЧЁТ ЗАДЕРЖКИ ВРУЧНУЮ (Для Проверки)

```cpp
// ПРИМЕР: вычисляем задержку для конкретного угла

float f_center = 1.5e6f;              // Центральная частота (1.5 МГц)
float c = 3.0e8f;                     // Скорость света (м/с)
float wavelength = c / f_center;      // λ = 200 м (!)
float element_spacing = wavelength / 2.0f;  // d = 100 м (!)

// ⚠️ ВНИМАНИЕ: Длины волны в 200 м — это очень большие антенные решётки!
// Это проверка математики. На практике используй f в ГГц или имитируй на GPU.

// Угол: 30°
float angle_deg = 30.0f;
float angle_rad = 30.0f * M_PI / 180.0f;  // 0.524 рад
float sin_angle = std::sin(angle_rad);    // 0.5

// Элемент 5 решётки
int element_index = 5;

// Геометрическая задержка
float element_pos = element_index * element_spacing;           // 5 × 100 = 500 м
float delay_time = (element_pos * sin_angle) / c;             // (500 × 0.5) / 3e8 = 833 нс
float delay_samples_f = delay_time * 12.0e6f;                 // 833 нс × 12 МГц = 10 отсчётов

int delay_samples_int = static_cast<int>(delay_samples_f);    // 10 отсчётов

std::cout << "Угол: " << angle_deg << "°\n";
std::cout << "Элемент: " << element_index << "\n";
std::cout << "Задержка (сек): " << delay_time << "\n";
std::cout << "Задержка (отсчёты): " << delay_samples_int << "\n";
```

---

## ИНТЕГРАЦИЯ С ТВОИМ GPU KERNEL

### На GPU (OpenCL) в твоем kernel:

```c
// В твоем kernel используй вычисленную задержку:

float delay_samples = ComputeDelayForAngle(angle_deg, element_idx);

// Раздели на целую и дробную части
int delay_int = (int)delay_samples;
float delay_frac = delay_samples - delay_int;

// Применяй Лагранжа интерполяцию (как ты уже делаешь!)
// для дробной части delay_frac

// Результат: точная временная задержка с интерполяцией ✓
```

---

**Версия:** 1.0  
**Статус:** Полный готовый код  
**Дата:** 10 января 2026
