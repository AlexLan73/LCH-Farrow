#include <iostream>
#include <vector>
#include <complex>
#include <iomanip>
#include "lfm_signal_generator_p3.h"

// ═════════════════════════════════════════════════════════════════════
// EXAMPLE: POINT 3 - DATA PREPARATION FOR GPU
// ═════════════════════════════════════════════════════════════════════

int main() {
    using namespace radar;
    
    std::cout << "═════════════════════════════════════════════════════════\n";
    std::cout << "POINT 3: LFM ANGLE ARRAY GENERATION (DATA PREPARATION)\n";
    std::cout << "═════════════════════════════════════════════════════════\n\n";
    
    // ─────────────────────────────────────────────────────────────────
    // 1. SETUP PARAMETERS
    // ─────────────────────────────────────────────────────────────────
    
    AngleArrayParams params;
    params.f_start = 100.0f;           // Start freq: 100 Hz
    params.f_stop = 500.0f;            // Stop freq: 500 Hz
    params.sample_rate = 8000.0f;      // Sample rate: 8 kHz
    params.duration = 1.0f;            // Duration: 1 sec
    
    // ANGLE ARRAY (PARAMETERIZED)
    params.angle_start_deg = -10.0f;   // Start: -10°
    params.angle_stop_deg = 10.0f;     // Stop: +10°
    params.angle_step_deg = 0.5f;      // Step: 0.5° = 41 angles
    
    // LAGRANGE INTERPOLATION
    params.lagrange_order = 48;        // Order: 48
    params.lagrange_row = 5;           // Row: 5
    
    std::cout << "Parameters:\n";
    std::cout << "  LFM: " << params.f_start << "-" << params.f_stop << " Hz\n";
    std::cout << "  Sample rate: " << params.sample_rate << " Hz\n";
    std::cout << "  Duration: " << params.duration << " sec\n";
    std::cout << "  Num samples: " << params.GetNumSamples() << "\n";
    std::cout << "  Angles: " << params.angle_start_deg << "° to " << params.angle_stop_deg 
              << "° (step " << params.angle_step_deg << "°)\n";
    std::cout << "  Num angles: " << params.GetNumAngles() << "\n";
    std::cout << "  Lagrange: order=" << params.lagrange_order 
              << ", row=" << params.lagrange_row << "\n\n";
    
    // ─────────────────────────────────────────────────────────────────
    // 2. VALIDATE
    // ─────────────────────────────────────────────────────────────────
    
    if (!params.IsValid()) {
        std::cerr << "ERROR: Invalid parameters!\n";
        return 1;
    }
    
    std::cout << "✓ Parameters validated\n\n";
    
    // ─────────────────────────────────────────────────────────────────
    // 3. CREATE GENERATOR
    // ─────────────────────────────────────────────────────────────────
    
    try {
        LFMSignalGenerator gen(params);
        std::cout << "✓ Generator created\n\n";
        
        // ─────────────────────────────────────────────────────────────
        // 4. GENERATE ANGLE ARRAY (POINT 3)
        // ─────────────────────────────────────────────────────────────
        
        std::cout << "Generating angle array with fractional delays...\n";
        gen.GenerateAngleArray();
        std::cout << "✓ Angle array generated\n\n";
        
        // ─────────────────────────────────────────────────────────────
        // 5. VERIFY DATA
        // ─────────────────────────────────────────────────────────────
        
        std::cout << "Data structure:\n";
        std::cout << "  m_signal_conjugate[num_angles][num_samples]\n";
        std::cout << "  Size: " << gen.GetNumAngles() << " × " 
                  << gen.GetNumSamples() << " = " 
                  << gen.GetNumAngles() * gen.GetNumSamples() << " complex floats\n";
        std::cout << "  Memory: " << (gen.GetDataSizeBytes() / 1024.0) << " KB\n\n";
        
        // ─────────────────────────────────────────────────────────────
        // 6. SHOW SAMPLE DATA
        // ─────────────────────────────────────────────────────────────
        
        std::cout << "Sample data (first 5 angles, first 3 samples):\n";
        std::cout << "┌─────────┬────────────────────────────────────────┐\n";
        std::cout << "│ Angle   │ Sample 0      Sample 1      Sample 2  │\n";
        std::cout << "├─────────┼────────────────────────────────────────┤\n";
        
        for (size_t a = 0; a < std::min(size_t(5), gen.GetNumAngles()); ++a) {
            float angle = gen.GetAngleForIndex(a);
            auto signal = gen.GetSignal(a);
            
            std::cout << "│ " << std::fixed << std::setw(6) << std::setprecision(1) 
                      << angle << "° │";
            
            for (size_t s = 0; s < 3 && s < signal->size(); ++s) {
                auto val = (*signal)[s];
                std::cout << " " << std::setprecision(2)
                         << val.real() << "+j" << val.imag() << "  ";
            }
            std::cout << "│\n";
        }
        
        std::cout << "└─────────┴────────────────────────────────────────┘\n\n";
        
        // ─────────────────────────────────────────────────────────────
        // 7. GPU TRANSFER SIMULATION
        // ─────────────────────────────────────────────────────────────
        
        std::cout << "GPU Transfer Simulation:\n";
        auto raw_data = gen.GetRawData();
        size_t size_bytes = gen.GetDataSizeBytes();
        
        if (raw_data && size_bytes > 0) {
            std::cout << "  ✓ Raw pointer ready: " << raw_data << "\n";
            std::cout << "  ✓ Data size: " << size_bytes << " bytes\n";
            std::cout << "  ✓ Ready for: cudaMemcpy(..., " << size_bytes << " bytes)\n";
        } else {
            std::cout << "  ✗ ERROR: No data!\n";
            return 1;
        }
        
        std::cout << "\n";
        
        // ─────────────────────────────────────────────────────────────
        // 8. SUMMARY
        // ─────────────────────────────────────────────────────────────
        
        std::cout << "═════════════════════════════════════════════════════════\n";
        std::cout << "SUMMARY:\n";
        std::cout << "  ✓ m_signal_conjugate[41][8000] created\n";
        std::cout << "  ✓ Each signal: conjugated complex LFM with fractional delay\n";
        std::cout << "  ✓ Lagrange order: 48, row: 5\n";
        std::cout << "  ✓ Memory allocated: " << (size_bytes / 1024.0) << " KB\n";
        std::cout << "  ✓ Ready for GPU transfer\n";
        std::cout << "═════════════════════════════════════════════════════════\n";
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

/*
 * ═════════════════════════════════════════════════════════════════════
 * COMPILATION:
 * ═════════════════════════════════════════════════════════════════════
 * 
 *   g++ -std=c++17 -O2 -o point3_example \
 *       lfm_signal_generator_p3.cpp \
 *       main_p3.cpp
 * 
 * RUN:
 * 
 *   ./point3_example
 * 
 * ═════════════════════════════════════════════════════════════════════
 */
