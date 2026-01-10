#ifndef FRACTIONAL_DELAY_CPU_H
#define FRACTIONAL_DELAY_CPU_H

#include "signal_buffer.h"
#include "lagrange_matrix.h"
#include <cstddef>

/**
 * @brief Выполнить дробную задержку сигнала с интерполяцией Лагранжа на CPU
 * 
 * Реализует тот же алгоритм, что и GPU kernel (kernel_fractional_delay.cl).
 * Использует матрицу Лагранжа 48×5 для интерполяции 5-го порядка.
 * 
 * @param input_output Буфер сигналов (in-place обработка)
 * @param lagrange_matrix Указатель на матрицу Лагранжа 48×5
 * @param delay_coefficients Массив коэффициентов задержки для каждого луча
 * @param num_beams Количество лучей
 * @param num_samples Количество отсчётов на луч
 * @return true если успешно, false при ошибке
 */
bool ExecuteFractionalDelayCPU(
    SignalBuffer* input_output,
    const LagrangeMatrix* lagrange_matrix,
    const float* delay_coefficients,
    size_t num_beams,
    size_t num_samples
);

#endif // FRACTIONAL_DELAY_CPU_H

