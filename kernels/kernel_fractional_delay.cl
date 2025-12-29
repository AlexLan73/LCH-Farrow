/**
 * @file kernel_fractional_delay.cl
 * @brief OpenCL kernel для дробной задержки сигнала с интерполяцией Лагранжа
 * 
 * Реализует дробную задержку сигнала с использованием полинома Лагранжа 5-го порядка.
 * Использует матрицу коэффициентов 48×5 для точной интерполяции.
 * Каждый work item обрабатывает один отсчёт одного луча.
 */

/**
 * @brief Структура параметров задержки для каждого луча
 */
typedef struct {
    int delay_integer;    // Целая часть задержки
    int lagrange_row;     // Индекс строки матрицы Лагранжа [0, 47]
} DelayParams;

/**
 * @brief Выполнить дробную задержку сигнала с интерполяцией Лагранжа
 * 
 * @param input Буфер входных данных [num_beams * num_samples]
 *              Каждый элемент - complex<float> (float2: x=real, y=imag)
 * @param output Буфер выходных данных [num_beams * num_samples] (in-place)
 * @param lagrange_matrix Матрица коэффициентов Лагранжа [48 * 5]
 * @param delay_params Параметры задержки для каждого луча [num_beams]
 * @param num_beams Количество лучей
 * @param num_samples Количество отсчётов на луч
 */
__kernel void fractional_delay(
    __global const float2* input,
    __global float2* output,
    __global const float* lagrange_matrix,
    __global const DelayParams* delay_params,
    const uint num_beams,
    const uint num_samples
) {
    // 1D grid: каждый work item обрабатывает один отсчёт одного луча
    uint global_id = get_global_id(0);
    uint total_items = num_beams * num_samples;
    
    // Проверка границ
    if (global_id >= total_items) {
        return;
    }
    
    // Определяем луч и отсчёт
    uint beam_id = global_id / num_samples;
    uint sample_id = global_id % num_samples;
    
    // Получаем параметры задержки для этого луча
    DelayParams params = delay_params[beam_id];
    int delay_integer = params.delay_integer;
    int lagrange_row = params.lagrange_row;
    
    // Индекс для интерполяции (с целой частью задержки)
    // Используем 5 точек: [n-2, n-1, n, n+1, n+2]
    int interp_idx = (int)sample_id - delay_integer - 2;
    
    // Интерполяция Лагранжа (5 точек)
    float2 result = (float2)(0.0f, 0.0f);
    const int LAGRANGE_COLS = 5;
    
    #pragma unroll
    for (int i = 0; i < LAGRANGE_COLS; i++) {
        int idx = interp_idx + i;
        
        // Обработка граничных условий (отражение)
        if (idx < 0) {
            idx = -idx;  // Отражение от начала
        }
        if (idx >= (int)num_samples) {
            idx = 2 * (int)num_samples - idx - 2;  // Отражение от конца
        }
        
        // Безопасное извлечение отсчёта
        if (idx >= 0 && idx < (int)num_samples) {
            size_t base_idx = beam_id * num_samples + idx;
            float2 sample = input[base_idx];
            
            // Коэффициент из матрицы Лагранжа
            int matrix_idx = lagrange_row * LAGRANGE_COLS + i;
            float coeff = lagrange_matrix[matrix_idx];
            
            // Применить коэффициент к комплексному числу
            result.x += coeff * sample.x;  // Real part
            result.y += coeff * sample.y;  // Imag part
        }
    }
    
    // Записать результат (in-place)
    size_t output_idx = beam_id * num_samples + sample_id;
    output[output_idx] = result;
}
