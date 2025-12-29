
#include "../include/lfm_signal_generator.h"

// ═════════════════════════════════════════════════════════════════════
// КЛАСС ГЕНЕРАТОРА ЛЧМ (РЕКОМЕНДУЕТСЯ!)
// ═════════════════════════════════════════════════════════════════════

 
    
LFMSignalGenerator::LFMSignalGenerator(float f_start, float f_stop, float sample_rate, float duration)
        : f_start_(f_start), f_stop_(f_stop), sample_rate_(sample_rate), duration_(duration) {
  chirp_rate_ = (f_stop_ - f_start_) / duration_;
}

LFMSignalGenerator::~LFMSignalGenerator() {
    // Деструктор (пустой, так как нет динамической памяти)
}
    
    // Генерировать ЛЧМ сигнал для одного луча
void LFMSignalGenerator::GenerateBeam(std::complex<float>* beam_data, size_t num_samples, float phase_offset, float delay_samples) {
  int delay_int = static_cast<int>(delay_samples);
        
  for (size_t sample = 0; sample < num_samples; ++sample) {
    int delayed_sample = static_cast<int>(sample) - delay_int;
            
    if (delayed_sample < 0) {
      beam_data[sample] = std::complex<float>(0.0f, 0.0f);
    } else {
        float t = static_cast<float>(delayed_sample) / sample_rate_;
                
        // ЛЧМ фаза: φ(t) = 2π(f_start*t + (μ/2)*t²)
        float phase = 2.0f * 3.14159265f * (f_start_ * t + 0.5f * chirp_rate_ * t * t) + phase_offset;
                
        beam_data[sample] = std::complex<float>(std::cos(phase),std::sin(phase));
      }
  }
}
    
// Генерировать для всех лучей с разными задержками
void LFMSignalGenerator::GenerateAllBeams(std::vector<std::complex<float>*>& beam_data_ptrs,
                          size_t num_samples, size_t num_beams, const std::vector<float>& delays) {
  for (size_t beam = 0; beam < num_beams; ++beam) {
    float delay = (delays.empty()) ? 0.0f : delays[beam];
    float phase_offset = (delays.empty()) ? 0.0f : 2.0f * 3.14159265f * beam / num_beams;
            LFMSignalGenerator::GenerateBeam(beam_data_ptrs[beam], num_samples, phase_offset, delay);
  }
}




/* 


// ═════════════════════════════════════════════════════════════════════
// Генерация ЛЧМ (LFM - Linear Frequency Modulation) сигнала
// на num_beams каналов
// ═════════════════════════════════════════════════════════════════════

// ПАРАМЕТРЫ ЛЧМ СИГНАЛА
const float f_start = 100.0f;          // Начальная частота (Гц)
const float f_stop = 500.0f;            // Конечная частота (Гц)
const float sample_rate = 8000.0f;      // Частота дискретизации (Гц)
const float duration = 1.0f;            // Длительность сигнала (сек)
const size_t num_samples = static_cast<size_t>(duration * sample_rate);
const size_t num_beams = 256;           // Количество лучей (каналов)

// Коэффициент sweep (модуляции)
const float chirp_rate = (f_stop - f_start) / duration;  // Hz/sec

// ═════════════════════════════════════════════════════════════════════
// ВАРИАНТ 1: Базовый ЛЧМ для всех лучей (одинаковый сигнал)
// ═════════════════════════════════════════════════════════════════════

for (size_t beam = 0; beam < num_beams; ++beam) {
    auto* beam_data = signal_buffer.GetBeamData(beam);
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        // Время текущего отсчёта (секунды)
        float t = static_cast<float>(sample) / sample_rate;
        
        // Текущая частота ЛЧМ сигнала
        float f_current = f_start + chirp_rate * t;
        
        // Фаза: φ(t) = 2π * (f_start * t + (chirp_rate/2) * t²)
        float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
        
        // Комплексный сигнал
        beam_data[sample] = std::complex<float>(
            std::cos(phase),
            std::sin(phase)
        );
    }
}

// ═════════════════════════════════════════════════════════════════════
// ВАРИАНТ 2: ЛЧМ с разными начальными фазами на каждом луче
// ═════════════════════════════════════════════════════════════════════

for (size_t beam = 0; beam < num_beams; ++beam) {
    auto* beam_data = signal_buffer.GetBeamData(beam);
    
    // Начальная фаза для каждого луча (array steering)
    float phase_offset = 2.0f * 3.14159265f * beam / num_beams;  // 0 до 2π
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) / sample_rate;
        float f_current = f_start + chirp_rate * t;
        
        // Фаза с offset для beam steering
        float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t) + phase_offset;
        
        beam_data[sample] = std::complex<float>(
            std::cos(phase),
            std::sin(phase)
        );
    }
}

// ═════════════════════════════════════════════════════════════════════
// ВАРИАНТ 3: ЛЧМ с разными задержками на каждом луче
// (симуляция радарного луча, отражённого от объекта)
// ═════════════════════════════════════════════════════════════════════

for (size_t beam = 0; beam < num_beams; ++beam) {
    auto* beam_data = signal_buffer.GetBeamData(beam);
    
    // Задержка зависит от луча (эмуляция DOA - Direction of Arrival)
    // Максимальная задержка: 0.5 периода между первым и последним лучом
    float delay_samples = (static_cast<float>(beam) / num_beams) * (sample_rate / (2.0f * f_start));
    int delay_int = static_cast<int>(delay_samples);
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        int delayed_sample = static_cast<int>(sample) - delay_int;
        
        if (delayed_sample < 0) {
            // До начала сигнала - зану́лить
            beam_data[sample] = std::complex<float>(0.0f, 0.0f);
        } else {
            float t = static_cast<float>(delayed_sample) / sample_rate;
            float f_current = f_start + chirp_rate * t;
            
            // Фаза ЛЧМ
            float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
            
            beam_data[sample] = std::complex<float>(
                std::cos(phase),
                std::sin(phase)
            );
        }
    }
}

// ═════════════════════════════════════════════════════════════════════
// ВАРИАНТ 4: ЛЧМ с фазовым фокусированием (beamforming)
// ═════════════════════════════════════════════════════════════════════

const float c = 3e8f;                   // Скорость света (м/s)
const float wavelength = c / ((f_start + f_stop) / 2.0f);  // Длина волны
const float element_spacing = wavelength / 2.0f;  // Расстояние между элементами
const float steering_angle = 30.0f * 3.14159265f / 180.0f;  // Угол 30 градусов

for (size_t beam = 0; beam < num_beams; ++beam) {
    auto* beam_data = signal_buffer.GetBeamData(beam);
    
    // Фазовый сдвиг для steering (фокусирование в направлении)
    float element_position = static_cast<float>(beam) * element_spacing;
    float phase_shift = 2.0f * 3.14159265f * element_position * std::sin(steering_angle) / wavelength;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) / sample_rate;
        float f_current = f_start + chirp_rate * t;
        
        // ЛЧМ фаза + фазовый steering
        float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t) + phase_shift;
        
        beam_data[sample] = std::complex<float>(
            std::cos(phase),
            std::sin(phase)
        );
    }
}

// ═════════════════════════════════════════════════════════════════════
// ВАРИАНТ 5: ЛЧМ с амплитудной модуляцией (envelope)
// ═════════════════════════════════════════════════════════════════════

for (size_t beam = 0; beam < num_beams; ++beam) {
    auto* beam_data = signal_buffer.GetBeamData(beam);
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) / sample_rate;
        
        // Нормализованное время [0, 1]
        float t_norm = t / duration;
        
        // Оконная функция (Hamming window для уменьшения боковых лепестков)
        float window = 0.54f - 0.46f * std::cos(2.0f * 3.14159265f * t_norm);
        
        // ЛЧМ фаза
        float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
        
        // Сигнал с окном
        float amplitude = window;
        
        beam_data[sample] = std::complex<float>(
            amplitude * std::cos(phase),
            amplitude * std::sin(phase)
        );
    }
}



*/



// ═════════════════════════════════════════════════════════════════════
// ПРИМЕР ИСПОЛЬЗОВАНИЯ:
// ═════════════════════════════════════════════════════════════════════

/*
// Параметры
const float f_start = 100.0f;
const float f_stop = 500.0f;
const float sample_rate = 8000.0f;
const float duration = 1.0f;

// Создать генератор
LFMSignalGenerator lfm(f_start, f_stop, sample_rate, duration);

// Получить буфер данных
const size_t num_samples = static_cast<size_t>(duration * sample_rate);
const size_t num_beams = 256;

std::vector<std::complex<float>*> beam_ptrs(num_beams);
for (size_t b = 0; b < num_beams; ++b) {
    beam_ptrs[b] = signal_buffer.GetBeamData(b);
}

// Задержки для каждого луча (имитация DOA)
std::vector<float> delays(num_beams);
for (size_t b = 0; b < num_beams; ++b) {
    delays[b] = b * 0.1f;  // 0, 0.1, 0.2, ... задержки в отсчётах
}

// Генерировать все лучи
lfm.GenerateAllBeams(beam_ptrs, num_samples, num_beams, delays);

printf("✅ ЛЧМ сигнал сгенерирован для %zu лучей\n", num_beams);
printf("   Частота: %.0f - %.0f Гц\n", f_start, f_stop);
printf("   Длительность: %.2f сек\n", duration);
printf("   Отсчётов: %zu\n", num_samples);
*/
