#include "report_manager.h"
#include <iostream>

namespace radar {

bool ReportManager::SaveProfilingReport(const ProfilingEngine& profiler, const std::string& json_filename) const {
    return reporter_.SaveProfiling(profiler, json_filename);
}

bool ReportManager::SaveDetailedGPUReport(const DetailedGPUProfiling& gpu_prof, 
                                         const std::map<std::string, std::string>& signal_params,
                                         const std::string& json_filename, 
                                         const std::string& md_filename) const {
    return reporter_.SaveDetailedGPU(gpu_prof, signal_params, json_filename, md_filename);
}

void ReportManager::PrintReport(const ProfilingEngine& profiler) const {
    profiler.ReportMetrics();
}

} // namespace radar