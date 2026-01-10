#pragma once



#include <complex>
#include <vector>
#include <cmath>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <cassert>

// ═════════════════════════════════════════════════════════════════════
// CONSTANTS
// ═════════════════════════════════════════════════════════════════════

namespace radar {

constexpr float PI = 3.14159265358979f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float SPEED_OF_LIGHT = 3.0e8f;

// ═════════════════════════════════════════════════════════════════════
// ENUMS
// ═════════════════════════════════════════════════════════════════════

enum class LFMVariant : uint8_t {
    BASIC = 0,              // V1: Same signal on all beams
    PHASE_OFFSET = 1,       // V2: Array steering
    DELAY = 2,              // V3: DOA simulation
    BEAMFORMING = 3,        // V4: Phased array
    WINDOWED = 4            // V5: With Hamming window
};

enum class ErrorCode : int {
    SUCCESS = 0,
    INVALID_PARAMS = -1,
    MEMORY_ALLOCATION_FAILED = -2,
    INVALID_BEAM_INDEX = -3,
    GENERATION_FAILED = -4
};

// ═════════════════════════════════════════════════════════════════════
// STRUCTURES
// ═════════════════════════════════════════════════════════════════════

struct LFMParameters {
    float f_start = 100.0f;
    float f_stop = 500.0f;
    float sample_rate = 8000.0f;
    float duration = 1.0f;
    size_t num_beams = 256;
    float steering_angle = 30.0f;  // degrees
    
    // VALIDATION
    bool IsValid() const noexcept {
        return f_start > 0.0f && f_stop > f_start && 
               sample_rate > 2.0f * f_stop &&  // Nyquist criterion
               duration > 0.0f && num_beams > 0;
    }
    
    float GetChirpRate() const noexcept {
        return (f_stop - f_start) / duration;
    }
    
    size_t GetNumSamples() const noexcept {
        return static_cast<size_t>(duration * sample_rate);
    }
    
    float GetWavelength() const noexcept {
        float f_center = (f_start + f_stop) / 2.0f;
        return SPEED_OF_LIGHT / f_center;
    }
};

struct GenerationStatistics {
    double generation_time_ms = 0.0;
    size_t total_samples = 0;
    float peak_amplitude = 0.0f;
    float rms_value = 0.0f;
};

class SignalBufferNew {
private:
    size_t num_beams_;
    size_t num_samples_;
    bool is_allocated_;
    
public:
    std::vector<std::complex<float>> data_;

    // CONSTRUCTOR
    explicit SignalBufferNew(size_t num_beams, size_t num_samples)
        : num_beams_(num_beams), num_samples_(num_samples), is_allocated_(false) {
        
        if (num_beams == 0 || num_samples == 0) {
            throw std::invalid_argument("num_beams and num_samples must be > 0");
        }
        
        try {
            data_.resize(num_beams * num_samples, std::complex<float>(0.0f, 0.0f));
            is_allocated_ = true;
        } catch (const std::bad_alloc& e) {
            throw std::runtime_error(std::string("Memory allocation failed: ") + e.what());
        }
    }
    
    // MOVE SEMANTICS
    SignalBufferNew(SignalBufferNew&& other) noexcept 
        : data_(std::move(other.data_)),
          num_beams_(other.num_beams_),
          num_samples_(other.num_samples_),
          is_allocated_(other.is_allocated_) {
        other.is_allocated_ = false;
    }
    
    SignalBufferNew& operator=(SignalBufferNew&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            num_beams_ = other.num_beams_;
            num_samples_ = other.num_samples_;
            is_allocated_ = other.is_allocated_;
            other.is_allocated_ = false;
        }
        return *this;
    }
    
    // DELETE COPY
    SignalBufferNew(const SignalBufferNew&) = delete;
    SignalBufferNew& operator=(const SignalBufferNew&) = delete;
    
    ~SignalBufferNew() = default;
    
    // ACCESSORS
    std::complex<float>* GetBeamData(size_t beam_idx) noexcept {
        assert(beam_idx < num_beams_ && is_allocated_);
        return data_.data() + (beam_idx * num_samples_);
    }
    
    const std::complex<float>* GetBeamData(size_t beam_idx) const noexcept {
        assert(beam_idx < num_beams_ && is_allocated_);
        return data_.data() + (beam_idx * num_samples_);
    }
    
    size_t GetNumBeams() const noexcept { return num_beams_; }
    size_t GetNumSamples() const noexcept { return num_samples_; }
    size_t GetTotalSize() const noexcept { return num_beams_ * num_samples_; }
    bool IsAllocated() const noexcept { return is_allocated_; }
    
    std::complex<float>* RawData() noexcept { return data_.data(); }
    const std::complex<float>* RawData() const noexcept { return data_.data(); }
    
    void Clear() noexcept {
        std::fill(data_.begin(), data_.end(), std::complex<float>(0.0f, 0.0f));
    }
    
    size_t MemorySizeBytes() const noexcept {
        return GetTotalSize() * sizeof(std::complex<float>);
    }
};

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

// ═════════════════════════════════════════════════════════════════════
// LFM SIGNAL GENERATOR (Main Class)
// ═════════════════════════════════════════════════════════════════════

class LFMSignalGenerator {
private:
    const LFMParameters params_;
    mutable GenerationStatistics stats_;
    
    // HELPER METHODS
    inline std::complex<float> GenerateComplexSample(float phase) const noexcept {
        return std::complex<float>(std::cos(phase), std::sin(phase));
    }
    
    inline float ComputePhase(float t, float phase_offset = 0.0f) const noexcept {
        float chirp_rate = params_.GetChirpRate();
        return TWO_PI * (params_.f_start * t + 0.5f * chirp_rate * t * t) + phase_offset;
    }
    
    // PRIVATE GENERATION METHODS
    void GenerateVariant_Basic(std::complex<float>* beam_data, size_t num_samples) const noexcept;
    void GenerateVariant_PhaseOffset(std::complex<float>* beam_data, size_t num_samples,
                                     float phase_offset) const noexcept;
    void GenerateVariant_Delay(std::complex<float>* beam_data, size_t num_samples,
                               float delay_samples) const noexcept;
    void GenerateVariant_Beamforming(std::complex<float>* beam_data, size_t num_samples,
                                     float phase_shift) const noexcept;
    void GenerateVariant_Windowed(std::complex<float>* beam_data, size_t num_samples) const noexcept;
    
public:
    // CONSTRUCTORS
    explicit LFMSignalGenerator(const LFMParameters& params)
        : params_(params) {
        
        if (!params_.IsValid()) {
            throw std::invalid_argument("Invalid LFM parameters");
        }
    }
    
    explicit LFMSignalGenerator(float f_start, float f_stop, float sample_rate, float duration)
        : LFMSignalGenerator(LFMParameters{f_start, f_stop, sample_rate, duration, 256, 30.0f}) {
    }
    
    // MOVE SEMANTICS
    LFMSignalGenerator(LFMSignalGenerator&&) = default;
    LFMSignalGenerator& operator=(LFMSignalGenerator&&) = default;
    
    // DELETE COPY
    LFMSignalGenerator(const LFMSignalGenerator&) = delete;
    LFMSignalGenerator& operator=(const LFMSignalGenerator&) = delete;
    
    virtual ~LFMSignalGenerator() = default;
    
    // MAIN API
    SignalBufferNew Generate(LFMVariant variant = LFMVariant::BASIC);
    
    ErrorCode GenerateIntoBuffer(SignalBufferNew& buffer, LFMVariant variant = LFMVariant::BASIC);
    
    // SINGLE BEAM GENERATION
    void GenerateBeam(std::complex<float>* beam_data, size_t num_samples,
                     LFMVariant variant, float beam_param = 0.0f) const;
    
    // GETTERS
    const LFMParameters& GetParameters() const noexcept { return params_; }
    const GenerationStatistics& GetStatistics() const noexcept { return stats_; }

    // NEW: Generate signal with noise (vectorized, no loops)
    std::pair<std::vector<std::complex<float>>, std::vector<double>> 
                GetSignalWithNoise(const NoiseParams& params);

};
std::ostream& operator<<(std::ostream& os, const LFMParameters& params);
std::ostream& operator<<(std::ostream& os, const GenerationStatistics& stats);

}  // namespace radar


/*
#include <complex>
#include <vector>

enum class LFMVariant : int {
    V1 = 1,
    V2 = 2,
    V3 = 3,
    V4 = 4,
    V5 = 5
  };

class LFMSignalGenerator
{
  
public:
  LFMSignalGenerator(float f_start, float f_stop, float sample_rate, float duration);
  void GenerateBeam(std::complex<float>* beam_data, size_t num_samples,
                      float phase_offset = 0.0f, float delay_samples = 0.0f);    

  void GenerateAllBeams(std::vector<std::complex<float>*>& beam_data_ptrs,
                          size_t num_samples, size_t num_beams,
                          const std::vector<float>& delays = {});
  ~LFMSignalGenerator();

*/

/**
* ═════════════════════════════════════════════════════════════════════
* Генерация ЛЧМ (LFM - Linear Frequency Modulation) сигнала
* на num_beams каналов
* ═════════════════════════════════════════════════════════════════════
*/

/**
* ═════════════════════════════════════════════════════════════════════
*  ВАРИАНТ 1: Базовый ЛЧМ для всех лучей (одинаковый сигнал)
* ═════════════════════════════════════════════════════════════════════
* ПАРАМЕТРЫ ЛЧМ СИГНАЛА
*  float f_start = 100.0f;              Начальная частота (Гц)
*  float f_stop = 500.0f;               Конечная частота (Гц)
*  float sample_rate = 8000.0f;         Частота дискретизации (Гц)
*  float duration = 1.0f;               Длительность сигнала (сек)
*  size_t num_beams = 256;              Количество лучей (каналов)
*  LFMVariant var 1                     Варинт сигнала
*/  
/*
  void BaseLFM(std::vector<std::complex<float>*>& beam_data_ptrs,
              const float f_start,      // Начальная частота (Гц)
              const float f_stop,       // Конечная частота (Гц)
              const float sample_rate,  // Частота дискретизации (Гц)
              const float duration,     // Длительность сигнала (сек)
              const size_t num_beams,   // Количество лучей (каналов)
              const LFMVariant var=LFMVariant::V1);    // 
  
private:
    float f_start_;
    float f_stop_;
    float sample_rate_;
    float duration_;
    float chirp_rate_;
  void LFM_v1(std::vector<std::complex<float>*>& beam_data_ptrs,
              const float f_start,      // Начальная частота (Гц)
              const float f_stop,       // Конечная частота (Гц)
              const float sample_rate,  // Частота дискретизации (Гц)
              const float duration,     // Длительность сигнала (сек)
              const size_t num_beams);  // Количество лучей (каналов)

  void LFM_v2(std::vector<std::complex<float>*>& beam_data_ptrs,
              const float f_start,      // Начальная частота (Гц)
              const float f_stop,       // Конечная частота (Гц)
              const float sample_rate,  // Частота дискретизации (Гц)
              const float duration,     // Длительность сигнала (сек)
              const size_t num_beams);  // Количество лучей (каналов)

  void LFM_v3(std::vector<std::complex<float>*>& beam_data_ptrs,
              const float f_start,      // Начальная частота (Гц)
              const float f_stop,       // Конечная частота (Гц)
              const float sample_rate,  // Частота дискретизации (Гц)
              const float duration,     // Длительность сигнала (сек)
              const size_t num_beams);  // Количество лучей (каналов)

  void LFM_v4(std::vector<std::complex<float>*>& beam_data_ptrs,
              const float f_start,      // Начальная частота (Гц)
              const float f_stop,       // Конечная частота (Гц)
              const float sample_rate,  // Частота дискретизации (Гц)
              const float duration,     // Длительность сигнала (сек)
              const size_t num_beams);  // Количество лучей (каналов)

  void LFM_v5(std::vector<std::complex<float>*>& beam_data_ptrs,
              const float f_start,      // Начальная частота (Гц)
              const float f_stop,       // Конечная частота (Гц)
              const float sample_rate,  // Частота дискретизации (Гц)
              const float duration,     // Длительность сигнала (сек)
              const size_t num_beams);  // Количество лучей (каналов)
              

};

*/