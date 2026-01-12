#include "../include/lfm_signal_generator.h"

#include <cmath>
#include <numeric>
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

SignalBuffer LFMSignalGenerator::Generate(LFMVariant variant) {
    auto start_time = std::chrono::high_resolution_clock::now();
    SignalBuffer buffer(params_.num_beams, params_.GetNumSamples());
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

ErrorCode LFMSignalGenerator::GenerateIntoBuffer(SignalBuffer& buffer, LFMVariant variant) {
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

            case LFMVariant::ANGLE_SWEEP: {
                float angle_deg = params_.angle_start_deg +
                    static_cast<float>(beam) * params_.angle_step_deg;
                GenerateVariant_AngleSweep(beam_data, num_samples, angle_deg, beam);
                break;
            }

            case LFMVariant::HETERODYNE: {
                GenerateVariant_Heterodyne(beam_data, num_samples);
                break;
            }

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
    default:
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
    const double chirp_rate = params.fdev / params.ti;

    // 3. Генерация случайного гауссова шума (Box-Muller transform)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dis(0.0, 1.0);

    // 4. Основной расчёт (векторизированный)
    for (int n = 0; n < N; ++n) {
        double tn = t[n];

        if (tn < 0.0 || tn > params.ti) {
            X[n] = 0.0f;
            continue;
        }

        double dt_half = tn - params.ti / 2.0;
        double phase = 2.0 * PI * params.f0 * tn +
            PI * params.fdev / params.ti * (dt_half * dt_half) +
            params.phi;

        float signal_real = static_cast<float>(params.a * cos(phase));
        float signal_imag = static_cast<float>(params.a * sin(phase));

        float noise_real = static_cast<float>(params.an * dis(gen));
        float noise_imag = static_cast<float>(params.an * dis(gen));

        X[n] = std::complex<float>(signal_real + noise_real,
                                   signal_imag + noise_imag);
    }

    return {X, t};
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 1: Вычисление задержки для угла (формула 5 из теории)
// ═══════════════════════════════════════════════════════════════════════════

float LFMSignalGenerator::ComputeDelayForAngle(
    float angle_deg,
    size_t element_index
) const noexcept {
    const float angle_rad = angle_deg * PI / 180.0f;
    const float sin_angle = std::sin(angle_rad);

    float f_center = (params_.f_start + params_.f_stop) / 2.0f;
    float wavelength = SPEED_OF_LIGHT / f_center;

    float element_spacing = wavelength / 2.0f;
    float element_position = static_cast<float>(element_index) * element_spacing;
    float delay_time = (element_position * sin_angle) / SPEED_OF_LIGHT;

    float delay_samples = delay_time * params_.sample_rate;
    return delay_samples;
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 2: Создание сопряжённой копии
// ═══════════════════════════════════════════════════════════════════════════

SignalBuffer LFMSignalGenerator::MakeConjugateCopy(
    const SignalBuffer& src
) const {
    SignalBuffer dst(src.GetNumBeams(), src.GetNumSamples());
    const std::complex<float>* src_data = src.RawData();
    std::complex<float>* dst_data = dst.RawData();
    size_t total_size = src.GetTotalSize();

    for (size_t i = 0; i < total_size; ++i) {
        dst_data[i] = std::conj(src_data[i]);
    }

    return dst;
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 3: Сопряжение на месте (экономит память)
// ═══════════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::ConjugateInPlace(SignalBuffer& buffer) const noexcept {
    std::complex<float>* data = buffer.RawData();
    size_t total_size = buffer.GetTotalSize();
    for (size_t i = 0; i < total_size; ++i) {
        data[i] = std::conj(data[i]);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// МЕТОД 4: Гетеродинирование (комплексное умножение)
// ═══════════════════════════════════════════════════════════════════════════

SignalBuffer LFMSignalGenerator::Heterodyne(
    const SignalBuffer& rx_signal,
    const SignalBuffer& ref_signal
) const {
    if (rx_signal.GetTotalSize() != ref_signal.GetTotalSize()) {
        throw std::invalid_argument(
            "Signals must have same size for heterodyning"
        );
    }

    SignalBuffer result(rx_signal.GetNumBeams(), rx_signal.GetNumSamples());
    const std::complex<float>* rx_data = rx_signal.RawData();
    const std::complex<float>* ref_data = ref_signal.RawData();
    std::complex<float>* out_data = result.RawData();
    size_t total_size = rx_signal.GetTotalSize();

    for (size_t i = 0; i < total_size; ++i) {
        out_data[i] = rx_data[i] * std::conj(ref_data[i]);
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// ПРИВАТНЫЙ МЕТОД: Генерация варианта с задержкой по углам
// ═══════════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::GenerateVariant_AngleSweep(
    std::complex<float>* beam_data,
    size_t num_samples,
    float angle_deg,
    size_t element_index
) const noexcept {
    float delay_samples = ComputeDelayForAngle(angle_deg, element_index);
    GenerateVariant_Delay(beam_data, num_samples, delay_samples);
}

// ═══════════════════════════════════════════════════════════════════════════
// ПРИВАТНЫЙ МЕТОД: Генерация варианта гетеродина (сопряжённый сигнал)
// ═══════════════════════════════════════════════════════════════════════════

void LFMSignalGenerator::GenerateVariant_Heterodyne(
    std::complex<float>* beam_data,
    size_t num_samples
) const noexcept {
    GenerateVariant_Basic(beam_data, num_samples);
    for (size_t i = 0; i < num_samples; ++i) {
        beam_data[i] = std::conj(beam_data[i]);
    }
}

// ═════════════════════════════════════════════════════════════════════
// HELPER: Pretty printing
// ═════════════════════════════════════════════════════════════════════

std::ostream& operator<<(std::ostream& os, const LFMParameters& params) {
    os << "LFM Parameters:\n"
        << " Frequency range: " << params.f_start << " - " << params.f_stop << " Hz\n"
        << " Sample rate: " << params.sample_rate << " Hz\n"
        << " Duration: " << params.duration << " sec\n"
        << " Num beams: " << params.num_beams << "\n"
        << " Chirp rate: " << params.GetChirpRate() << " Hz/sec\n"
        << " Num samples: " << params.GetNumSamples() << "\n"
        << " Wavelength: " << params.GetWavelength() << " m";
    return os;
}

std::ostream& operator<<(std::ostream& os, const GenerationStatistics& stats) {
    os << "Generation Statistics:\n"
        << " Time: " << stats.generation_time_ms << " ms\n"
        << " Total samples: " << stats.total_samples << "\n"
        << " Peak amplitude: " << stats.peak_amplitude << "\n"
        << " RMS value: " << stats.rms_value;
    return os;
}

} // namespace radar
