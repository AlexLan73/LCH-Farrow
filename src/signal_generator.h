#pragma once

#include "signal_buffer.h"
#include "lfm_signal_generator.h"

namespace radar {

class SignalGenerator {
public:
    explicit SignalGenerator(const LFMParameters& params);
    ~SignalGenerator();

    // Заполнить SignalBuffer сигналами согласно варианту генерации
    bool Generate(SignalBuffer& out_buffer, LFMVariant variant);

private:
    LFMParameters params_;
    LFMSignalGenerator generator_;
};

} // namespace radar
