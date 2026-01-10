#pragma once

#include "signal_buffer.h"
#include <memory>

namespace radar {

class GPUProcessor {
public:
    GPUProcessor();
    ~GPUProcessor();

    // Выполнить дробную задержку на GPU: входной буфер -> выходной буфер
    bool ProcessFractionalDelay(const SignalBuffer& input,
                                const float* delay_coeffs,
                                SignalBuffer& output);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace radar
