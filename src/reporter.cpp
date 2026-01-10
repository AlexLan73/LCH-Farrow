#include "reporter.h"
#include <iostream>
#include <filesystem>
#include "gpu_profiling.h"

namespace radar {

bool Reporter::SaveProfiling(const ProfilingEngine& profiler, const std::string& json_filename) const {
    try {
        std::filesystem::path p = std::filesystem::path(json_filename).parent_path();
        if (!p.empty() && !std::filesystem::exists(p)) {
            std::filesystem::create_directories(p);
        }
    } catch (...) {
        // ignore directory creation errors
    }
    return profiler.SaveReportToJson(json_filename);
}

bool Reporter::SaveDetailedGPU(const DetailedGPUProfiling& gpu_prof, const std::map<std::string, std::string>& signal_params,
                               const std::string& json_filename, const std::string& md_filename) const {
    bool ok1 = SaveDetailedGPUProfilingToJson(gpu_prof, json_filename);
    bool ok2 = SaveDetailedGPUProfilingToMarkdown(gpu_prof, signal_params, md_filename);
    return ok1 && ok2;
}

} // namespace radar
