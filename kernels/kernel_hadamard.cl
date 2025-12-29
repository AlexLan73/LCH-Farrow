/**
 * @file kernel_hadamard.cl
 * @brief OpenCL kernel для поэлементного умножения (Hadamard product)
 * 
 * Выполняет поэлементное умножение каждого луча с опорной FFT.
 * Используется для свёртки в частотной области.
 */

/**
 * @brief Выполнить поэлементное умножение (Hadamard product)
 * 
 * @param beams Буфер лучей [num_beams * num_samples] (in-place)
 *              Каждый элемент - complex<float> (float2: x=real, y=imag)
 * @param reference_fft Опорная FFT [num_samples]
 *                      Каждый элемент - complex<float> (float2: x=real, y=imag)
 * @param num_beams Количество лучей
 * @param num_samples Количество отсчётов на луч
 */
__kernel void hadamard_multiply(
    __global float2* beams,
    __global const float2* reference_fft,
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
    
    // Индексы
    uint beam_idx = beam_id * num_samples + sample_id;
    uint ref_idx = sample_id;
    
    // Получаем значения
    float2 beam_value = beams[beam_idx];
    float2 ref_value = reference_fft[ref_idx];
    
    // Умножение комплексных чисел: (a+bi) * (c+di) = (ac-bd) + (ad+bc)i
    float2 result;
    result.x = beam_value.x * ref_value.x - beam_value.y * ref_value.y;  // real
    result.y = beam_value.x * ref_value.y + beam_value.y * ref_value.x;  // imag
    
    // Записываем результат in-place
    beams[beam_idx] = result;
}

