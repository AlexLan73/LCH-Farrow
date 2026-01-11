# 📘 SOLUTION FINAL - ПОЛНЫЙ КОД

## lfm_angle_array_final.h

```cpp
#pragma once

#include <complex>
#include <vector>
#include <cmath>
#include <string>
#include <stdexcept>

namespace radar {

constexpr float PI = 3.14159265358979f;
constexpr float SPEED_OF_LIGHT = 3.0e8f;

struct AngleArrayParams {
    float f_start = 1.0e6f;
    float f_stop = 2.0e6f;
    float sample_rate = 12.0e6f;
    size_t num_samples = 512;
    
    float angle_start_deg = -15.0f;
    float angle_stop_deg = 15.0f;
    float angle_step_deg = 0.5f;
    
    size_t antenna_element_idx = 5;
    float antenna_element_spacing_m = 100.0f;
    
    size_t lagrange_order = 48;
    size_t lagrange_row = 5;
};

class LFMAngleArray {
private:
    AngleArrayParams params_;
    std::vector<std::vector<std::complex<float>>> m_signal_conjugate;
    
    float ComputeDelay(float angle_deg) const;
    std::vector<std::complex<float>> GenerateBaseLFM() const;
    std::complex<float> InterpolateLagrange(
        const std::vector<std::complex<float>>& signal,
        float delay_samples) const;

public:
    explicit LFMAngleArray(const AngleArrayParams& params);
    
    void Generate();
    
    size_t GetNumAngles() const { return m_signal_conjugate.size(); }
    size_t GetNumSamples() const { return params_.num_samples; }
    
    std::string ExportToJSON() const;
};

} // namespace radar
```

---

## lfm_angle_array_final.cpp

```cpp
#include "lfm_angle_array_final.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace radar {

LFMAngleArray::LFMAngleArray(const AngleArrayParams& params)
    : params_(params) {
    if (params_.f_start <= 0.0f || params_.f_stop <= params_.f_start) {
        throw std::invalid_argument("Invalid frequency parameters");
    }
    if (params_.sample_rate <= 0.0f || params_.num_samples == 0) {
        throw std::invalid_argument("Invalid sampling parameters");
    }
    if (params_.angle_step_deg <= 0.0f) {
        throw std::invalid_argument("Invalid angle step");
    }
}

float LFMAngleArray::ComputeDelay(float angle_deg) const {
    float angle_rad = angle_deg * PI / 180.0f;
    float sin_angle = std::sin(angle_rad);
    
    float f_center = (params_.f_start + params_.f_stop) / 2.0f;
    float wavelength = SPEED_OF_LIGHT / f_center;
    float element_spacing = wavelength / 2.0f;
    
    float element_position = static_cast<float>(params_.antenna_element_idx) * element_spacing;
    float delay_time = (element_position * sin_angle) / SPEED_OF_LIGHT;
    float delay_samples = delay_time * params_.sample_rate;
    
    return delay_samples;
}

std::vector<std::complex<float>> LFMAngleArray::GenerateBaseLFM() const {
    std::vector<std::complex<float>> lfm(params_.num_samples);
    
    float chirp_rate = (params_.f_stop - params_.f_start) / 
                       (static_cast<float>(params_.num_samples) / params_.sample_rate);
    
    for (size_t n = 0; n < params_.num_samples; ++n) {
        float t = static_cast<float>(n) / params_.sample_rate;
        float phase = 2.0f * PI * (params_.f_start * t + 0.5f * chirp_rate * t * t);
        
        lfm[n] = std::complex<float>(std::cos(phase), std::sin(phase));
    }
    
    return lfm;
}

std::complex<float> LFMAngleArray::InterpolateLagrange(
    const std::vector<std::complex<float>>& signal,
    float delay_samples) const {
    
    int delay_int = static_cast<int>(std::floor(delay_samples));
    float delay_frac = delay_samples - delay_int;
    
    if (delay_int < 0 || delay_int >= static_cast<int>(signal.size())) {
        return std::complex<float>(0.0f, 0.0f);
    }
    
    // Лагранжа порядка 48, позиция 5
    size_t order = params_.lagrange_order;
    size_t center = params_.lagrange_row;
    int start = delay_int - static_cast<int>(center);
    
    if (start < 0) start = 0;
    if (start + order > signal.size()) {
        start = signal.size() - order;
    }
    if (start < 0) start = 0;
    
    std::complex<float> result(0.0f, 0.0f);
    
    for (size_t i = 0; i < order && start + i < signal.size(); ++i) {
        float L = 1.0f;
        for (size_t j = 0; j < order; ++j) {
            if (i != j) {
                L *= (delay_frac - static_cast<float>(j)) / 
                     (static_cast<float>(i) - static_cast<float>(j));
            }
        }
        result += signal[start + i] * L;
    }
    
    return result;
}

void LFMAngleArray::Generate() {
    auto base_lfm = GenerateBaseLFM();
    
    size_t num_angles = static_cast<size_t>(
        (params_.angle_stop_deg - params_.angle_start_deg) / params_.angle_step_deg + 1
    );
    
    m_signal_conjugate.resize(num_angles);
    
    for (size_t angle_idx = 0; angle_idx < num_angles; ++angle_idx) {
        float angle_deg = params_.angle_start_deg + 
                         angle_idx * params_.angle_step_deg;
        
        float delay_samples = ComputeDelay(angle_deg);
        
        std::vector<std::complex<float>> delayed_signal(params_.num_samples);
        
        for (size_t n = 0; n < params_.num_samples; ++n) {
            int delayed_idx = static_cast<int>(n) - static_cast<int>(std::round(delay_samples));
            
            if (delayed_idx >= 0 && delayed_idx < static_cast<int>(params_.num_samples)) {
                delayed_signal[n] = InterpolateLagrange(base_lfm, delay_samples);
            } else {
                delayed_signal[n] = std::complex<float>(0.0f, 0.0f);
            }
        }
        
        // Сопряжение
        std::vector<std::complex<float>> conjugate_signal(params_.num_samples);
        for (size_t n = 0; n < params_.num_samples; ++n) {
            conjugate_signal[n] = std::conj(delayed_signal[n]);
        }
        
        m_signal_conjugate[angle_idx] = conjugate_signal;
    }
}

std::string LFMAngleArray::ExportToJSON() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    oss << "{\n";
    oss << "  \"metadata\": {\n";
    oss << "    \"num_angles\": " << m_signal_conjugate.size() << ",\n";
    oss << "    \"num_samples\": " << params_.num_samples << ",\n";
    oss << "    \"angle_start_deg\": " << params_.angle_start_deg << ",\n";
    oss << "    \"angle_stop_deg\": " << params_.angle_stop_deg << ",\n";
    oss << "    \"angle_step_deg\": " << params_.angle_step_deg << ",\n";
    oss << "    \"f_start_hz\": " << params_.f_start << ",\n";
    oss << "    \"f_stop_hz\": " << params_.f_stop << ",\n";
    oss << "    \"sample_rate_hz\": " << params_.sample_rate << ",\n";
    oss << "    \"antenna_element_idx\": " << params_.antenna_element_idx << ",\n";
    oss << "    \"antenna_element_spacing_m\": " << params_.antenna_element_spacing_m << ",\n";
    oss << "    \"lagrange_order\": " << params_.lagrange_order << ",\n";
    oss << "    \"lagrange_row\": " << params_.lagrange_row << "\n";
    oss << "  },\n";
    oss << "  \"reference_signals\": [\n";
    
    for (size_t i = 0; i < m_signal_conjugate.size(); ++i) {
        float angle = params_.angle_start_deg + i * params_.angle_step_deg;
        oss << "    {\n";
        oss << "      \"angle_deg\": " << angle << ",\n";
        oss << "      \"num_samples\": " << params_.num_samples << ",\n";
        oss << "      \"data\": {\n";
        oss << "        \"real\": [";
        
        for (size_t n = 0; n < m_signal_conjugate[i].size(); ++n) {
            if (n > 0) oss << ", ";
            oss << m_signal_conjugate[i][n].real();
        }
        
        oss << "],\n";
        oss << "        \"imag\": [";
        
        for (size_t n = 0; n < m_signal_conjugate[i].size(); ++n) {
            if (n > 0) oss << ", ";
            oss << m_signal_conjugate[i][n].imag();
        }
        
        oss << "]\n";
        oss << "      }\n";
        oss << "    }";
        
        if (i < m_signal_conjugate.size() - 1) oss << ",";
        oss << "\n";
    }
    
    oss << "  ]\n";
    oss << "}\n";
    
    return oss.str();
}

} // namespace radar
```

---

## ИСПОЛЬЗОВАНИЕ

Смотри QUICK_START.md для примера main.cpp
