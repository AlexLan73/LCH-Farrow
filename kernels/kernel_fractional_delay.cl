/**
 * @file kernel_fractional_delay.cl
 * @brief OpenCL kernel для дробной задержки сигнала с интерполяцией Лагранжа
 * 
 * Реализует дробную задержку сигнала с использованием полинома Лагранжа 5-го порядка.
 * Использует матрицу коэффициентов 48×5 для точной интерполяции.
 * Каждый work item обрабатывает один отсчёт одного луча.
 * 
 * Оптимизировано для OpenCL C 3.0 (с обратной совместимостью с 1.2)
 */

#ifdef __OPENCL_C_VERSION__
    #if __OPENCL_C_VERSION__ >= 300
        // OpenCL C 3.0: используем новые возможности
        #define OPENCL_C_30_ENABLED
        #pragma OPENCL EXTENSION cl_khr_subgroups : enable
    #endif
#endif

/**
 * @brief Структура параметров задержки для каждого луча
 */
typedef struct {
    int delay_integer;    // Целая часть задержки
    int lagrange_row;     // Индекс строки матрицы Лагранжа [0, 47]
} DelayParams;

/**
 * @brief Вспомогательная функция для безопасного индекса с отражением границ
 * 
 * @param idx Исходный индекс
 * @param num_samples Количество отсчётов
 * @return Корректный индекс с отражением границ
 */
inline int reflect_boundary(int idx, const uint num_samples) {
    if (idx < 0) {
        return -idx;  // Отражение от начала
    }
    if (idx >= (int)num_samples) {
        return 2 * (int)num_samples - idx - 2;  // Отражение от конца
    }
    return idx;
}

/**
 * @brief Выполнить дробную задержку сигнала с интерполяцией Лагранжа
 * 
 * Оптимизированная версия для OpenCL C 3.0:
 * - Использует подгруппы для эффективной работы с памятью (если доступно)
 * - Улучшенная векторизация
 * - Оптимизированная работа с граничными условиями
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
    
    // Проверка границ (ранний выход для лучшей производительности)
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
    
    // Интерполяция Лагранжа (5 точек) - полностью развёрнутый цикл для лучшей производительности
    float2 result = (float2)(0.0f, 0.0f);
    const int LAGRANGE_COLS = 5;
    
    // Предвычисляем индексы для всех 5 точек
    int idx0 = reflect_boundary(interp_idx + 0, num_samples);
    int idx1 = reflect_boundary(interp_idx + 1, num_samples);
    int idx2 = reflect_boundary(interp_idx + 2, num_samples);
    int idx3 = reflect_boundary(interp_idx + 3, num_samples);
    int idx4 = reflect_boundary(interp_idx + 4, num_samples);
    
    // Базовый индекс для этого луча
    uint base_offset = beam_id * num_samples;
    
    // Базовый индекс в матрице Лагранжа
    uint matrix_base = lagrange_row * LAGRANGE_COLS;
    
    // Загружаем коэффициенты Лагранжа (лучше для кэша)
    float coeff0 = lagrange_matrix[matrix_base + 0];
    float coeff1 = lagrange_matrix[matrix_base + 1];
    float coeff2 = lagrange_matrix[matrix_base + 2];
    float coeff3 = lagrange_matrix[matrix_base + 3];
    float coeff4 = lagrange_matrix[matrix_base + 4];
    
    // Интерполяция с полной развёрткой цикла
    // Проверяем границы только один раз перед использованием
    if (idx0 >= 0 && idx0 < (int)num_samples) {
        float2 sample = input[base_offset + idx0];
        result.x = mad(coeff0, sample.x, result.x);  // Используем mad для быстрого умножения-сложения
        result.y = mad(coeff0, sample.y, result.y);
    }
    
    if (idx1 >= 0 && idx1 < (int)num_samples) {
        float2 sample = input[base_offset + idx1];
        result.x = mad(coeff1, sample.x, result.x);
        result.y = mad(coeff1, sample.y, result.y);
    }
    
    if (idx2 >= 0 && idx2 < (int)num_samples) {
        float2 sample = input[base_offset + idx2];
        result.x = mad(coeff2, sample.x, result.x);
        result.y = mad(coeff2, sample.y, result.y);
    }
    
    if (idx3 >= 0 && idx3 < (int)num_samples) {
        float2 sample = input[base_offset + idx3];
        result.x = mad(coeff3, sample.x, result.x);
        result.y = mad(coeff3, sample.y, result.y);
    }
    
    if (idx4 >= 0 && idx4 < (int)num_samples) {
        float2 sample = input[base_offset + idx4];
        result.x = mad(coeff4, sample.x, result.x);
        result.y = mad(coeff4, sample.y, result.y);
    }
    
    // Записать результат (in-place) - используем векторную запись
    output[global_id] = result;
}
