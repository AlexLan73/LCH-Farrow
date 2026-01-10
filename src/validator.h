#pragma once

#include "result_comparator.h"
#include "signal_buffer.h"

namespace radar {

class Validator {
public:
    Validator() = default;
    ~Validator() = default;

    // Выполнить сравнение CPU vs GPU, вернуть true если успешно
    bool Validate(const SignalBuffer& cpu, const SignalBuffer& gpu, float tolerance, ComparisonMetrics* out_metrics);
};

} // namespace radar
