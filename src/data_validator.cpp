#include "data_validator.h"
#include <iostream>

namespace radar {

bool DataValidator::ValidateData(
    const SignalBuffer& cpu_results,
    const SignalBuffer& gpu_results,
    float tolerance,
    ComparisonMetrics* metrics
) {
    if (verbose_) {
        std::cout << "DataValidator: Starting validation..." << std::endl;
    }

    // Используем существующий Validator для проверки
    Validator validator;
    bool result = validator.Validate(cpu_results, gpu_results, tolerance, metrics);

    if (verbose_ && metrics) {
        PrintMetrics(*metrics);
    }

    if (verbose_) {
        if (result) {
            std::cout << "DataValidator: Validation completed successfully." << std::endl;
        } else {
            std::cout << "DataValidator: Validation failed." << std::endl;
        }
    }

    return result;
}

bool DataValidator::ValidateWithExternalValidator(
    Validator& validator,
    const SignalBuffer& cpu_results,
    const SignalBuffer& gpu_results,
    float tolerance,
    ComparisonMetrics* metrics
) {
    if (verbose_) {
        std::cout << "DataValidator: Starting validation with external validator..." << std::endl;
    }

    bool result = validator.Validate(cpu_results, gpu_results, tolerance, metrics);

    if (verbose_ && metrics) {
        PrintMetrics(*metrics);
    }

    if (verbose_) {
        if (result) {
            std::cout << "DataValidator: External validation completed successfully." << std::endl;
        } else {
            std::cout << "DataValidator: External validation failed." << std::endl;
        }
    }

    return result;
}

void DataValidator::PrintMetrics(const ComparisonMetrics& metrics) const {
    std::cout << "\nValidation Metrics:" << std::endl;
    std::cout << "  Max difference (real): " << metrics.max_diff_real << std::endl;
    std::cout << "  Max difference (imag): " << metrics.max_diff_imag << std::endl;
    std::cout << "  Max difference (magnitude): " << metrics.max_diff_magnitude << std::endl;
    std::cout << "  Average difference (magnitude): " << metrics.avg_diff_magnitude << std::endl;
    std::cout << "  Max relative error: " << metrics.max_relative_error << std::endl;
    std::cout << "  Errors above tolerance: " << metrics.errors_above_tolerance << std::endl;
    std::cout << "  Total points: " << metrics.total_points << std::endl;
}

} // namespace radar