#include "lfm_signal_generator.h"
#include <cmath>
#include <algorithm>

LFMSignalGenerator::LFMSignalGenerator(float f_start, float f_stop, 
                                       float sample_rate, float duration)
    : f_start_(f_start), f_stop_(f_stop), 
      sample_rate_(sample_rate), duration_(duration) {
}

void LFMSignalGenerator::GenerateBeam(std::complex<float>* signal, 
                                      size_t num_samples,
                                      float delay_samples) {
    const float pi = 3.14159265359f;
    const float bandwidth = f_stop_ - f_start_;
    const float chirp_rate = bandwidth / duration_;
    
    // Вычисляем целую и дробную части задержки
    int delay_int = static_cast<int>(delay_samples);
    float delay_frac = delay_samples - delay_int;
    
    for (size_t n = 0; n < num_samples; ++n) {
        // Время с учётом задержки
        int sample_idx = static_cast<int>(n) - delay_int;
        if (sample_idx < 0) {
            signal[n] = std::complex<float>(0.0f, 0.0f);
            continue;
        }
        
        float t = sample_idx / sample_rate_;
        
        // ЛЧМ сигнал: exp(j * 2π * (f_start*t + chirp_rate/2 * t²))
        float phase = 2.0f * pi * (f_start_ * t + 0.5f * chirp_rate * t * t);
        float real_part = std::cos(phase);
        float imag_part = std::sin(phase);
        
        // Применяем дробную задержку через линейную интерполяцию
        if (delay_frac > 0.0f && sample_idx > 0) {
            // Вычисляем интерполированное значение
            float t_prev = (sample_idx - 1) / sample_rate_;
            float phase_prev = 2.0f * pi * (f_start_ * t_prev + 0.5f * chirp_rate * t_prev * t_prev);
            
            float real_prev = std::cos(phase_prev);
            float imag_prev = std::sin(phase_prev);
            
            // Линейная интерполяция
            real_part = (1.0f - delay_frac) * real_prev + delay_frac * real_part;
            imag_part = (1.0f - delay_frac) * imag_prev + delay_frac * imag_part;
        }
        
        signal[n] = std::complex<float>(real_part, imag_part);
    }
}

void LFMSignalGenerator::GenerateAllBeams(
    const std::vector<std::complex<float>*>& beams,
    size_t num_samples, size_t num_beams,
    const std::vector<float>& delays) {
    
    for (size_t i = 0; i < num_beams && i < beams.size(); ++i) {
        float delay = (i < delays.size()) ? delays[i] : 0.0f;
        GenerateBeam(beams[i], num_samples, delay);
    }
}
