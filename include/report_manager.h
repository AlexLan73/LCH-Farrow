#pragma once

#include "reporter.h"
#include "profiling_engine.h"
#include <string>

namespace radar {

class ReportManager {
public:
    ReportManager() = default;
    ~ReportManager() = default;

    /**
     * @brief Сохранить отчет о профилировании
     * @param profiler Объект профилировщика
     * @param json_filename Имя JSON файла для сохранения
     * @return true если успешно
     */
    bool SaveProfilingReport(const ProfilingEngine& profiler, const std::string& json_filename) const;

    /**
     * @brief Сохранить детальный отчет о GPU
     * @param gpu_prof Объект профилирования GPU
     * @param signal_params Параметры сигнала
     * @param json_filename Имя JSON файла для сохранения
     * @param md_filename Имя Markdown файла для сохранения
     * @return true если успешно
     */
    bool SaveDetailedGPUReport(const DetailedGPUProfiling& gpu_prof, 
                              const std::map<std::string, std::string>& signal_params,
                              const std::string& json_filename, 
                              const std::string& md_filename) const;

    /**
     * @brief Вывести отчет в консоль
     * @param profiler Объект профилировщика
     */
    void PrintReport(const ProfilingEngine& profiler) const;

private:
    Reporter reporter_;
};

} // namespace radar