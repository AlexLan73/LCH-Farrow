# 📊 SUMMARY TABLE - ВСЕ ПАРАМЕТРЫ И ФОРМУЛЫ

## ПАРАМЕТРЫ AngleArrayParams

| Параметр | Тип | Значение по умолчанию | Диапазон/Описание |
|----------|-----|----------------------|-------------------|
| `f_start` | float | 1.0e6 | > 0, < f_stop (Гц) |
| `f_stop` | float | 2.0e6 | > f_start (Гц) |
| `sample_rate` | float | 12.0e6 | > 2*f_stop (Гц) |
| `num_samples` | size_t | 512 | 2^n (256, 512, 1024...) |
| `angle_start_deg` | float | -15.0 | -90..+90 (градусы) |
| `angle_stop_deg` | float | +15.0 | > angle_start_deg |
| `angle_step_deg` | float | 0.5 | > 0 (0.125, 0.25, 0.5, 1.0...) |
| `antenna_element_idx` | size_t | 5 | 0..255 (индекс элемента) |
| `antenna_element_spacing_m` | float | 100.0 | обычно λ/2 (метры) |
| `lagrange_order` | size_t | 48 | 4..128 (четное число) |
| `lagrange_row` | size_t | 5 | < lagrange_order |

---

## ВЫЧИСЛЯЕМЫЕ ПАРАМЕТРЫ

| Параметр | Формула | Пример |
|----------|---------|--------|
| `duration` | `num_samples / sample_rate` | 512 / 12e6 = 42.67 µs |
| `chirp_rate` | `(f_stop - f_start) / duration` | 1e6 / 42.67e-6 = 23.4 GHz/s |
| `f_center` | `(f_start + f_stop) / 2` | 1.5 МГц |
| `wavelength` | `SPEED_OF_LIGHT / f_center` | 3e8 / 1.5e6 = 200 м |
| `num_angles` | `(angle_stop - angle_start) / angle_step + 1` | 31 / 0.5 + 1 = 61 |

---

## ОСНОВНЫЕ ФОРМУЛЫ

### 1. Задержка (отсчёты)
```
delay_samples = (k · d · sin(θ) · f_s) / c

k = antenna_element_idx
d = antenna_element_spacing_m (λ/2)
θ = угол (радианы)
f_s = sample_rate
c = 3×10⁸ м/с
```

### 2. ЛЧМ фаза
```
φ[n] = 2π(f_start·t[n] + 0.5·K·t[n]²)

K = (f_stop - f_start) / duration
t[n] = n / sample_rate
```

### 3. ЛЧМ сигнал
```
s[n] = cos(φ[n]) + j·sin(φ[n])
     = exp(j·φ[n])
```

### 4. Сопряжение (для гетеродина)
```
s*[n] = cos(φ[n]) - j·sin(φ[n])
      = conj(s[n])
```

### 5. Гетеродинирование
```
y[n] = x[n] × s*[n]
```

### 6. Лагранжева интерполяция (порядок P)
```
s_interp = Σ(i=0 to P) s[i] · L_i(d_frac)

где:
L_i(x) = Π(j=0, j≠i, to P) (x - j) / (i - j)
d_frac = дробная часть delay_samples
```

---

## JSON СТРУКТУРА

### Метаданные
```json
{
  "metadata": {
    "num_angles": integer,
    "num_samples": integer,
    "angle_start_deg": float,
    "angle_stop_deg": float,
    "angle_step_deg": float,
    "f_start_hz": float,
    "f_stop_hz": float,
    "sample_rate_hz": float,
    "antenna_element_idx": integer,
    "antenna_element_spacing_m": float,
    "lagrange_order": integer,
    "lagrange_row": integer
  }
}
```

### Сигналы
```json
{
  "reference_signals": [
    {
      "angle_deg": float,
      "num_samples": integer,
      "data": {
        "real": [float, float, ...],
        "imag": [float, float, ...]
      }
    }
  ]
}
```

---

## ПРИМЕРЫ КОНФИГУРАЦИЙ

### A. Стандарт
```cpp
angle_start_deg = -15.0f;
angle_stop_deg = 15.0f;
angle_step_deg = 0.5f;
num_samples = 512;
// → 61 луч, 512 отсчётов
```

### B. Высокое разрешение
```cpp
angle_start_deg = 0.0f;
angle_stop_deg = 15.0f;
angle_step_deg = 0.125f;
num_samples = 1024;
// → 121 луч, 1024 отсчёта
```

### C. Быстрый тест
```cpp
angle_start_deg = -5.0f;
angle_stop_deg = 5.0f;
angle_step_deg = 1.0f;
num_samples = 256;
// → 11 лучей, 256 отсчётов
```

---

## ПРОВЕРОЧНЫЕ ВЫЧИСЛЕНИЯ

### Пример: θ=30°, элемент 5

```
f_center = 1.5 МГц
λ = c / f_center = 3×10⁸ / 1.5×10⁶ = 200 м
d = λ/2 = 100 м

sin(30°) = 0.5

τ_5(30°) = (5 × 100 × 0.5 × 12×10⁶) / (3×10⁸)
         = (300×10⁶) / (3×10⁸)
         = 1.0 sample
```

---

## ОГРАНИЧЕНИЯ

| Параметр | Min | Max | Причина |
|----------|-----|-----|---------|
| f_start | 1 Гц | f_stop - 1 | Физика |
| f_stop | f_start + 1 | 6 МГц | Найквист с 12 МГц |
| num_samples | 256 | 1 млн | Память/производительность |
| angle_step_deg | 0.01° | 90° | Точность/данные |
| lagrange_order | 4 | 128 | Стабильность |

---

## РАЗМЕР ПАМЯТИ

```
JSON размер ≈ num_angles × num_samples × 24 байт
              (2 float × 8 байт real/imag + overhead)

Пример: 61 × 512 × 24 ≈ 750 КБ
```

---

## ✅ ГОТОВО
