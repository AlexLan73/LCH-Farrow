#ifndef GPU_PROFILING_H
#define GPU_PROFILING_H

#include <string>
#include <map>
#include <vector>
#include <CL/cl.hpp>

// Forward declaration
class IGPUBackend;

/**
 * @brief Структура для детальных GPU метрик через OpenCL Events
 */
struct GPUEventMetrics {
    std::string event_name;        // Имя события (H2D, Kernel, D2H)
    double time_queued_ns;         // Время постановки в очередь (ns)
    double time_submit_ns;         // Время постановки в submit (ns)
    double time_start_ns;          // Время начала выполнения (ns)
    double time_end_ns;            // Время окончания (ns)
    
    // Вычисленные метрики
    double queue_time_ns;          // SUBMIT - QUEUED (ожидание постановки)
    double wait_time_ns;           // START - SUBMIT (ожидание очереди)
    double execution_time_ns;      // END - START (время выполнения)
    double total_time_ns;          // END - QUEUED (общее время)
    
    // В миллисекундах для удобства
    double queue_time_ms;
    double wait_time_ms;
    double execution_time_ms;
    double total_time_ms;
    
    GPUEventMetrics() 
        : time_queued_ns(0.0), time_submit_ns(0.0), 
          time_start_ns(0.0), time_end_ns(0.0),
          queue_time_ns(0.0), wait_time_ns(0.0),
          execution_time_ns(0.0), total_time_ns(0.0),
          queue_time_ms(0.0), wait_time_ms(0.0),
          execution_time_ms(0.0), total_time_ms(0.0) {}
};

/**
 * @brief Структура для системной информации
 */
struct SystemInfo {
    // GPU информация
    std::string device_name;
    std::string device_vendor;
    std::string device_version;        // OpenCL API версия устройства
    std::string driver_version;
    std::string opencl_c_version;      // OpenCL C язык версия
    std::string platform_name;
    std::string platform_version;
    size_t device_memory_mb;
    size_t max_work_group_size;
    size_t compute_units;
    
    // Операционная система
    std::string os_name;
    std::string os_version;
    
    SystemInfo() : device_memory_mb(0), max_work_group_size(0), compute_units(0) {}
};

/**
 * @brief Структура для детального GPU профилирования
 */
struct DetailedGPUProfiling {
    SystemInfo system_info;
    std::vector<GPUEventMetrics> gpu_events;
    double total_gpu_time_ms;
    
    DetailedGPUProfiling() : total_gpu_time_ms(0.0) {}
};

/**
 * @brief Функции для работы с GPU профилированием
 */

/**
 * @brief Получить системную информацию от OpenCL backend
 * @param gpu_backend Указатель на GPU backend
 * @return Структура с системной информацией
 */
SystemInfo GetSystemInfo(IGPUBackend* gpu_backend);

/**
 * @brief Вычислить метрики из временных меток OpenCL Event
 */
GPUEventMetrics CalculateEventMetrics(
    const std::string& event_name,
    cl_ulong queued_time,
    cl_ulong submit_time,
    cl_ulong start_time,
    cl_ulong end_time
);

/**
 * @brief Сохранить детальный GPU профилинг в JSON
 */
bool SaveDetailedGPUProfilingToJson(
    const DetailedGPUProfiling& profiling,
    const std::string& filename
);

/**
 * @brief Сохранить детальный GPU профилинг в Markdown
 */
bool SaveDetailedGPUProfilingToMarkdown(
    const DetailedGPUProfiling& profiling,
    const std::map<std::string, std::string>& signal_params,
    const std::string& filename
);

#endif // GPU_PROFILING_H

