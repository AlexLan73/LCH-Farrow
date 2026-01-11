// include/gpu_kernel.h
#pragma once

#include <cstddef>

namespace radar {

// Функция для выполнения дробной задержки на CUDA
bool ExecuteFractionalDelayCUDA(
    void* device_buffer,
    const float* delay_coeffs,
    size_t num_beams,
    size_t num_samples
);

} // namespace radar