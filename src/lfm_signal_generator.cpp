
#include <random>
#include <cmath>

//#include "../include/lfm_signal_generator.h"
// //#include <signal_buffer.h>

// ═════════════════════════════════════════════════════════════════════
// lfm_signal_generator.cpp (OPTIMIZED - Senior Level)
// ═════════════════════════════════════════════════════════════════════

#include "../include/lfm_signal_generator.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace radar {

// ═════════════════════════════════════════════════════════════════════
// PRIVATE IMPLEMENTATION
// ═════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::GenerateVariant_Basic(
    std::complex<float>* beam_data, 
    size_t num_samples) const noexcept {
    
    const float inv_sample_rate = 1.0f / params_.sample_rate;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) * inv_sample_rate;
        float phase = ComputePhase(t);
        beam_data[sample] = GenerateComplexSample(phase);
    }
}

void LFMSignalGenerator::GenerateVariant_PhaseOffset(
    std::complex<float>* beam_data, 
    size_t num_samples,
    float phase_offset) const noexcept {
    
    const float inv_sample_rate = 1.0f / params_.sample_rate;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) * inv_sample_rate;
        float phase = ComputePhase(t, phase_offset);
        beam_data[sample] = GenerateComplexSample(phase);
    }
}

void LFMSignalGenerator::GenerateVariant_Delay(
    std::complex<float>* beam_data, 
    size_t num_samples,
    float delay_samples) const noexcept {
    
    const int delay_int = static_cast<int>(delay_samples);
    const float inv_sample_rate = 1.0f / params_.sample_rate;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        int delayed_sample = static_cast<int>(sample) - delay_int;
        
        if (delayed_sample < 0) {
            beam_data[sample] = std::complex<float>(0.0f, 0.0f);
        } else {
            float t = static_cast<float>(delayed_sample) * inv_sample_rate;
            float phase = ComputePhase(t);
            beam_data[sample] = GenerateComplexSample(phase);
        }
    }
}

void LFMSignalGenerator::GenerateVariant_Beamforming(
    std::complex<float>* beam_data, 
    size_t num_samples,
    float phase_shift) const noexcept {
    
    const float inv_sample_rate = 1.0f / params_.sample_rate;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) * inv_sample_rate;
        float phase = ComputePhase(t, phase_shift);
        beam_data[sample] = GenerateComplexSample(phase);
    }
}

void LFMSignalGenerator::GenerateVariant_Windowed(
    std::complex<float>* beam_data, 
    size_t num_samples) const noexcept {
    
    const float inv_sample_rate = 1.0f / params_.sample_rate;
    const float inv_duration = 1.0f / params_.duration;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        float t = static_cast<float>(sample) * inv_sample_rate;
        float t_norm = t * inv_duration;
        
        // Hamming window: w(n) = 0.54 - 0.46*cos(2πn)
        float window = 0.54f - 0.46f * std::cos(TWO_PI * t_norm);
        
        float phase = ComputePhase(t);
        auto sample_val = GenerateComplexSample(phase);
        beam_data[sample] = sample_val * window;
    }
}

// ═════════════════════════════════════════════════════════════════════
// PUBLIC API
// ═════════════════════════════════════════════════════════════════════

SignalBufferNew LFMSignalGenerator::Generate(LFMVariant variant) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SignalBufferNew buffer(params_.num_beams, params_.GetNumSamples());
    ErrorCode result = GenerateIntoBuffer(buffer, variant);
    
    if (result != ErrorCode::SUCCESS) {
        throw std::runtime_error("Signal generation failed");
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    stats_.generation_time_ms = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    stats_.total_samples = buffer.GetTotalSize();
    
    return buffer;
}

ErrorCode LFMSignalGenerator::GenerateIntoBuffer(
    SignalBufferNew& buffer, 
    LFMVariant variant) {
    
    if (!params_.IsValid()) {
        return ErrorCode::INVALID_PARAMS;
    }
    
    if (!buffer.IsAllocated()) {
        return ErrorCode::MEMORY_ALLOCATION_FAILED;
    }
    
    try {
        const size_t num_samples = params_.GetNumSamples();
        float wavelength = params_.GetWavelength();
        float element_spacing = wavelength / 2.0f;
        float steering_rad = params_.steering_angle * PI / 180.0f;
        
        for (size_t beam = 0; beam < params_.num_beams; ++beam) {
            auto* beam_data = buffer.GetBeamData(beam);
            
            switch (variant) {
                case LFMVariant::BASIC:
                    GenerateVariant_Basic(beam_data, num_samples);
                    break;
                    
                case LFMVariant::PHASE_OFFSET: {
                    float phase_offset = TWO_PI * beam / params_.num_beams;
                    GenerateVariant_PhaseOffset(beam_data, num_samples, phase_offset);
                    break;
                }
                    
                case LFMVariant::DELAY: {
                    float delay_factor = static_cast<float>(beam) / params_.num_beams;
                    float delay_samples = delay_factor * (params_.sample_rate / (2.0f * params_.f_start));
                    GenerateVariant_Delay(beam_data, num_samples, delay_samples);
                    break;
                }
                    
                case LFMVariant::BEAMFORMING: {
                    float element_pos = static_cast<float>(beam) * element_spacing;
                    float phase_shift = TWO_PI * element_pos * std::sin(steering_rad) / wavelength;
                    GenerateVariant_Beamforming(beam_data, num_samples, phase_shift);
                    break;
                }
                    
                case LFMVariant::WINDOWED:
                    GenerateVariant_Windowed(beam_data, num_samples);
                    break;
                    
                default:
                    return ErrorCode::GENERATION_FAILED;
            }
        }
        
        // Compute statistics
        float peak_amp = 0.0f;
        float rms = 0.0f;
        
        const auto* raw_data = buffer.RawData();
        for (size_t i = 0; i < buffer.GetTotalSize(); ++i) {
            float amp = std::abs(raw_data[i]);
            peak_amp = std::max(peak_amp, amp);
            rms += amp * amp;
        }
        
        stats_.peak_amplitude = peak_amp;
        stats_.rms_value = std::sqrt(rms / buffer.GetTotalSize());
        
        return ErrorCode::SUCCESS;
        
    } catch (const std::exception& e) {
        std::cerr << "Generation error: " << e.what() << std::endl;
        return ErrorCode::GENERATION_FAILED;
    }
}

void LFMSignalGenerator::GenerateBeam(
    std::complex<float>* beam_data, 
    size_t num_samples,
    LFMVariant variant, 
    float beam_param) const {
    
    if (!beam_data || num_samples == 0) {
        throw std::invalid_argument("Invalid beam_data or num_samples");
    }
    
    switch (variant) {
        case LFMVariant::BASIC:
            GenerateVariant_Basic(beam_data, num_samples);
            break;
            
        case LFMVariant::PHASE_OFFSET:
        case LFMVariant::BEAMFORMING:
            GenerateVariant_PhaseOffset(beam_data, num_samples, beam_param);
            break;
            
        case LFMVariant::DELAY:
            GenerateVariant_Delay(beam_data, num_samples, beam_param);
            break;
            
        case LFMVariant::WINDOWED:
            GenerateVariant_Windowed(beam_data, num_samples);
            break;
    }
}


std::pair<std::vector<std::complex<float>>, std::vector<double>>
    LFMSignalGenerator::GetSignalWithNoise(const NoiseParams& params) {
    
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
        double phase = 2.0 * radar::PI * params.f0 * tn +
                      radar::PI * params.fdev / params.ti * (dt_half * dt_half) +
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




// ═════════════════════════════════════════════════════════════════════
// HELPER: Pretty printing
// ═════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const LFMParameters& params) {
    os << "LFM Parameters:\n"
       << "  Frequency range: " << params.f_start << " - " << params.f_stop << " Hz\n"
       << "  Sample rate: " << params.sample_rate << " Hz\n"
       << "  Duration: " << params.duration << " sec\n"
       << "  Num beams: " << params.num_beams << "\n"
       << "  Chirp rate: " << params.GetChirpRate() << " Hz/sec\n"
       << "  Num samples: " << params.GetNumSamples() << "\n"
       << "  Wavelength: " << params.GetWavelength() << " m";
    return os;
}

std::ostream& operator<<(std::ostream& os, const GenerationStatistics& stats) {
    os << "Generation Statistics:\n"
       << "  Time: " << stats.generation_time_ms << " ms\n"
       << "  Total samples: " << stats.total_samples << "\n"
       << "  Peak amplitude: " << stats.peak_amplitude << "\n"
       << "  RMS value: " << stats.rms_value;
    return os;
}


}  // namespace radar












// // ═════════════════════════════════════════════════════════════════════
// // КЛАСС ГЕНЕРАТОРА ЛЧМ (РЕКОМЕНДУЕТСЯ!)
// // ═════════════════════════════════════════════════════════════════════

 
    
// LFMSignalGenerator::LFMSignalGenerator(float f_start, float f_stop, float sample_rate, float duration)
//         : f_start_(f_start), f_stop_(f_stop), sample_rate_(sample_rate), duration_(duration) {
//   chirp_rate_ = (f_stop_ - f_start_) / duration_;
// }

// LFMSignalGenerator::~LFMSignalGenerator() {
//     // Деструктор (пустой, так как нет динамической памяти)
// }
    
//     // Генерировать ЛЧМ сигнал для одного луча
// void LFMSignalGenerator::GenerateBeam(std::complex<float>* beam_data, size_t num_samples, float phase_offset, float delay_samples) {
//   int delay_int = static_cast<int>(delay_samples);
        
//   for (size_t sample = 0; sample < num_samples; ++sample) {
//     int delayed_sample = static_cast<int>(sample) - delay_int;
            
//     if (delayed_sample < 0) {
//       beam_data[sample] = std::complex<float>(0.0f, 0.0f);
//     } else {
//         float t = static_cast<float>(delayed_sample) / sample_rate_;
                
//         // ЛЧМ фаза: φ(t) = 2π(f_start*t + (μ/2)*t²)
//         float phase = 2.0f * 3.14159265f * (f_start_ * t + 0.5f * chirp_rate_ * t * t) + phase_offset;
                
//         beam_data[sample] = std::complex<float>(std::cos(phase),std::sin(phase));
//       }
//   }
// }
    
// // Генерировать для всех лучей с разными задержками
// void LFMSignalGenerator::GenerateAllBeams(std::vector<std::complex<float>*>& beam_data_ptrs,
//                           size_t num_samples, size_t num_beams, const std::vector<float>& delays) {
//   for (size_t beam = 0; beam < num_beams; ++beam) {
//     float delay = (delays.empty()) ? 0.0f : delays[beam];
//     float phase_offset = (delays.empty()) ? 0.0f : 2.0f * 3.14159265f * beam / num_beams;
//             LFMSignalGenerator::GenerateBeam(beam_data_ptrs[beam], num_samples, phase_offset, delay);
//   }
// }

// /**
// * ═════════════════════════════════════════════════════════════════════
// * Генерация ЛЧМ (LFM - Linear Frequency Modulation) сигнала
// * на num_beams каналов
// * ═════════════════════════════════════════════════════════════════════
// */

// /**
// * ═════════════════════════════════════════════════════════════════════
// *  ВАРИАНТ 1: Базовый ЛЧМ для всех лучей (одинаковый сигнал)
// * ═════════════════════════════════════════════════════════════════════
// * ПАРАМЕТРЫ ЛЧМ СИГНАЛА
// *  float f_start = 100.0f;          // Начальная частота (Гц)
// *  float f_stop = 500.0f;            // Конечная частота (Гц)
// *  float sample_rate = 8000.0f;      // Частота дискретизации (Гц)
// *  float duration = 1.0f;            // Длительность сигнала (сек)
// *  size_t num_beams = 256;           // Количество лучей (каналов)
// */  
// void LFMSignalGenerator::BaseLFM(std::vector<std::complex<float>*>& beam_data_ptrs, 
//                                   const float f_start,      // Начальная частота (Гц)
//                                   const float f_stop,       // Конечная частота (Гц)
//                                   const float sample_rate,  // Частота дискретизации (Гц)
//                                   const float duration,     // Длительность сигнала (сек)
//                                   const size_t num_beams,   // Количество лучей (каналов)
//                                   const LFMVariant var)     // Тип формирования сигнала  
// {
//  switch (var) {
//         case LFMVariant::V1:
//           LFMSignalGenerator::LFM_v1(beam_data_ptrs, f_start, f_stop, sample_rate, duration, num_beams);        
//           break;
//         case LFMVariant::V2:
//           LFMSignalGenerator::LFM_v2(beam_data_ptrs, f_start, f_stop, sample_rate, duration, num_beams);        
//             break;
//         case LFMVariant::V3:
//           LFMSignalGenerator::LFM_v3(beam_data_ptrs, f_start, f_stop, sample_rate, duration, num_beams);        
//             break;
//         case LFMVariant::V4:
//           LFMSignalGenerator::LFM_v4(beam_data_ptrs, f_start, f_stop, sample_rate, duration, num_beams);        
//             break;
//         case LFMVariant::V5:
//           LFMSignalGenerator::LFM_v4(beam_data_ptrs, f_start, f_stop, sample_rate, duration, num_beams);        
//             break;
//         default:
//             printf("Другое типы формирования сигналов. \n");
//     }

// }

// void LFMSignalGenerator::LFM_v1(std::vector<std::complex<float>*>& beam_data_ptrs,
//               const float f_start,      // Начальная частота (Гц)
//               const float f_stop,       // Конечная частота (Гц)
//               const float sample_rate,  // Частота дискретизации (Гц)
//               const float duration,     // Длительность сигнала (сек)
//               const size_t num_beams){  // Количество лучей (каналов)
//   const size_t num_samples = static_cast<size_t>(duration * sample_rate);
//   // Коэффициент sweep (модуляции)
//   const float chirp_rate = (f_stop - f_start) / duration;  // Hz/sec
//   // Создаём компоненты
//   SignalBuffer signal_buffer(num_beams, num_samples);

//   for (size_t beam = 0; beam < num_beams; ++beam) {
//     auto* beam_data = signal_buffer.GetBeamData(beam);
    
//     for (size_t sample = 0; sample < num_samples; ++sample) {
//     // Время текущего отсчёта (секунды)
//       float t = static_cast<float>(sample) / sample_rate;
        
//       // Текущая частота ЛЧМ сигнала
//       float f_current = f_start + chirp_rate * t;
        
//       // Фаза: φ(t) = 2π * (f_start * t + (chirp_rate/2) * t²)
//       float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
        
//       // Комплексный сигнал
//       beam_data[sample] = std::complex<float>(std::cos(phase), std::sin(phase));
//     }
//     beam_data_ptrs[beam] = beam_data; // добавил    
//   }
// }
// /**
// *  ═════════════════════════════════════════════════════════════════════
// *  ВАРИАНТ 2: ЛЧМ с разными начальными фазами на каждом луче
// *  ═════════════════════════════════════════════════════════════════════
// */
// void LFMSignalGenerator::LFM_v2(std::vector<std::complex<float>*>& beam_data_ptrs,
//               const float f_start,      // Начальная частота (Гц)
//               const float f_stop,       // Конечная частота (Гц)
//               const float sample_rate,  // Частота дискретизации (Гц)
//               const float duration,     // Длительность сигнала (сек)
//               const size_t num_beams){  // Количество лучей (каналов)
//   const size_t num_samples = static_cast<size_t>(duration * sample_rate);
//   // Коэффициент sweep (модуляции)
//   const float chirp_rate = (f_stop - f_start) / duration;  // Hz/sec
//   // Создаём компоненты
//   SignalBuffer signal_buffer(num_beams, num_samples);
//   for (size_t beam = 0; beam < num_beams; ++beam) {
//     auto* beam_data = signal_buffer.GetBeamData(beam);
      
//     // Начальная фаза для каждого луча (array steering)
//     float phase_offset = 2.0f * 3.14159265f * beam / num_beams;  // 0 до 2π
      
//     for (size_t sample = 0; sample < num_samples; ++sample) {
//       float t = static_cast<float>(sample) / sample_rate;
//       float f_current = f_start + chirp_rate * t;
          
//       // Фаза с offset для beam steering
//       float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t) + phase_offset;
          
//       beam_data[sample] = std::complex<float>(std::cos(phase), std::sin(phase));
//     }
//   }
// }

// /**
//  * ═════════════════════════════════════════════════════════════════════
//  * ВАРИАНТ 3: ЛЧМ с разными задержками на каждом луче
//  * (симуляция радарного луча, отражённого от объекта)
//  * ═════════════════════════════════════════════════════════════════════
//  */
// void LFMSignalGenerator::LFM_v3(std::vector<std::complex<float>*>& beam_data_ptrs,
//               const float f_start,      // Начальная частота (Гц)
//               const float f_stop,       // Конечная частота (Гц)
//               const float sample_rate,  // Частота дискретизации (Гц)
//               const float duration,     // Длительность сигнала (сек)
//               const size_t num_beams){  // Количество лучей (каналов)
//   const size_t num_samples = static_cast<size_t>(duration * sample_rate);
//   // Коэффициент sweep (модуляции)
//   const float chirp_rate = (f_stop - f_start) / duration;  // Hz/sec
//   // Создаём компоненты
//   SignalBuffer signal_buffer(num_beams, num_samples);
//   for (size_t beam = 0; beam < num_beams; ++beam) {
//     auto* beam_data = signal_buffer.GetBeamData(beam);
      
//     // Задержка зависит от луча (эмуляция DOA - Direction of Arrival)
//     // Максимальная задержка: 0.5 периода между первым и последним лучом
//     float delay_samples = (static_cast<float>(beam) / num_beams) * (sample_rate / (2.0f * f_start));
//     int delay_int = static_cast<int>(delay_samples);
      
//     for (size_t sample = 0; sample < num_samples; ++sample) {
//       int delayed_sample = static_cast<int>(sample) - delay_int;
          
//       if (delayed_sample < 0) {
//         // До начала сигнала - зану́лить
//         beam_data[sample] = std::complex<float>(0.0f, 0.0f);
//       } else {
//         float t = static_cast<float>(delayed_sample) / sample_rate;
//         float f_current = f_start + chirp_rate * t;
              
//         // Фаза ЛЧМ
//         float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
              
//         beam_data[sample] = std::complex<float>(std::cos(phase),std::sin(phase));
//       }
//     }
//   }
// }

// /**
//  * 
//  * ═════════════════════════════════════════════════════════════════════
//  * ВАРИАНТ 4: ЛЧМ с фазовым фокусированием (beamforming)
//  * ═════════════════════════════════════════════════════════════════════
//  */
// void LFMSignalGenerator::LFM_v4(std::vector<std::complex<float>*>& beam_data_ptrs,
//               const float f_start,      // Начальная частота (Гц)
//               const float f_stop,       // Конечная частота (Гц)
//               const float sample_rate,  // Частота дискретизации (Гц)
//               const float duration,     // Длительность сигнала (сек)
//               const size_t num_beams){  // Количество лучей (каналов)
//   const size_t num_samples = static_cast<size_t>(duration * sample_rate);
//   // Коэффициент sweep (модуляции)
//   const float chirp_rate = (f_stop - f_start) / duration;  // Hz/sec
//   // Создаём компоненты
//   SignalBuffer signal_buffer(num_beams, num_samples);
//   const float c = 3e8f;                   // Скорость света (м/s)
//   const float wavelength = c / ((f_start + f_stop) / 2.0f);  // Длина волны
//   const float element_spacing = wavelength / 2.0f;  // Расстояние между элементами
//   const float steering_angle = 30.0f * 3.14159265f / 180.0f;  // Угол 30 градусов

//   for (size_t beam = 0; beam < num_beams; ++beam) {
//       auto* beam_data = signal_buffer.GetBeamData(beam);
      
//       // Фазовый сдвиг для steering (фокусирование в направлении)
//       float element_position = static_cast<float>(beam) * element_spacing;
//       float phase_shift = 2.0f * 3.14159265f * element_position * std::sin(steering_angle) / wavelength;
      
//       for (size_t sample = 0; sample < num_samples; ++sample) {
//           float t = static_cast<float>(sample) / sample_rate;
//           float f_current = f_start + chirp_rate * t;
          
//           // ЛЧМ фаза + фазовый steering
//           float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t) + phase_shift;
          
//           beam_data[sample] = std::complex<float>(
//               std::cos(phase),
//               std::sin(phase)
//           );
//       }
//   }
// }

// /**
// * ═════════════════════════════════════════════════════════════════════
// * ВАРИАНТ 5: ЛЧМ с амплитудной модуляцией (envelope)
// * ═════════════════════════════════════════════════════════════════════
// */
// void LFMSignalGenerator::LFM_v5(std::vector<std::complex<float>*>& beam_data_ptrs,
//               const float f_start,      // Начальная частота (Гц)
//               const float f_stop,       // Конечная частота (Гц)
//               const float sample_rate,  // Частота дискретизации (Гц)
//               const float duration,     // Длительность сигнала (сек)
//               const size_t num_beams){  // Количество лучей (каналов)
//   const size_t num_samples = static_cast<size_t>(duration * sample_rate);
//   // Коэффициент sweep (модуляции)
//   const float chirp_rate = (f_stop - f_start) / duration;  // Hz/sec
//   // Создаём компоненты
//   SignalBuffer signal_buffer(num_beams, num_samples);

//   for (size_t beam = 0; beam < num_beams; ++beam) {
//     auto* beam_data = signal_buffer.GetBeamData(beam);
    
//     for (size_t sample = 0; sample < num_samples; ++sample) {
//         float t = static_cast<float>(sample) / sample_rate;
        
//         // Нормализованное время [0, 1]
//         float t_norm = t / duration;
        
//         // Оконная функция (Hamming window для уменьшения боковых лепестков)
//         float window = 0.54f - 0.46f * std::cos(2.0f * 3.14159265f * t_norm);
        
//         // ЛЧМ фаза
//         float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
        
//         // Сигнал с окном
//         float amplitude = window;
        
//         beam_data[sample] = std::complex<float>(amplitude * std::cos(phase), amplitude * std::sin(phase)
//     );
//     }
//   }
// }

// /* 






// // ═════════════════════════════════════════════════════════════════════
// // ВАРИАНТ 5: ЛЧМ с амплитудной модуляцией (envelope)
// // ═════════════════════════════════════════════════════════════════════



// */



// // ═════════════════════════════════════════════════════════════════════
// // ПРИМЕР ИСПОЛЬЗОВАНИЯ:
// // ═════════════════════════════════════════════════════════════════════

// /*
// // Параметры
// const float f_start = 100.0f;
// const float f_stop = 500.0f;
// const float sample_rate = 8000.0f;
// const float duration = 1.0f;

// // Создать генератор
// LFMSignalGenerator lfm(f_start, f_stop, sample_rate, duration);

// // Получить буфер данных
// const size_t num_samples = static_cast<size_t>(duration * sample_rate);
// const size_t num_beams = 256;

// std::vector<std::complex<float>*> beam_ptrs(num_beams);
// for (size_t b = 0; b < num_beams; ++b) {
//     beam_ptrs[b] = signal_buffer.GetBeamData(b);
// }

// // Задержки для каждого луча (имитация DOA)
// std::vector<float> delays(num_beams);
// for (size_t b = 0; b < num_beams; ++b) {
//     delays[b] = b * 0.1f;  // 0, 0.1, 0.2, ... задержки в отсчётах
// }

// // Генерировать все лучи
// lfm.GenerateAllBeams(beam_ptrs, num_samples, num_beams, delays);

// printf("✅ ЛЧМ сигнал сгенерирован для %zu лучей\n", num_beams);
// printf("   Частота: %.0f - %.0f Гц\n", f_start, f_stop);
// printf("   Длительность: %.2f сек\n", duration);
// printf("   Отсчётов: %zu\n", num_samples);
// */
