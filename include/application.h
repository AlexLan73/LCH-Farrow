#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>
#include "signal_buffer.h"
#include "profiling_engine.h"
#include "validator.h"
#include "reporter.h"

namespace radar {

class Application {
public:
    struct Config {
        float f_start = 100.0f;
        float f_stop = 500.0f;
        float sample_rate = 500000.0f;
        float duration = 1.0f;
        size_t num_beams = 128;
        float steering_angle = 30.0f;
        float tolerance = 1e-5f;
    };

    explicit Application(const Config& cfg);
    ~Application();

    // Выполняет полную последовательность: генерация, CPU, GPU, сравнение, отчёты
    int Run();

private:
    Config cfg_;

    // Низкоуровневые шаги, инкапсулированы для читаемости и тестируемости
    bool GenerateSignal();
    bool LoadLagrangeMatrix();
    bool RunCpuFractionalDelay();
    bool RunGpuFractionalDelay();
    bool CompareAndReport();

    // Вспомогательные структуры, доступные между шагами
    SignalBuffer signal_buffer_;
    SignalBuffer cpu_signal_buffer_;
    SignalBuffer gpu_signal_buffer_;
    std::vector<float> delay_coeffs_;
    ProfilingEngine profiler_;
    Validator validator_;
    Reporter reporter_;
};

} // namespace radar
