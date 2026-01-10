#include "profiling_engine.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

ProfilingEngine::ProfilingEngine()
    : profiling_enabled_(true) {
}

void ProfilingEngine::StartTimer(const std::string& name) {
    if (!profiling_enabled_) {
        return;
    }
    
    start_times_[name] = std::chrono::high_resolution_clock::now();
}

void ProfilingEngine::StopTimer(const std::string& name) {
    if (!profiling_enabled_) {
        return;
    }
    
    auto it = start_times_.find(name);
    if (it == start_times_.end()) {
        std::cerr << "Предупреждение: таймер '" << name << "' не был запущен" << std::endl;
        return;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - it->second
    ).count();
    
    double time_ms = static_cast<double>(duration) / 1000.0;
    UpdateMetricStats(name, time_ms);
    
    start_times_.erase(it);
}

void ProfilingEngine::RecordGpuEvent(const std::string& event_name, double time_ms) {
    if (!profiling_enabled_) {
        return;
    }
    
    UpdateMetricStats(event_name, time_ms);
}

void ProfilingEngine::ReportMetrics() const {
    if (metrics_.metrics.empty()) {
        std::cout << "Нет метрик для отчёта" << std::endl;
        return;
    }
    
    std::cout << "\n========================================\n";
    std::cout << "ОТЧЁТ О ПРОИЗВОДИТЕЛЬНОСТИ\n";
    std::cout << "========================================\n\n";
    
    std::cout << std::left << std::setw(30) << "Операция"
              << std::right << std::setw(12) << "Время (мс)"
              << std::setw(12) << "Вызовов"
              << std::setw(12) << "Мин (мс)"
              << std::setw(12) << "Макс (мс)"
              << std::setw(12) << "Сред (мс)" << std::endl;
    std::cout << std::string(90, '-') << std::endl;
    
    double total = 0.0;
    for (const auto& pair : metrics_.metrics) {
        const auto& metric = pair.second;
        std::cout << std::left << std::setw(30) << metric.name
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(12) << metric.time_ms
                  << std::setw(12) << metric.call_count
                  << std::setw(12) << metric.min_time_ms
                  << std::setw(12) << metric.max_time_ms
                  << std::setw(12) << metric.avg_time_ms << std::endl;
        total += metric.time_ms;
    }
    
    std::cout << std::string(90, '-') << std::endl;
    std::cout << std::left << std::setw(30) << "ИТОГО"
              << std::right << std::fixed << std::setprecision(3)
              << std::setw(12) << total << std::endl;
    std::cout << "========================================\n" << std::endl;
}

bool ProfilingEngine::SaveReportToJson(const std::string& filename) const {
    // Создаём директорию если не существует
    size_t last_slash = filename.find_last_of("/");
    if (last_slash != std::string::npos) {
        std::string dir = filename.substr(0, last_slash);
        std::string cmd = "mkdir -p " + dir;
        system(cmd.c_str());
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл " << filename << std::endl;
        return false;
    }
    
    file << "{\n";
    file << "  \"metrics\": [\n";
    
    bool first = true;
    double total = 0.0;
    for (const auto& pair : metrics_.metrics) {
        if (!first) {
            file << ",\n";
        }
        first = false;
        
        const auto& metric = pair.second;
        file << "    {\n";
        file << "      \"name\": \"" << metric.name << "\",\n";
        file << "      \"time_ms\": " << std::fixed << std::setprecision(6) << metric.time_ms << ",\n";
        file << "      \"call_count\": " << metric.call_count << ",\n";
        file << "      \"min_time_ms\": " << metric.min_time_ms << ",\n";
        file << "      \"max_time_ms\": " << metric.max_time_ms << ",\n";
        file << "      \"avg_time_ms\": " << metric.avg_time_ms << "\n";
        file << "    }";
        total += metric.time_ms;
    }
    
    file << "\n  ],\n";
    file << "  \"total_time_ms\": " << std::fixed << std::setprecision(6) << total << "\n";
    file << "}\n";
    
    file.close();
    return true;
}

const TimingMetric& ProfilingEngine::GetMetric(const std::string& name) const {
    static TimingMetric empty_metric;
    auto it = metrics_.metrics.find(name);
    if (it != metrics_.metrics.end()) {
        return it->second;
    }
    return empty_metric;
}

void ProfilingEngine::Reset() {
    metrics_.metrics.clear();
    metrics_.total_time_ms = 0.0;
    start_times_.clear();
}

void ProfilingEngine::UpdateMetricStats(const std::string& name, double time_ms) {
    auto& metric = metrics_.metrics[name];
    metric.name = name;
    metric.time_ms += time_ms;
    metric.call_count++;
    
    if (metric.call_count == 1) {
        metric.min_time_ms = time_ms;
        metric.max_time_ms = time_ms;
    } else {
        metric.min_time_ms = std::min(metric.min_time_ms, time_ms);
        metric.max_time_ms = std::max(metric.max_time_ms, time_ms);
    }
    
    metric.avg_time_ms = metric.time_ms / static_cast<double>(metric.call_count);
    metrics_.total_time_ms += time_ms;
}

