#pragma once

#include <complex>
#include <vector>

class LFMSignalGenerator
{
private:
    float f_start_;
    float f_stop_;
    float sample_rate_;
    float duration_;
    float chirp_rate_;
  
public:
  LFMSignalGenerator(float f_start, float f_stop, float sample_rate, float duration);
  void GenerateBeam(std::complex<float>* beam_data, size_t num_samples,
                      float phase_offset = 0.0f, float delay_samples = 0.0f);    

  void GenerateAllBeams(std::vector<std::complex<float>*>& beam_data_ptrs,
                          size_t num_samples, size_t num_beams,
                          const std::vector<float>& delays = {});
  ~LFMSignalGenerator();
  
};

