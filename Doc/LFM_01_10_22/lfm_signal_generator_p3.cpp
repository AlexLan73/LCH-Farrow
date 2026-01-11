#include "lfm_signal_generator_p3.h"
#include <algorithm>
#include <cstring>

namespace radar {

// ═════════════════════════════════════════════════════════════════════
// CONSTRUCTORS
// ═════════════════════════════════════════════════════════════════════

LFMSignalGenerator::LFMSignalGenerator(const AngleArrayParams& params)
    : params_(params) {
    if (!params_.IsValid()) {
        throw std::invalid_argument("Invalid angle array parameters");
    }
}

LFMSignalGenerator::LFMSignalGenerator(
    float f_start, float f_stop, float sample_rate, float duration,
    float angle_start_deg, float angle_stop_deg, float angle_step_deg)
    : LFMSignalGenerator(AngleArrayParams{
        f_start, f_stop, sample_rate, duration,
        angle_start_deg, angle_stop_deg, angle_step_deg,
        48, 5  // lagrange_order, lagrange_row
    }) {}

// ═════════════════════════════════════════════════════════════════════
// PRIVATE: GENERATE BASE LFM SIGNAL (NO DELAY)
// ═════════════════════════════════════════════════════════════════════

std::vector<std::complex<float>> LFMSignalGenerator::GenerateBaseLFM() const {
    size_t num_samples = params_.GetNumSamples();
    std::vector<std::complex<float>> signal(num_samples);
    
    float chirp_rate = params_.GetChirpRate();
    float inv_sample_rate = 1.0f / params_.sample_rate;
    
    for (size_t n = 0; n < num_samples; ++n) {
        float t = static_cast<float>(n) * inv_sample_rate;
        
        // LFM phase: φ(t) = 2π(f_start*t + (μ/2)*t²)
        float phase = TWO_PI * (params_.f_start * t + 0.5f * chirp_rate * t * t);
        
        // Complex sample: e^(jφ)
        signal[n] = std::complex<float>(std::cos(phase), std::sin(phase));
    }
    
    return signal;
}

// ═════════════════════════════════════════════════════════════════════
// PRIVATE: LAGRANGE BASIS POLYNOMIAL
// ═════════════════════════════════════════════════════════════════════

float LFMSignalGenerator::LagrangeBasis(float x, int i, int order) const {
    // Compute L_i(x) - Lagrange basis polynomial
    float result = 1.0f;
    
    for (int m = 0; m < order; ++m) {
        if (m != i) {
            result *= (x - m) / (i - m);
        }
    }
    
    return result;
}

// ═════════════════════════════════════════════════════════════════════
// PRIVATE: LAGRANGE INTERPOLATION FOR SINGLE SAMPLE
// ═════════════════════════════════════════════════════════════════════

std::complex<float> LFMSignalGenerator::InterpolateLagrange(
    const std::vector<std::complex<float>>& signal,
    float delay_samples,
    size_t sample_idx) const {
    
    int delay_int = static_cast<int>(delay_samples);
    float delay_frac = delay_samples - delay_int;
    
    size_t num_samples = signal.size();
    int order = params_.lagrange_order;
    int row = params_.lagrange_row;
    
    // Center position for interpolation window
    int center = static_cast<int>(sample_idx) - delay_int - row;
    
    std::complex<float> result(0.0f, 0.0f);
    
    // Sum over Lagrange basis polynomials
    for (int i = 0; i < order; ++i) {
        int idx = center + i;
        
        // Boundary check
        if (idx < 0 || idx >= static_cast<int>(num_samples)) {
            continue;  // Skip out-of-bounds samples
        }
        
        float basis = LagrangeBasis(delay_frac, i, order);
        result += basis * signal[idx];
    }
    
    return result;
}

// ═════════════════════════════════════════════════════════════════════
// PRIVATE: APPLY FRACTIONAL DELAY TO SIGNAL
// ═════════════════════════════════════════════════════════════════════

std::vector<std::complex<float>> LFMSignalGenerator::ApplyFractionalDelay(
    const std::vector<std::complex<float>>& signal,
    float delay_samples) const {
    
    size_t num_samples = signal.size();
    std::vector<std::complex<float>> delayed_signal(num_samples);
    
    int delay_int = static_cast<int>(delay_samples);
    
    for (size_t n = 0; n < num_samples; ++n) {
        if (n < static_cast<size_t>(delay_int)) {
            // Before signal starts - zero
            delayed_signal[n] = std::complex<float>(0.0f, 0.0f);
        } else {
            // Apply Lagrange interpolation
            delayed_signal[n] = InterpolateLagrange(signal, delay_samples, n);
        }
    }
    
    return delayed_signal;
}

// ═════════════════════════════════════════════════════════════════════
// PUBLIC: GENERATE ANGLE ARRAY (POINT 3)
// ═════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::GenerateAngleArray() {
    size_t num_angles = params_.GetNumAngles();
    
    // Clear previous data
    m_signal_conjugate.clear();
    m_signal_conjugate.resize(num_angles);
    
    // Generate base LFM signal (no delay)
    auto base_signal = GenerateBaseLFM();
    
    // For each angle
    for (size_t angle_idx = 0; angle_idx < num_angles; ++angle_idx) {
        float angle_deg = params_.angle_start_deg + angle_idx * params_.angle_step_deg;
        
        // For Point 3: delay is based on angle
        // Simple model: delay proportional to angle
        // You can replace this with your own formula
        float delay_factor = (angle_deg - params_.angle_start_deg) / 
                            (params_.angle_stop_deg - params_.angle_start_deg);
        float max_delay = params_.sample_rate * 0.001f;  // Max 1ms delay
        float delay_samples = delay_factor * max_delay;
        
        // Apply fractional delay
        auto delayed_signal = ApplyFractionalDelay(base_signal, delay_samples);
        
        // Conjugate for GPU heterodyne mixing: y[n] = x[n] * conj(ref[n])
        for (auto& sample : delayed_signal) {
            sample = std::conj(sample);
        }
        
        // Store
        m_signal_conjugate[angle_idx] = std::move(delayed_signal);
    }
}

// ═════════════════════════════════════════════════════════════════════
// PUBLIC: GET RAW DATA FOR GPU TRANSFER
// ═════════════════════════════════════════════════════════════════════

const std::complex<float>* LFMSignalGenerator::GetRawData() const noexcept {
    if (m_signal_conjugate.empty()) {
        return nullptr;
    }
    return m_signal_conjugate[0].data();
}

// ═════════════════════════════════════════════════════════════════════
// PUBLIC: GET DATA SIZE IN BYTES
// ═════════════════════════════════════════════════════════════════════

size_t LFMSignalGenerator::GetDataSizeBytes() const noexcept {
    return m_signal_conjugate.size() * m_signal_conjugate[0].size() * 
           sizeof(std::complex<float>);
}

} // namespace radar
