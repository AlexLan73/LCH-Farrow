#include "result_comparator.h"
#include <iostream>
#include <cmath>
#include <algorithm>

bool CompareResults(
    const SignalBuffer* cpu_results,
    const SignalBuffer* gpu_results,
    float tolerance,
    ComparisonMetrics* metrics) {
    
    if (!cpu_results || !gpu_results) {
        std::cerr << "Ошибка: неверные параметры для CompareResults" << std::endl;
        return false;
    }
    
    if (cpu_results->GetNumBeams() != gpu_results->GetNumBeams() ||
        cpu_results->GetNumSamples() != gpu_results->GetNumSamples()) {
        std::cerr << "Ошибка: несоответствие размеров буферов для сравнения" << std::endl;
        return false;
    }
    
    if (!cpu_results->IsValid() || !gpu_results->IsValid()) {
        std::cerr << "Ошибка: один из буферов не валиден" << std::endl;
        return false;
    }
    
    size_t num_beams = cpu_results->GetNumBeams();
    size_t num_samples = cpu_results->GetNumSamples();
    
    // Инициализация метрик
    ComparisonMetrics local_metrics;
    if (metrics) {
        *metrics = ComparisonMetrics();  // Сброс метрик
    }
    
    float max_diff_real = 0.0f;
    float max_diff_imag = 0.0f;
    float max_diff_magnitude = 0.0f;
    float sum_diff_magnitude = 0.0f;
    float max_relative_error = 0.0f;
    size_t errors_count = 0;
    size_t total_points = num_beams * num_samples;
    
    // Сравнение по точкам
    for (size_t beam = 0; beam < num_beams; ++beam) {
        const SignalBuffer::ComplexType* cpu_data = cpu_results->GetBeamData(beam);
        const SignalBuffer::ComplexType* gpu_data = gpu_results->GetBeamData(beam);
        
        if (!cpu_data || !gpu_data) {
            std::cerr << "Ошибка: не удалось получить данные для луча " << beam << std::endl;
            return false;
        }
        
        for (size_t sample = 0; sample < num_samples; ++sample) {
            SignalBuffer::ComplexType cpu_val = cpu_data[sample];
            SignalBuffer::ComplexType gpu_val = gpu_data[sample];
            
            // Вычисляем разницу
            float diff_real = std::abs(cpu_val.real() - gpu_val.real());
            float diff_imag = std::abs(cpu_val.imag() - gpu_val.imag());
            
            // Максимальная разница по компонентам
            max_diff_real = std::max(max_diff_real, diff_real);
            max_diff_imag = std::max(max_diff_imag, diff_imag);
            
            // Вычисляем разницу по модулю
            float diff_magnitude = std::abs(cpu_val - gpu_val);
            max_diff_magnitude = std::max(max_diff_magnitude, diff_magnitude);
            sum_diff_magnitude += diff_magnitude;
            
            // Вычисляем относительную ошибку
            float cpu_magnitude = std::abs(cpu_val);
            if (cpu_magnitude > 1e-10f) {  // Избегаем деления на ноль
                float relative_error = diff_magnitude / cpu_magnitude;
                max_relative_error = std::max(max_relative_error, relative_error);
            }
            
            // Проверяем превышение tolerance
            if (diff_magnitude > tolerance) {
                errors_count++;
            }
        }
    }
    
    // Вычисляем среднюю разницу по модулю
    float avg_diff_magnitude = (total_points > 0) ? (sum_diff_magnitude / static_cast<float>(total_points)) : 0.0f;
    
    // Заполняем метрики
    local_metrics.max_diff_real = max_diff_real;
    local_metrics.max_diff_imag = max_diff_imag;
    local_metrics.max_diff_magnitude = max_diff_magnitude;
    local_metrics.avg_diff_magnitude = avg_diff_magnitude;
    local_metrics.max_relative_error = max_relative_error;
    local_metrics.errors_above_tolerance = errors_count;
    local_metrics.total_points = total_points;
    
    // Копируем метрики в выходную структуру
    if (metrics) {
        *metrics = local_metrics;
    }
    
    return true;
}

