#include "signal_generator.h"
#include <iostream>

namespace radar {

SignalGenerator::SignalGenerator(const LFMParameters& params)
    : params_(params), generator_(params_) {}

SignalGenerator::~SignalGenerator() = default;

bool SignalGenerator::Generate(SignalBuffer& out_buffer, LFMVariant variant) {
    if (!out_buffer.IsValid()) return false;

    size_t num_beams = out_buffer.GetNumBeams();
    size_t num_samples = out_buffer.GetNumSamples();

    for (size_t beam = 0; beam < num_beams; ++beam) {
        auto* beam_ptr = out_buffer.GetBeamData(beam);
        if (!beam_ptr) return false;
        // LFMSignalGenerator expects complex<float>* and num_samples
        generator_.GenerateBeam(beam_ptr, num_samples, variant, static_cast<float>(beam) * 0.125f);
    }

    std::cout << "SignalGenerator: generated " << num_beams << " beams, " << num_samples << " samples each\n";
    return true;
}

} // namespace radar
