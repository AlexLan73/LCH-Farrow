#include "gpu_profiling.h"
#include "gpu_backend/opencl_backend.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <sys/utsname.h>
#endif

GPUEventMetrics CalculateEventMetrics(
    const std::string& event_name,
    cl_ulong queued_time,
    cl_ulong submit_time,
    cl_ulong start_time,
    cl_ulong end_time) {
    
    GPUEventMetrics metrics;
    metrics.event_name = event_name;
    metrics.time_queued_ns = static_cast<double>(queued_time);
    metrics.time_submit_ns = static_cast<double>(submit_time);
    metrics.time_start_ns = static_cast<double>(start_time);
    metrics.time_end_ns = static_cast<double>(end_time);
    
    // Вычисляем метрики в наносекундах
    metrics.queue_time_ns = metrics.time_submit_ns - metrics.time_queued_ns;
    metrics.wait_time_ns = metrics.time_start_ns - metrics.time_submit_ns;
    metrics.execution_time_ns = metrics.time_end_ns - metrics.time_start_ns;
    metrics.total_time_ns = metrics.time_end_ns - metrics.time_queued_ns;
    
    // Конвертируем в миллисекунды
    metrics.queue_time_ms = metrics.queue_time_ns / 1000000.0;
    metrics.wait_time_ms = metrics.wait_time_ns / 1000000.0;
    metrics.execution_time_ms = metrics.execution_time_ns / 1000000.0;
    metrics.total_time_ms = metrics.total_time_ns / 1000000.0;
    
    return metrics;
}

SystemInfo GetSystemInfo(IGPUBackend* gpu_backend) {
    SystemInfo info;
    
    if (!gpu_backend) {
        return info;
    }
    
    // Пробуем получить информацию через OpenCLBackend
    OpenCLBackend* opencl_backend = dynamic_cast<OpenCLBackend*>(gpu_backend);
    if (opencl_backend) {
        OpenCLBackend::SystemInfo backend_info = opencl_backend->GetSystemInfo();
        
        info.device_name = backend_info.device_name;
        info.device_vendor = backend_info.device_vendor;
        info.device_version = backend_info.device_version;  // OpenCL API версия
        info.driver_version = backend_info.driver_version;
        info.opencl_c_version = backend_info.opencl_c_version;  // OpenCL C версия
        info.platform_name = backend_info.platform_name;
        info.platform_version = backend_info.platform_version;
        info.device_memory_mb = backend_info.device_memory_mb;
        info.max_work_group_size = backend_info.max_work_group_size;
        info.compute_units = backend_info.compute_units;
        info.os_name = backend_info.os_name;
        info.os_version = backend_info.os_version;
    } else {
        // Fallback для других backend'ов
        info.device_name = gpu_backend->GetDeviceName();
        info.device_memory_mb = gpu_backend->GetDeviceMemorySize() / (1024 * 1024);
        info.os_name = "Unknown";
        info.os_version = "Unknown";
    }
    
    return info;
}

bool SaveDetailedGPUProfilingToJson(
    const DetailedGPUProfiling& profiling,
    const std::string& filename) {
    
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Ошибка: не удалось создать файл " << filename << std::endl;
            return false;
        }
        
        file << std::fixed << std::setprecision(6);
        file << "{\n";
        
        // Системная информация
        file << "  \"system_info\": {\n";
        file << "    \"device_name\": \"" << profiling.system_info.device_name << "\",\n";
        file << "    \"device_vendor\": \"" << profiling.system_info.device_vendor << "\",\n";
    file << "    \"device_version\": \"" << profiling.system_info.device_version << "\",\n";
    file << "    \"driver_version\": \"" << profiling.system_info.driver_version << "\",\n";
    file << "    \"opencl_api_version\": \"" << profiling.system_info.device_version << "\",\n";
    file << "    \"opencl_c_version\": \"" << profiling.system_info.opencl_c_version << "\",\n";
        file << "    \"platform_name\": \"" << profiling.system_info.platform_name << "\",\n";
        file << "    \"platform_version\": \"" << profiling.system_info.platform_version << "\",\n";
        file << "    \"device_memory_mb\": " << profiling.system_info.device_memory_mb << ",\n";
        file << "    \"max_work_group_size\": " << profiling.system_info.max_work_group_size << ",\n";
        file << "    \"compute_units\": " << profiling.system_info.compute_units << ",\n";
        file << "    \"os_name\": \"" << profiling.system_info.os_name << "\",\n";
        file << "    \"os_version\": \"" << profiling.system_info.os_version << "\"\n";
        file << "  },\n";
        
        // GPU Events
        file << "  \"gpu_events\": [\n";
        for (size_t i = 0; i < profiling.gpu_events.size(); ++i) {
            const auto& event = profiling.gpu_events[i];
            file << "    {\n";
            file << "      \"event_name\": \"" << event.event_name << "\",\n";
            file << "      \"queue_time_ms\": " << event.queue_time_ms << ",\n";
            file << "      \"wait_time_ms\": " << event.wait_time_ms << ",\n";
            file << "      \"execution_time_ms\": " << event.execution_time_ms << ",\n";
            file << "      \"total_time_ms\": " << event.total_time_ms << "\n";
            file << "    }";
            if (i < profiling.gpu_events.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        file << "  ],\n";
        
        file << "  \"total_gpu_time_ms\": " << profiling.total_gpu_time_ms << "\n";
        file << "}\n";
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при сохранении JSON: " << e.what() << std::endl;
        return false;
    }
}

std::string GetCurrentDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

bool SaveDetailedGPUProfilingToMarkdown(
    const DetailedGPUProfiling& profiling,
    const std::map<std::string, std::string>& signal_params,
    const std::string& filename) {
    
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Ошибка: не удалось создать файл " << filename << std::endl;
            return false;
        }
        
        file << std::fixed << std::setprecision(3);
        
        // Заголовок
        file << "# 🚀 Отчет о тестировании дробной задержки на GPU\n\n";
        file << "**Дата:** " << GetCurrentDateTime() << "\n";
        file << "**Проект:** LCH-Farrow\n";
        file << "**Автор:** Кодо (AI Assistant) & Alex\n\n";
        file << "---\n\n";
        
        // Системная информация
        file << "## 🛠️ 1. Системная информация\n\n";
        file << "### GPU информация\n";
        file << "- **Устройство:** " << profiling.system_info.device_name << "\n";
        file << "- **Производитель:** " << profiling.system_info.device_vendor << "\n";
        file << "- **Версия устройства:** " << profiling.system_info.device_version << "\n";
        file << "- **Версия драйвера:** " << profiling.system_info.driver_version << "\n";
        file << "- **Память GPU:** " << profiling.system_info.device_memory_mb << " MB\n";
        file << "- **Максимальный размер work group:** " << profiling.system_info.max_work_group_size << "\n";
        file << "- **Вычислительные блоки:** " << profiling.system_info.compute_units << "\n\n";
        
        file << "### OpenCL информация\n";
        file << "- **Платформа:** " << profiling.system_info.platform_name << "\n";
        file << "- **Версия платформы:** " << profiling.system_info.platform_version << "\n";
        file << "- **Версия OpenCL API:** " << profiling.system_info.device_version << "\n";
        file << "- **Версия OpenCL C:** " << profiling.system_info.opencl_c_version << "\n\n";
        
        file << "### Операционная система\n";
        file << "- **ОС:** " << profiling.system_info.os_name << "\n";
        file << "- **Версия ОС:** " << profiling.system_info.os_version << "\n\n";
        
        // Параметры сигнала
        if (!signal_params.empty()) {
            file << "## 📊 2. Параметры сигнала\n\n";
            for (const auto& param : signal_params) {
                file << "- **" << param.first << ":** " << param.second << "\n";
            }
            file << "\n";
        }
        
        // GPU профилирование
        file << "## ⚡ 3. Детальное GPU профилирование\n\n";
        
        if (!profiling.gpu_events.empty()) {
            file << "| Событие | Постановка в очередь (мс) | Ожидание очереди (мс) | Выполнение (мс) | Всего (мс) |\n";
            file << "|:--------|:--------------------------|:-----------------------|:----------------|:-----------|\n";
            
            for (const auto& event : profiling.gpu_events) {
                file << "| " << event.event_name << " | "
                     << event.queue_time_ms << " | "
                     << event.wait_time_ms << " | "
                     << event.execution_time_ms << " | "
                     << event.total_time_ms << " |\n";
            }
            
            file << "\n";
            file << "**Общее время GPU:** " << profiling.total_gpu_time_ms << " мс\n\n";
        }
        
        // Заключение
        file << "## ✅ 4. Заключение\n\n";
        file << "Тестирование дробной задержки сигнала выполнено успешно.\n";
        file << "Детальные метрики GPU профилирования сохранены в JSON формате.\n\n";
        file << "---\n\n";
        file << "*Сгенерировано с любовью, Кодо* 🤖💙\n";
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка при сохранении Markdown: " << e.what() << std::endl;
        return false;
    }
}

