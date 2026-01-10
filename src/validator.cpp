#include "validator.h"
#include "result_comparator.h"
#include <iostream>

namespace radar {

bool Validator::Validate(const SignalBuffer& cpu, const SignalBuffer& gpu, float tolerance, ComparisonMetrics* out_metrics) {
    if (cpu.GetNumBeams() != gpu.GetNumBeams() || cpu.GetNumSamples() != gpu.GetNumSamples()) {
        std::cerr << "Validator: buffer sizes mismatch\n";
        return false;
    }

    if (!CompareResults(&cpu, &gpu, tolerance, out_metrics)) {
        std::cerr << "Validator: CompareResults returned false\n";
        return false;
    }

    return true;
}

} // namespace radar
