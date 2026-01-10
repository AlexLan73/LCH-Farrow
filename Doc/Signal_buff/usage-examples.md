# 📚 Примеры использования обновлённого SignalBuffer

## 📚 Содержание

1. [Базовое использование](#базовое-использование)
2. [Работа с дробными задержками](#работа-с-дробными-задержками)
3. [Доступ к данным](#доступ-к-данным)
4. [Интеграция с GPU](#интеграция-с-gpu)
5. [Советы оптимизации](#советы-оптимизации)

---

## Базовое использование

### Пример 1: Создание и инициализация буфера

```cpp
#include "signal_buffer.h"
#include "lfm_signal_generator.h"

int main() {
    // Параметры
    const size_t num_beams = 256;      // Количество лучей (антенн)
    const size_t num_samples = 8000;   // Отсчётов на луч (частота дискретизации × время)
    
    // 1. Создаём буфер
    SignalBuffer buffer(num_beams, num_samples);
    
    // 2. Проверяем размер
    std::cout << "Лучей: " << buffer.GetNumBeams() << "\n";          // 256
    std::cout << "Отсчётов: " << buffer.GetNumSamples() << "\n";    // 8000
    std::cout << "Всего элементов: " << buffer.GetRawData().size() << "\n";  // 2,048,000
    
    return 0;
}
```

### Пример 2: Генерация ЛЧМ сигнала

```cpp
#include "signal_buffer.h"
#include "lfm_signal_generator.h"

int main() {
    // Параметры ЛЧМ (chirp)
    const float f_start = 100.0f;      // Начальная частота, Гц
    const float f_stop = 500.0f;       // Конечная частота, Гц
    const float sample_rate = 8000.0f; // Частота дискретизации
    const float duration = 1.0f;       // Длительность сигнала, сек
    
    // Создаём генератор
    LFMSignalGenerator lfm(f_start, f_stop, sample_rate, duration);
    
    // Создаём буфер для данных
    const size_t num_beams = 4;
    const size_t num_samples = static_cast<size_t>(duration * sample_rate);
    SignalBuffer buffer(num_beams, num_samples);
    
    // Получаем указатели на данные каждого луча
    std::vector<std::complex<float>*> beam_ptrs(num_beams);
    for (size_t i = 0; i < num_beams; ++i) {
        beam_ptrs[i] = buffer.GetBeamData(i);
    }
    
    // Без задержек (все лучи содержат одинаковый сигнал)
    lfm.GenerateAllBeams(beam_ptrs, num_samples, num_beams);
    
    std::cout << "✅ Сигнал сгенерирован\n";
    return 0;
}
```

---

## Работа с дробными задержками

### Пример 3: Дробные задержки (Fractional Delay)

```cpp
#include "signal_buffer.h"
#include "lfm_signal_generator.h"
#include <vector>

int main() {
    // Генератор ЛЧМ
    LFMSignalGenerator lfm(100.0f, 500.0f, 8000.0f, 1.0f);
    
    // Буфер
    const size_t num_beams = 8;
    const size_t num_samples = 8000;
    SignalBuffer buffer(num_beams, num_samples);
    
    // Указатели на лучи
    std::vector<std::complex<float>*> beam_ptrs(num_beams);
    for (size_t i = 0; i < num_beams; ++i) {
        beam_ptrs[i] = buffer.GetBeamData(i);
    }
    
    // ✨ ДРОБНЫЕ ЗАДЕРЖКИ
    std::vector<float> delays(num_beams);
    for (size_t i = 0; i < num_beams; ++i) {
        // Задержки: 0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875
        delays[i] = i * 0.125f;
    }
    
    // Генерируем лучи с дробными задержками
    lfm.GenerateAllBeams(beam_ptrs, num_samples, num_beams, delays);
    
    // Выводим информацию
    std::cout << "✅ Сгенерировано лучей с дробными задержками:\n";
    for (size_t i = 0; i < num_beams; ++i) {
        std::cout << "  Луч " << i << ": задержка = " << delays[i] << " отсчётов\n";
    }
    
    return 0;
}
```

### Пример 4: Симуляция DOA (Direction of Arrival)

```cpp
#include "signal_buffer.h"
#include "lfm_signal_generator.h"
#include <cmath>
#include <vector>

int main() {
    // Параметры массива антенн
    const float c = 3e8f;                           // Скорость света, м/с
    const float carrier_freq = 1e9f;                // Частота несущей, Гц
    const float wavelength = c / carrier_freq;      // Длина волны
    const float element_spacing = wavelength / 2.0f; // Расстояние между антеннами
    
    // Параметры сигнала
    const float f_start = 100.0f;
    const float f_stop = 500.0f;
    LFMSignalGenerator lfm(f_start, f_stop, 8000.0f, 1.0f);
    
    // Буфер для 64 антенн
    const size_t num_beams = 64;
    const size_t num_samples = 8000;
    SignalBuffer buffer(num_beams, num_samples);
    
    // Указатели
    std::vector<std::complex<float>*> beam_ptrs(num_beams);
    for (size_t i = 0; i < num_beams; ++i) {
        beam_ptrs[i] = buffer.GetBeamData(i);
    }
    
    // Направление прихода сигнала (угол)
    const float theta = 30.0f * 3.14159f / 180.0f; // 30 градусов
    
    // Вычисляем задержки для каждой антенны
    // На основе угла прихода сигнала
    std::vector<float> delays(num_beams);
    for (size_t n = 0; n < num_beams; ++n) {
        // Разница в пути: d*sin(θ)
        float path_diff = element_spacing * std::sin(theta) * n;
        
        // Преобразуем в задержку (в отсчётах)
        // delay = path_diff / (c / sample_rate)
        float sample_rate = 8000.0f;
        delays[n] = (path_diff / c) * sample_rate;
    }
    
    // Генерируем сигналы с соответствующими задержками
    lfm.GenerateAllBeams(beam_ptrs, num_samples, num_beams, delays);
    
    std::cout << "✅ Симуляция DOA (θ = 30°):\n";
    std::cout << "  Максимальная задержка: " << delays[num_beams-1] << " отсчётов\n";
    
    return 0;
}
```

---

## Доступ к данным

### Пример 5: Разные способы доступа к элементам

```cpp
#include "signal_buffer.h"

int main() {
    SignalBuffer buffer(10, 1000);
    
    // СПОСОБ 1: Через указатель на луч (БЫСТРО - для циклов)
    {
        std::cout << "Способ 1: Указатель на луч\n";
        auto* beam_5 = buffer.GetBeamData(5);
        
        for (size_t sample = 0; sample < 100; ++sample) {
            auto value = beam_5[sample];
            std::cout << "Луч 5, Отсчёт " << sample << ": " 
                      << value.real() << " + j" << value.imag() << "\n";
        }
    }
    
    // СПОСОБ 2: Через GetElement() (УДОБНО для проверок)
    {
        std::cout << "\nСпособ 2: Метод GetElement()\n";
        auto element = buffer.GetElement(5, 50);
        std::cout << "Element[5][50] = " << element.real() << " + j" << element.imag() << "\n";
    }
    
    // СПОСОБ 3: Через SetElement() (для установки значений)
    {
        std::cout << "\nСпособ 3: Метод SetElement()\n";
        std::complex<float> new_value(1.0f, -0.5f);
        buffer.SetElement(5, 50, new_value);
        auto element = buffer.GetElement(5, 50);
        std::cout << "Установлено: " << element.real() << " + j" << element.imag() << "\n";
    }
    
    // СПОСОБ 4: Прямой доступ к линейному буферу (для GPU/SIMD)
    {
        std::cout << "\nСпособ 4: Прямой доступ GetRawData()\n";
        auto& raw = buffer.GetRawData();
        std::cout << "Общий размер буфера: " << raw.size() << " элементов\n";
    }
    
    return 0;
}
```

---

## Интеграция с GPU

### Пример 7: Копирование на GPU (OpenCL)

```cpp
#include "signal_buffer.h"
#include <CL/cl.h>

int main() {
    // Создаём буфер
    SignalBuffer buffer(256, 8000);
    
    // OpenCL setup
    cl_context context = clCreateContextFromType(
        nullptr, CL_DEVICE_TYPE_GPU, nullptr, nullptr, nullptr);
    // ... (инициализация OpenCL) ...
    
    // ✨ Ключевое преимущество линейного буфера:
    // Прямое копирование без переупаковки!
    
    auto& raw_data = buffer.GetRawData();
    
    cl_mem gpu_buffer = clCreateBuffer(
        context,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
        raw_data.size() * sizeof(std::complex<float>),
        (void*)raw_data.data(),  // ← Прямой указатель!
        nullptr
    );
    
    std::cout << "✅ Данные скопированы на GPU\n";
    std::cout << "   Размер: " << (raw_data.size() * sizeof(std::complex<float>) / 1e6) 
              << " МБ\n";
    
    return 0;
}
```

---

## Советы оптимизации

### Пример 8: Производительность доступа

```cpp
#include "signal_buffer.h"
#include <chrono>

int main() {
    const size_t num_beams = 256;
    const size_t num_samples = 8000;
    SignalBuffer buffer(num_beams, num_samples);
    
    // ⚠️ МЕДЛЕННО: Много GetElement() вызовов (проверки границ)
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t b = 0; b < 10; ++b) {
            for (size_t s = 0; s < num_samples; ++s) {
                auto elem = buffer.GetElement(b, s);  // Проверка + доступ
                // Обработка...
            }
        }
        
        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "GetElement(): " 
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " мс\n";
    }
    
    // ✅ БЫСТРО: Кешируем указатель на луч
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t b = 0; b < 10; ++b) {
            auto* beam_data = buffer.GetBeamData(b);  // Один раз
            for (size_t s = 0; s < num_samples; ++s) {
                auto elem = beam_data[s];  // Прямой доступ
                // Обработка...
            }
        }
        
        auto duration = std::chrono::high_resolution_clock::now() - start;
        std::cout << "GetBeamData() + прямой доступ: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                  << " мс\n";
    }
    
    return 0;
}
```

---

**Помните**: Для максимальной производительности используйте `GetBeamData()` в циклах и `GetRawData()` для GPU операций!
