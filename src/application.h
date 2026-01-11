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
        float duration = -1.0f;
        size_t num_beams = 128;
        float steering_angle = 30.0f;
        float tolerance = 1e-5f;
        size_t count_points =1024*8;  // Новое поле для количества точек в одном луче

    bool IsValid() {
        if(count_points > 0) {
           duration = static_cast<float>(count_points) /  static_cast<float>(sample_rate);
            // Если задано count_points, то duration игнорируется
            return f_start > 0.0f && f_stop > f_start &&
                   sample_rate > 2.0f * f_stop &&
                   count_points > 0 && num_beams > 0;
        }   

        if(duration > 0.0f) {
           count_points = static_cast<size_t>(duration * sample_rate);
            // Если задано duration, то count_points игнорируется
            return f_start > 0.0f && f_stop > f_start &&
                   sample_rate > 2.0f * f_stop &&
                   duration > 0.0f && num_beams > 0;
        }   

        return count_points > 0 && duration > 0.0f &&
            f_start > 0.0f && f_stop > f_start &&
            sample_rate > 2.0f * f_stop &&
            duration > 0.0f && num_beams > 0;
    }


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
