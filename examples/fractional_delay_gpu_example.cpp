#include "fractional_delay_gpu.h"
#include "gpu_backend/gpu_factory.h"
#include "signal_buffer.h"
#include "profiling_engine.h"
#include "gpu_profiling.h"
#include "lagrange_matrix.h"
#include <iostream>
#include <vector>

using namespace radar;

int main() {
    std::cout << "=== Fractional Delay GPU Example ===" << std::endl;
    
    // Create GPU backend using factory
    auto backend = GPUFactory::CreateBackend();
    if (!backend) {
        std::cerr << "Failed to create GPU backend" << std::endl;
        return 1;
    }
    
    // Initialize backend
    if (!backend->Initialize()) {
        std::cerr << "Failed to initialize GPU backend" << std::endl;
        return 1;
    }
    
    // Create fractional delay GPU processor
    FractionalDelayGPU fractional_delay(backend.get());
    
    if (!fractional_delay.IsInitialized()) {
        std::cerr << "Failed to initialize FractionalDelayGPU" << std::endl;
        return 1;
    }
    
    // Get system info
    auto system_info = fractional_delay.GetSystemInfo();
    std::cout << "GPU Device: " << system_info.device_name << std::endl;
    std::cout << "GPU Memory: " << system_info.device_memory_mb << " MB" << std::endl;
    
    // Create test signal buffer
    const size_t num_beams = 4;
    const size_t num_samples = 1024;
    SignalBuffer input(num_beams, num_samples);
    SignalBuffer output(num_beams, num_samples);
    
    // Fill input with test data
    for (size_t b = 0; b < num_beams; ++b) {
        auto* beam_data = input.GetBeamData(b);
        for (size_t s = 0; s < num_samples; ++s) {
            beam_data[s] = std::complex<float>(static_cast<float>(s), static_cast<float>(b));
        }
    }
    
    // Create delay coefficients (one per beam)
    std::vector<float> delay_coeffs(num_beams);
    for (size_t b = 0; b < num_beams; ++b) {
        delay_coeffs[b] = static_cast<float>(b) * 0.5f; // Different delays for each beam
    }
    
    // Load Lagrange matrix for fractional delay
    LagrangeMatrix lagrange;
    if (!lagrange.LoadFromFile("lagrange_matrix_48x5.json")) {
        std::cerr << "Warning: Failed to load Lagrange matrix, using default" << std::endl;
        // Use default matrix if file not found
        lagrange.GenerateDefaultMatrix();
    }
    
    // Upload Lagrange matrix to GPU
    if (!fractional_delay.UploadLagrangeMatrix(lagrange.GetData())) {
        std::cerr << "Warning: Failed to upload Lagrange matrix" << std::endl;
    }
    
    // Create profiling engine
    ProfilingEngine profiling;
    profiling.EnableProfiling(true);
    
    // Process fractional delay with profiling
    std::cout << "\nProcessing fractional delay on GPU..." << std::endl;
    if (!fractional_delay.ProcessFractionalDelay(input, delay_coeffs.data(), output, &profiling)) {
        std::cerr << "Failed to process fractional delay" << std::endl;
        return 1;
    }
    
    // Print profiling results
    std::cout << "\nProfiling Results:" << std::endl;
    profiling.ReportMetrics();
    
    // Save profiling to JSON
    if (!profiling.SaveReportToJson("profile_report_gpu_example.json")) {
        std::cerr << "Warning: Failed to save profiling report" << std::endl;
    }
    
    // Test detailed profiling
    std::cout << "\nTesting detailed profiling..." << std::endl;
    DetailedGPUProfiling detailed_profiling;
    if (!fractional_delay.ProcessFractionalDelayWithDetailedProfiling(
        input, delay_coeffs.data(), output, detailed_profiling)) {
        std::cerr << "Failed to process fractional delay with detailed profiling" << std::endl;
        return 1;
    }
    
    // Save detailed profiling
    std::map<std::string, std::string> signal_params;
    signal_params["num_beams"] = std::to_string(num_beams);
    signal_params["num_samples"] = std::to_string(num_samples);
    signal_params["backend"] = backend->GetBackendName();
    signal_params["device"] = backend->GetDeviceName();
    
    if (!SaveDetailedGPUProfilingToMarkdown(
        detailed_profiling, signal_params, "detailed_profiling_report.md")) {
        std::cerr << "Warning: Failed to save detailed profiling report" << std::endl;
    }
    
    std::cout << "\n=== Example completed successfully ===" << std::endl;
    std::cout << "Total GPU time: " << detailed_profiling.total_gpu_time_ms << " ms" << std::endl;
    
    return 0;
}