#ifndef RESULT_COMPARATOR_H
#define RESULT_COMPARATOR_H

#include "signal_buffer.h"
#include <cstddef>

/**
 * @brief Метрики сравнения результатов CPU и GPU
 */
struct ComparisonMetrics {
    float max_diff_real;           // Максимальная разница (real часть)
    float max_diff_imag;           // Максимальная разница (imag часть)
    float max_diff_magnitude;      // Максимальная разница по модулю
    float avg_diff_magnitude;      // Средняя разница по модулю
    float max_relative_error;      // Максимальная относительная ошибка
    size_t errors_above_tolerance; // Количество точек с превышением tolerance
    size_t total_points;           // Всего точек для сравнения
    
    ComparisonMetrics()
        : max_diff_real(0.0f),
          max_diff_imag(0.0f),
          max_diff_magnitude(0.0f),
          avg_diff_magnitude(0.0f),
          max_relative_error(0.0f),
          errors_above_tolerance(0),
          total_points(0) {}
};

/**
 * @brief Сравнить результаты CPU и GPU обработки
 * 
 * @param cpu_results Результаты CPU обработки
 * @param gpu_results Результаты GPU обработки
 * @param tolerance Допустимая погрешность
 * @param metrics Указатель на структуру для метрик сравнения (может быть nullptr)
 * @return true если сравнение успешно, false при ошибке
 */
bool CompareResults(
    const SignalBuffer* cpu_results,
    const SignalBuffer* gpu_results,
    float tolerance,
    ComparisonMetrics* metrics
);

#endif // RESULT_COMPARATOR_H

