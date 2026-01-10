#pragma once

#include "profiling_engine.h"
#include "gpu_profiling.h"
#include <map>

namespace radar {

class Reporter {
public:
    Reporter() = default;
    ~Reporter() = default;

    bool SaveProfiling(const ProfilingEngine& profiler, const std::string& json_filename) const;

    bool SaveDetailedGPU(const DetailedGPUProfiling& gpu_prof, const std::map<std::string, std::string>& signal_params,
                         const std::string& json_filename, const std::string& md_filename) const;
};

} // namespace radar
