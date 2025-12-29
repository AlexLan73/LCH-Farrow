/**
 * @file kernel_fractional_delay.cl
 * @brief OpenCL kernel для дробной задержки сигнала
 * 
 * Реализует дробную задержку сигнала с использованием интерполяции.
 * Каждый work item обрабатывает один отсчёт одного луча.
 */

/**
 * @brief Выполнить дробную задержку сигнала
 * 
 * @param input_output Буфер входных/выходных данных [num_beams * num_samples]
 *                      Каждый элемент - complex<float> (float2: x=real, y=imag)
 * @param delay_coefficients Коэффициенты задержки для каждого луча [num_beams]
 * @param num_beams Количество лучей
 * @param num_samples Количество отсчётов на луч
 */
__kernel void fractional_delay(
    __global float2* input_output,
    __global const float* delay_coefficients,
    const uint num_beams,
    const uint num_samples
) {
    uint global_id = get_global_id(0);
    uint total_items = num_beams * num_samples;
    
    if (global_id >= total_items) {
        return;
    }
    
    // Определяем луч и отсчёт
    uint beam_id = global_id / num_samples;
    uint sample_id = global_id % num_samples;
    
    // Получаем коэффициент задержки для этого луча
    float delay = delay_coefficients[beam_id];
    
    // Вычисляем задержанный индекс (с дробной частью)
    float delayed_index = (float)sample_id - delay;
    
    // Простая линейная интерполяция
    uint idx_low = (uint)floor(delayed_index);
    uint idx_high = idx_low + 1;
    float t = delayed_index - (float)idx_low;
    
    // Обработка граничных условий
    if (idx_low >= num_samples) {
        idx_low = num_samples - 1;
    }
    if (idx_high >= num_samples) {
        idx_high = num_samples - 1;
    }
    
    // Индексы в глобальном буфере
    uint idx_current = beam_id * num_samples + sample_id;
    uint idx_delayed_low = beam_id * num_samples + idx_low;
    uint idx_delayed_high = beam_id * num_samples + idx_high;
    
    // Линейная интерполяция комплексных чисел
    float2 sample_low = input_output[idx_delayed_low];
    float2 sample_high = input_output[idx_delayed_high];
    
    float2 result;
    result.x = mix(sample_low.x, sample_high.x, t);  // real
    result.y = mix(sample_low.y, sample_high.y, t);  // imag
    
    // Записываем результат in-place
    input_output[idx_current] = result;
}

