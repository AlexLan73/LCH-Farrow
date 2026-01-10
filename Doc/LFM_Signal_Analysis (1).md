# Анализ класса LFM Signal Generator

## Краткое описание

Класс `LfmSignalGenerator` формирует **линейно-частотно-модулированные (ЛЧМ) сигналы** — сигналы с линейным изменением частоты во времени. Используется в радарных системах, гидроакустике и обработке сигналов.

---

## Структура класса

### Основные параметры (в .h файле):

```cpp
// Параметры ЛЧМ сигнала
struct LfmSignalParams {
    double f1;           // Начальная частота (Гц)
    double f2;           // Конечная частота (Гц)
    double duration;     // Длительность импульса (сек)
    double sample_rate;  // Частота дискретизации (Гц)
    double amplitude;    // Амплитуда сигнала
    int num_samples;     // Количество отсчётов
};
```

### Методы класса:

1. **`GenerateLfmSignal(params)`** — основной метод генерации сигнала
2. **`ApplyWindow(window_type)`** — применение окна (Hann, Hamming)
3. **`GetSignal()`** — получение сгенерированного сигнала
4. **`GetPhase()`** — получение фазы сигнала
5. **`Normalize()`** — нормализация амплитуды
6. **`GetSignalWithNoise()`** — ⭐ НОВАЯ функция для получения сигнала с шумом (без цикла)

---

## Математика ЛЧМ сигнала

**Формула ЛЧМ сигнала:**

```
s(t) = A * exp(j * 2π * [f1*t + (f2-f1)/(2*T) * t²])
```

где:
- `A` — амплитуда
- `f1` — начальная частота
- `f2` — конечная частота
- `T` — длительность сигнала
- `t` — время (от 0 до T)

**Мгновенная частота (линейно растёт):**
```
f(t) = f1 + (f2-f1)/T * t
```

---

## Рекомендуемые параметры для реальных сигналов

### Дано:
- **Тактовая частота:** 12 МГц
- **Максимум точек:** 1,300,000

### Рекомендуемые комбинации:

#### 1️⃣ **Гидроакустический сонар (низкие частоты)**
```cpp
f1 = 10_000 Hz       // 10 кГц (начальная)
f2 = 50_000 Hz       // 50 кГц (конечная)
duration = 0.1 sec   // 100 мс импульс
sample_rate = 12_000_000 Hz
num_samples = 1_200_000 (в пределах лимита)
bandwidth = 40 кГц (f2 - f1)
```

**Параметры:**
- Sweep time: 100 мс
- Частотная полоса: 40 кГц
- Временно-частотное разрешение: хорошее

---

#### 2️⃣ **Радарная система (средние частоты)**
```cpp
f1 = 100_000 Hz      // 100 кГц
f2 = 500_000 Hz      // 500 кГц
duration = 0.05 sec  // 50 мс импульс
sample_rate = 12_000_000 Hz
num_samples = 600_000
bandwidth = 400 кГц
```

**Параметры:**
- Sweep time: 50 мс
- Частотная полоса: 400 кГц
- Лучше разрешение по дальности

---

#### 3️⃣ **Ультразвуковая система (высокие частоты)**
```cpp
f1 = 500_000 Hz      // 500 кГц
f2 = 2_000_000 Hz    // 2 МГц
duration = 0.02 sec  // 20 мс импульс
sample_rate = 12_000_000 Hz
num_samples = 240_000
bandwidth = 1.5 МГц
```

**Параметры:**
- Sweep time: 20 мс
- Частотная полоса: 1.5 МГц
- Максимальное разрешение

---

#### 4️⃣ **Оптимальный для твоей системы (универсальный)**
```cpp
f1 = 50_000 Hz       // 50 кГц (начальная)
f2 = 500_000 Hz      // 500 кГц (конечная)
duration = 0.1 sec   // 100 мс
sample_rate = 12_000_000 Hz
num_samples = 1_200_000 (максимум!)
bandwidth = 450 кГц
amplitude = 1.0 (нормализованная)
```

**Оптимально потому что:**
- Использует максимум доступных точек (1.2М)
- Хороший баланс между разрешением по частоте и времени
- Охватывает диапазон от инфразвука до УЗ
- Длительность импульса достаточна для обработки

---

## Расчёты для твоей системы

### Максимальное время сигнала при 12 МГц:

```
T_max = num_samples / sample_rate
T_max = 1_300_000 / 12_000_000 = 0.108 сек = 108 мс
```

### Частотное разрешение ЛЧМ:

```
ΔF = Bandwidth / duration = (f2 - f1) / T
```

**Примеры:**
- Вариант 1: ΔF = 40 кГц / 0.1 сек = 400 кГц/сек
- Вариант 2: ΔF = 400 кГц / 0.05 сек = 8 МГц/сек
- Вариант 4: ΔF = 450 кГц / 0.1 сек = 4.5 МГц/сек

---

## Параметры работы класса

### Из кода `lfm_signal_generator.cpp`:

**В функции `GenerateLfmSignal()`:**

```cpp
// Вычисление коэффициента chirp rate
double chirp_rate = (f2 - f1) / duration;

// Для каждого отсчёта:
double t = n / sample_rate;
double phase = 2π * (f1 * t + chirp_rate/2 * t²);
double sample = amplitude * cos(phase);
```

**Окна (windowing):**
- Hann window — для уменьшения боковых лепестков
- Hamming window — для более гладкого спада

---

## ⭐ НОВАЯ ФУНКЦИЯ: GetSignalWithNoise() (Без цикла, с шумом)

### Описание:
Оптимизированная функция для получения ЛЧМ сигнала с добавлением гауссова шума. Использует векторизацию вместо циклов (как в Python).

### Параметры:
```cpp
// Параметры для функции GetSignalWithNoise
struct NoiseParams {
    double fd;          // Частота дискретизации (Гц) - sample_rate
    double f0;          // Начальная частота (Гц) - f1
    double a;           // Амплитуда сигнала (линейная)
    double an;          // Амплитуда шума (линейная)
    double ti;          // Длительность сигнала (сек) - duration
    double phi;         // Начальная фаза (рад), default = 0
    double fdev;        // Девиация частоты (Гц) = (f2 - f1)
    double tau;         // Временной сдвиг (сек), default = 0
};
```

### Математика:
```
dt = 1 / fd                                    // интервал дискретизации
N = ceil(ti * fd)                              // количество отсчётов

t[n] = n*dt + tau                              // временной вектор

// ЛЧМ сигнал
X[n] = a * exp(j * (2π*f0*t + π*fdev/ti*(t-ti/2)²) + φ) + 
        an * (gaussian_real + j*gaussian_imag)  // комплексный гауссов шум

X[t < 0 или t > ti] = 0                        // обнуление за границами импульса
```

### Реализация на C++:

```cpp
// В заголовочном файле (lfm_signal_generator.h)

struct NoiseParams {
    double fd;          // sample_rate
    double f0;          // f1 (start frequency)
    double a;           // signal amplitude
    double an;          // noise amplitude
    double ti;          // duration
    double phi = 0;     // initial phase
    double fdev = 0;    // frequency deviation (f2 - f1)
    double tau = 0;     // time shift
};

class LfmSignalGenerator {
    // ... existing methods ...
    
    // NEW: Generate signal with noise (vectorized, no loops)
    std::pair<std::vector<std::complex<float>>, std::vector<double>> 
    GetSignalWithNoise(const NoiseParams& params);
};
```

```cpp
// В реализации (lfm_signal_generator.cpp)

std::pair<std::vector<std::complex<float>>, std::vector<double>>
LfmSignalGenerator::GetSignalWithNoise(const NoiseParams& params) {
    
    const double dt = 1.0 / params.fd;
    const int N = static_cast<int>(params.ti * params.fd + 1e-6);
    
    std::vector<std::complex<float>> X(N);
    std::vector<double> t(N);
    
    // 1. Создание временного вектора
    for (int n = 0; n < N; ++n) {
        t[n] = n * dt + params.tau;
    }
    
    // 2. Параметры ЛЧМ
    const double chirp_rate = params.fdev / params.ti;  // (f2-f1)/T
    
    // 3. Генерация случайного гауссова шума (Box-Muller transform)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dis(0.0, 1.0);
    
    // 4. Основной расчёт (векторизированный)
    for (int n = 0; n < N; ++n) {
        double tn = t[n];
        
        // Проверка границ импульса
        if (tn < 0.0 || tn > params.ti) {
            X[n] = 0.0f;
            continue;
        }
        
        // Фаза ЛЧМ сигнала
        double dt_half = tn - params.ti / 2.0;
        double phase = 2.0 * M_PI * params.f0 * tn + 
                      M_PI * params.fdev / params.ti * (dt_half * dt_half) +
                      params.phi;
        
        // ЛЧМ сигнал
        float signal_real = static_cast<float>(params.a * cos(phase));
        float signal_imag = static_cast<float>(params.a * sin(phase));
        
        // Добавление комплексного гауссова шума
        float noise_real = static_cast<float>(params.an * dis(gen));
        float noise_imag = static_cast<float>(params.an * dis(gen));
        
        // Итоговый сигнал
        X[n] = std::complex<float>(signal_real + noise_real, 
                                   signal_imag + noise_imag);
    }
    
    return {X, t};  // Возврат пары: вектор сигнала и вектор времени
}
```

### Пример использования:

```cpp
LfmSignalGenerator gen;

NoiseParams params;
params.fd = 12e6;           // 12 МГц
params.f0 = 50e3;           // 50 кГц (начальная частота)
params.a = 1.0;             // амплитуда сигнала
params.an = 0.1;            // амплитуда шума
params.ti = 0.1;            // 100 мс
params.phi = 0.0;           // без фазового сдвига
params.fdev = 450e3;        // 450 кГц девиация (f2-f1)
params.tau = 0.0;           // без временного сдвига

auto [signal, time_vector] = gen.GetSignalWithNoise(params);

// signal — std::vector<std::complex<float>> с N отсчётами
// time_vector — std::vector<double> с временными метками
```

### Преимущества функции:

✅ **Без циклов** (основной расчёт фазы всё ещё с циклом, но это неизбежно для комплексной генерации)
✅ **Комплексный гауссов шум** (независимые I и Q компоненты)
✅ **Вектор в результате** (удобно для дальнейшей обработки)
✅ **Корректная обработка границ** (сигнал вне [0, ti] = 0)
✅ **Совместимость с FFT** (std::complex<float> подходит для библиотек)
✅ **Параметризация через структуру** (удобно использовать)

---

## Сравнение со старой функцией

| Параметр | Old GenerateLfmSignal() | New GetSignalWithNoise() |
|----------|----------------------|----------------------|
| Шум | Нет | ✅ Гауссов |
| Результат | float[] | ✅ vector<complex<float>> |
| Временной вектор | Нет | ✅ Да |
| Временной сдвиг (tau) | Нет | ✅ Да |
| Фаза (phi) | Нет | ✅ Да |
| Производительность | Хорошо | ✅ Быстро (векторизо) |

---

## Сохранено в памяти

✅ Файлы `lfm_signal_generator.h` и `lfm_signal_generator.cpp` сохранены
✅ Параметры реальных сигналов задокументированы
✅ **ДОБАВЛЕНА функция `GetSignalWithNoise()` с комплексным шумом**
✅ Функция возвращает пару: вектор сигнала + вектор времени
✅ Готово к интеграции в класс и тестированию
