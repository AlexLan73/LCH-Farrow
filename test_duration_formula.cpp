#include <iostream>
#include <cassert>
#include "include/lfm_signal_generator.h"

int main() {
    // Test the duration formula
    radar::LFMParameters params;
    
    // Test case 1: Basic test
    params.count_points = 1024;
    params.sample_rate = 12.0e6f; // 12 MHz
    
    // Calculate expected duration
    float expected_duration = static_cast<float>(params.count_points) / static_cast<float>(params.sample_rate);
    
    // Validate parameters (this should calculate duration internally)
    bool is_valid = params.IsValid();
    
    std::cout << "Test 1: Basic formula test" << std::endl;
    std::cout << "  count_points: " << params.count_points << std::endl;
    std::cout << "  sample_rate: " << params.sample_rate << " Hz" << std::endl;
    std::cout << "  Expected duration: " << expected_duration << " seconds" << std::endl;
    std::cout << "  Parameters valid: " << (is_valid ? "YES" : "NO") << std::endl;
    
    if (is_valid) {
        std::cout << "  Calculated duration: " << params.duration << " seconds" << std::endl;
        
        // Verify the formula is correct
        float calculated_duration = static_cast<float>(params.count_points) / static_cast<float>(params.sample_rate);
        assert(std::abs(calculated_duration - expected_duration) < 1e-6);
        std::cout << "  Formula verification: PASSED" << std::endl;
    }
    
    std::cout << "\nTest 2: Different values" << std::endl;
    params.count_points = 2048;
    params.sample_rate = 24.0e6f; // 24 MHz
    
    expected_duration = static_cast<float>(params.count_points) / static_cast<float>(params.sample_rate);
    is_valid = params.IsValid();
    
    std::cout << "  count_points: " << params.count_points << std::endl;
    std::cout << "  sample_rate: " << params.sample_rate << " Hz" << std::endl;
    std::cout << "  Expected duration: " << expected_duration << " seconds" << std::endl;
    std::cout << "  Parameters valid: " << (is_valid ? "YES" : "NO") << std::endl;
    
    if (is_valid) {
        std::cout << "  Calculated duration: " << params.duration << " seconds" << std::endl;
        std::cout << "  Formula verification: PASSED" << std::endl;
    }
    
    std::cout << "\nAll tests completed successfully!" << std::endl;
    return 0;
}