# 🚀 QUICK START

## ФАЙЛЫ

1. `lfm_angle_array_final.h` — из SOLUTION_FINAL.md
2. `lfm_angle_array_final.cpp` — из SOLUTION_FINAL.md
3. `main.cpp` — пример ниже

---

## ПРИМЕР main.cpp

```cpp
#include <fstream>
#include <iostream>
#include "lfm_angle_array_final.h"

int main() {
    using namespace radar;

    AngleArrayParams params;
    params.f_start = 1.0e6f;
    params.f_stop = 2.0e6f;
    params.sample_rate = 12.0e6f;
    params.num_samples = 512;  // 2^9

    // Углы
    params.angle_start_deg = -15.0f;
    params.angle_stop_deg = 15.0f;
    params.angle_step_deg = 0.5f;

    // Антенна
    params.antenna_element_idx = 5;
    float f_center = (params.f_start + params.f_stop) / 2.0f;
    float wavelength = 3.0e8f / f_center;
    params.antenna_element_spacing_m = wavelength / 2.0f;

    // Лагранжа
    params.lagrange_order = 48;
    params.lagrange_row = 5;

    try {
        LFMAngleArray angle_array(params);
        angle_array.Generate();

        std::cout << "✅ Angles: " << angle_array.GetNumAngles() << "\n";
        std::cout << "✅ Samples: " << angle_array.GetNumSamples() << "\n";

        std::string json = angle_array.ExportToJSON();
        std::ofstream out("reference_signals.json");
        out << json;
        out.close();

        std::cout << "💾 JSON saved\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

---

## КОМПИЛЯЦИЯ

```bash
g++ -std=c++17 -O2 -o test main.cpp lfm_angle_array_final.cpp
./test
# → reference_signals.json
```

---

## ПРИМЕРЫ КОНФИГОВ

### A. ±10° шаг 0.5°
```cpp
params.angle_start_deg = -10.0f;
params.angle_stop_deg = 10.0f;
params.angle_step_deg = 0.5f;
// → 41 луч
```

### B. 0..15° шаг 0.125°
```cpp
params.angle_start_deg = 0.0f;
params.angle_stop_deg = 15.0f;
params.angle_step_deg = 0.125f;
// → 121 луч
```

### C. Быстрый тест -5..+5° шаг 1°
```cpp
params.angle_start_deg = -5.0f;
params.angle_stop_deg = 5.0f;
params.angle_step_deg = 1.0f;
// → 11 лучей
```

---

## JSON СТРУКТУРА

```json
{
  "metadata": {
    "num_angles": 61,
    "num_samples": 512,
    "angle_start_deg": -15.0,
    "angle_step_deg": 0.5
  },
  "reference_signals": [
    {
      "angle_deg": -15.0,
      "data": {
        "real": [...],
        "imag": [...]
      }
    }
  ]
}
```

---

## ПРОВЕРКА PYTHON

```python
import json
with open("reference_signals.json") as f:
    data = json.load(f)
meta = data["metadata"]
signals = data["reference_signals"]
print(f"OK: {len(signals)} signals")
```

---

## ✅ ГОТОВО!
