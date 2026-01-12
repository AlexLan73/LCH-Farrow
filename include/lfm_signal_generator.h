#pragma once

#include "signal_buffer.h"
#include <complex>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <random>
#include <algorithm>
#include <lfm_parameters.h>

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
    BASIC = 0,              // Базовый ЛЧМ для всех лучей одинаково
    PHASE_OFFSET = 1,       // С фазовыми сдвигами (array steering)
    DELAY = 2,              // С временными задержками
    BEAMFORMING = 3,        // С фазовым фокусированием
    WINDOWED = 4,           // С Hamming окном
    ANGLE_SWEEP = 5,        // 🆕 По углам с шагом 0.5° (НОВОЕ!)
    HETERODYNE = 6          // 🆕 Для гетеродина (сопряжённый сигнал)
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

// struct LFMParameters {
//     float f_start = 100.0f;              // Начальная частота (Гц)
//     float f_stop = 500.0f;               // Конечная частота (Гц)
//     float sample_rate = 12.0e6f;         // Частота дискретизации (12 МГц)
//     mutable float duration = 0.0f;       // Длительность сигнала (сек)
//     size_t num_beams = 256;              // Количество лучей
//     float steering_angle = 30.0f;        // Базовый угол (градусы)

//     // 🆕 НОВЫЕ ПОЛЯ для задержки с шагом угла:
//     float angle_step_deg = 0.5f;         // Шаг по углу (градусы) - СТАНДАРТ 0.5°
//     float angle_start_deg = -60.0f;      // Начальный угол (градусы)
//     float angle_stop_deg = 60.0f;        // Конечный угол (градусы)
//     mutable size_t count_points = 1024*8;  // Количество точек (отсчётов) на луч

//     // ДЛЯ ГЕТЕРОДИНА:
//     bool apply_heterodyne = false;       // Применять ли сопряжение

//     // ВАЛИДАЦИЯ (обновлена)
//     bool IsValid() const noexcept {
//         if(count_points > 0) {
//             duration = static_cast<float>(count_points) / static_cast<float>(sample_rate);
//             // Если задано count_points, то duration игнорируется
//             return f_start > 0.0f && f_stop > f_start &&
//                 sample_rate > 2.0f * f_stop &&
//                 count_points > 0 && num_beams > 0 &&
//                 angle_step_deg > 0.0f;
//         }

//         if(duration > 0.0f) {
//             count_points = static_cast<size_t>(duration * sample_rate);
//             // Если задано duration, то count_points игнорируется
//             return f_start > 0.0f && f_stop > f_start &&
//                 sample_rate > 2.0f * f_stop &&
//                 duration > 0.0f && num_beams > 0 &&
//                 angle_step_deg > 0.0f;
//         }

//         return count_points > 0 && duration > 0.0f &&
//             f_start > 0.0f && f_stop > f_start &&
//             sample_rate > 2.0f * f_stop &&
//             duration > 0.0f && num_beams > 0 &&
//             angle_step_deg > 0.0f;
//     }

//     float GetChirpRate() const noexcept {
//         return (f_stop - f_start) / duration;
//     }

//     size_t GetNumSamples() const noexcept {
//         return static_cast<size_t>(duration * sample_rate);
//     }

//     float GetWavelength() const noexcept {
//         float f_center = (f_start + f_stop) / 2.0f;
//         return SPEED_OF_LIGHT / f_center;
//     }
// };

struct GenerationStatistics {
    double generation_time_ms = 0.0;
    size_t total_samples = 0;
    float peak_amplitude = 0.0f;
    float rms_value = 0.0f;
};

struct NoiseParams {
    double fd;              // sample_rate
    double f0;              // f1 (start frequency)
    double a;               // signal amplitude
    double an;              // noise amplitude
    double ti;              // duration
    double phi = 0;         // initial phase
    double fdev = 0;        // frequency deviation (f2 - f1)
    double tau = 0;         // time shift
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

    // 🆕 ПРИВАТНЫЕ ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ:
    void GenerateVariant_AngleSweep(
        std::complex<float>* beam_data,
        size_t num_samples,
        float angle_deg,
        size_t element_index
    ) const noexcept;

    void GenerateVariant_Heterodyne(
        std::complex<float>* beam_data,
        size_t num_samples
    ) const noexcept;

public:
    // CONSTRUCTORS
    explicit LFMSignalGenerator(const LFMParameters& params)
        : params_(params) {
        if (!params_.IsValid()) {
            throw std::invalid_argument("Invalid LFM parameters");
        }
    }

    explicit LFMSignalGenerator(float f_start, float f_stop, float sample_rate, float duration)
        : LFMSignalGenerator([=]() {
            LFMParameters p;
            p.f_start = f_start;
            p.f_stop = f_stop;
            p.sample_rate = sample_rate;
            p.duration = duration;
            p.num_beams = 256;
            p.steering_angle = 30.0f;
            return p;
        }()) {
    }

    // MOVE SEMANTICS
    LFMSignalGenerator(LFMSignalGenerator&&) = default;
    LFMSignalGenerator& operator=(LFMSignalGenerator&&) = default;

    // DELETE COPY
    LFMSignalGenerator(const LFMSignalGenerator&) = delete;
    LFMSignalGenerator& operator=(const LFMSignalGenerator&) = delete;

    virtual ~LFMSignalGenerator() = default;

    // MAIN API
    SignalBuffer Generate(LFMVariant variant = LFMVariant::BASIC);

    ErrorCode GenerateIntoBuffer(SignalBuffer& buffer, LFMVariant variant = LFMVariant::BASIC);

    // SINGLE BEAM GENERATION
    void GenerateBeam(std::complex<float>* beam_data, size_t num_samples,
        LFMVariant variant, float beam_param = 0.0f) const;

    // GETTERS
    const LFMParameters& GetParameters() const noexcept { return params_; }

    const GenerationStatistics& GetStatistics() const noexcept { return stats_; }

    // NEW: Generate signal with noise (vectorized, no loops)
    std::pair<std::vector<std::complex<float>>, std::vector<double>>
    GetSignalWithNoise(const NoiseParams& params);

    // 🆕 НОВЫЙ МЕТОД 1: Генерация с задержкой по углам (0.5° шаг)
    float ComputeDelayForAngle(
        float angle_deg,      // Угол в градусах
        size_t element_index  // Индекс элемента (0, 1, 2, ...)
    ) const noexcept;

    // 🆕 НОВЫЙ МЕТОД 2: Создать сопряжённую копию буфера (гетеродин)
    SignalBuffer MakeConjugateCopy(const SignalBuffer& src) const;

    // 🆕 НОВЫЙ МЕТОД 3: In-place сопряжение (экономит память)
    void ConjugateInPlace(SignalBuffer& buffer) const noexcept;

    // 🆕 НОВЫЙ МЕТОД 4: Гетеродинирование (умножение двух сигналов)
    // Результат: y[n] = x[n] * h[n], где h[n] = сопряжённый опорный сигнал
    SignalBuffer Heterodyne(
        const SignalBuffer& rx_signal,  // Принятый сигнал
        const SignalBuffer& ref_signal  // Опорный сигнал (ЛЧМ)
    ) const;
};

std::ostream& operator<<(std::ostream& os, const LFMParameters& params);
std::ostream& operator<<(std::ostream& os, const GenerationStatistics& stats);

} // namespace radar
