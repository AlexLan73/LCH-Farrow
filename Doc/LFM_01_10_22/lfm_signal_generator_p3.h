#pragma once

#include <complex>
#include <vector>
#include <cmath>
#include <cstddef>
#include <stdexcept>

// ═════════════════════════════════════════════════════════════════════
// LFM SIGNAL GENERATOR - ANGLE ARRAY FOR POINT 3 (DATA PREPARATION)
// ═════════════════════════════════════════════════════════════════════

namespace radar {

constexpr float PI = 3.14159265358979f;
constexpr float TWO_PI = 2.0f * PI;

// ═════════════════════════════════════════════════════════════════════
// PARAMETERS FOR ANGLE ARRAY GENERATION
// ═════════════════════════════════════════════════════════════════════

struct AngleArrayParams {
    // LFM Signal parameters
    float f_start = 100.0f;           // Start frequency (Hz)
    float f_stop = 500.0f;            // Stop frequency (Hz)
    float sample_rate = 8000.0f;      // Sample rate (Hz)
    float duration = 1.0f;            // Duration (sec)
    
    // Angle array parameters (PARAMETERIZED - not fixed!)
    float angle_start_deg = -10.0f;   // Start angle (degrees)
    float angle_stop_deg = 10.0f;     // Stop angle (degrees)
    float angle_step_deg = 0.5f;      // Angle step (degrees)
    
    // Lagrange interpolation parameters
    int lagrange_order = 48;          // Order of interpolation (must be even)
    int lagrange_row = 5;             // Center row position
    
    // VALIDATION
    bool IsValid() const noexcept {
        return f_start > 0.0f && f_stop > f_start &&
               sample_rate > 2.0f * f_stop &&  // Nyquist
               duration > 0.0f &&
               angle_start_deg <= angle_stop_deg &&
               angle_step_deg > 0.0f &&
               lagrange_order > 0 && lagrange_row >= 0;
    }
    
    // COMPUTED PROPERTIES
    size_t GetNumSamples() const noexcept {
        return static_cast<size_t>(duration * sample_rate);
    }
    
    size_t GetNumAngles() const noexcept {
        return static_cast<size_t>((angle_stop_deg - angle_start_deg) / angle_step_deg) + 1;
    }
    
    float GetChirpRate() const noexcept {
        return (f_stop - f_start) / duration;
    }
};

// ═════════════════════════════════════════════════════════════════════
// MAIN CLASS: LFM SIGNAL GENERATOR WITH ANGLE ARRAY
// ═════════════════════════════════════════════════════════════════════

class LFMSignalGenerator {
private:
    const AngleArrayParams params_;
    
    // Storage for conjugated signals with fractional delays
    // m_signal_conjugate[angle_idx][sample_idx]
    std::vector<std::vector<std::complex<float>>> m_signal_conjugate;
    
    // ─────────────────────────────────────────────────────────────────
    // PRIVATE HELPER METHODS
    // ─────────────────────────────────────────────────────────────────
    
    // Generate base LFM signal (no delay)
    std::vector<std::complex<float>> GenerateBaseLFM() const;
    
    // Apply fractional delay using Lagrange interpolation
    std::vector<std::complex<float>> ApplyFractionalDelay(
        const std::vector<std::complex<float>>& signal,
        float delay_samples) const;
    
    // Lagrange interpolation for single sample
    std::complex<float> InterpolateLagrange(
        const std::vector<std::complex<float>>& signal,
        float delay_samples,
        size_t sample_idx) const;
    
    // Compute Lagrange basis polynomial
    float LagrangeBasis(float x, int i, int order) const;
    
public:
    // ─────────────────────────────────────────────────────────────────
    // CONSTRUCTORS
    // ─────────────────────────────────────────────────────────────────
    
    explicit LFMSignalGenerator(const AngleArrayParams& params);
    
    explicit LFMSignalGenerator(
        float f_start, float f_stop, float sample_rate, float duration,
        float angle_start_deg = -10.0f, float angle_stop_deg = 10.0f, 
        float angle_step_deg = 0.5f);
    
    // MOVE SEMANTICS
    LFMSignalGenerator(LFMSignalGenerator&&) = default;
    LFMSignalGenerator& operator=(LFMSignalGenerator&&) = default;
    
    // DELETE COPY
    LFMSignalGenerator(const LFMSignalGenerator&) = delete;
    LFMSignalGenerator& operator=(const LFMSignalGenerator&) = delete;
    
    virtual ~LFMSignalGenerator() = default;
    
    // ─────────────────────────────────────────────────────────────────
    // POINT 3: GENERATE ANGLE ARRAY WITH FRACTIONAL DELAYS
    // ─────────────────────────────────────────────────────────────────
    
    // Generate m_signal_conjugate[num_angles] with fractional delays
    // Each signal is conjugated and ready for GPU heterodyne mixing
    void GenerateAngleArray();
    
    // ─────────────────────────────────────────────────────────────────
    // ACCESSORS
    // ─────────────────────────────────────────────────────────────────
    
    const AngleArrayParams& GetParameters() const noexcept {
        return params_;
    }
    
    size_t GetNumAngles() const noexcept {
        return params_.GetNumAngles();
    }
    
    size_t GetNumSamples() const noexcept {
        return params_.GetNumSamples();
    }
    
    // Get signal for specific angle index
    const std::vector<std::complex<float>>* GetSignal(size_t angle_idx) const noexcept {
        if (angle_idx < m_signal_conjugate.size()) {
            return &m_signal_conjugate[angle_idx];
        }
        return nullptr;
    }
    
    // Get all signals (for GPU transfer)
    const std::vector<std::vector<std::complex<float>>>& GetAllSignals() const noexcept {
        return m_signal_conjugate;
    }
    
    // Get raw pointer to data for GPU (row-major layout)
    const std::complex<float>* GetRawData() const noexcept;
    
    // Get size in bytes for GPU transfer
    size_t GetDataSizeBytes() const noexcept;
    
    // Get angle value for index
    float GetAngleForIndex(size_t angle_idx) const noexcept {
        return params_.angle_start_deg + angle_idx * params_.angle_step_deg;
    }
};

} // namespace radar

/*
 * USAGE EXAMPLE (Point 3):
 * 
 *   radar::AngleArrayParams params;
 *   params.f_start = 100.0f;
 *   params.f_stop = 500.0f;
 *   params.sample_rate = 8000.0f;
 *   params.duration = 1.0f;
 *   params.angle_start_deg = -10.0f;
 *   params.angle_stop_deg = 10.0f;
 *   params.angle_step_deg = 0.5f;
 *   params.lagrange_order = 48;
 *   params.lagrange_row = 5;
 *   
 *   LFMSignalGenerator gen(params);
 *   gen.GenerateAngleArray();  // Generate m_signal_conjugate[41][num_samples]
 *   
 *   // Transfer to GPU:
 *   auto data = gen.GetRawData();
 *   size_t size = gen.GetDataSizeBytes();
 *   // cudaMemcpy(..., data, size, cudaMemcpyHostToDevice);
 */
