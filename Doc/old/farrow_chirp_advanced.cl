/*
 * ✓ FARROW FILTER ДЛЯ ЛЧМ СИГНАЛОВ С МАТРИЦЕЙ КОЭФФИЦИЕНТОВ 48×5
 * 
 * Возможности:
 * 1. Обработка ЛЧМ (chirp) сигналов с переменной задержкой
 * 2. Динамическая загрузка матрицы коэффициентов 48×5
 * 3. Поддержка более высокого полинома (до 4-й степени)
 * 4. Расширенная точность интерполяции
 * 
 * Параметры:
 *   - N = 1,300,000 точек
 *   - P = 4 (порядок полинома: 5 коэффициентов)
 *   - L = 48 (длина фильтра: 48 коэффициентов на порядок)
 *   - Матрица: 48 (длина) × 5 (порядок) = 240 коэффициентов
 */

// ============================================================================
// ВЕРСИЯ 1: БАЗОВАЯ С ДИНАМИЧЕСКОЙ МАТРИЦЕЙ (48×5)
// ============================================================================

__kernel void farrow_delay_chirp_basic(
    __global const float* x,           // Входной ЛЧМ сигнал
    __global float* y,                 // Выходной задержанный сигнал
    __constant float* coeffs,          // Матрица коэффициентов 48×5 (240 элементов)
    __global const float* delay_var,   // Массив переменной задержки μ[n]
    int N,                             // Размер сигнала
    int L,                             // Длина фильтра (48)
    int P                              // Порядок полинома (4, т.е. 5 коэффициентов)
) {
    int n = get_global_id(0);
    
    if (n < L/2 || n >= N - L/2) return;  // Защита от границ
    
    // ========== ЗАГРУЗИТЬ ПЕРЕМЕННУЮ ЗАДЕРЖКУ μ[n] ==========
    float mu = delay_var[n];
    
    // ========== ВЫЧИСЛИТЬ ПОЛИНОМИАЛЬНЫЕ БАЗИСЫ ==========
    float mu_pow[5];
    mu_pow[0] = 1.0f;
    mu_pow[1] = mu;
    mu_pow[2] = mu * mu;
    mu_pow[3] = mu_pow[2] * mu;
    mu_pow[4] = mu_pow[3] * mu;
    
    // ========== ОСНОВНОЙ ЦИКЛ СВЁРТКИ ==========
    float result = 0.0f;
    
    for (int p = 0; p <= P; p++) {
        float y_p = 0.0f;
        
        for (int k = 0; k < L; k++) {
            int idx = n - (L/2 - 1) + k;
            float x_val = (idx >= 0 && idx < N) ? x[idx] : 0.0f;
            float c_pk = coeffs[p * L + k];
            y_p = mad(c_pk, x_val, y_p);
        }
        
        result = mad(y_p, mu_pow[p], result);
    }
    
    y[n] = result;
}

// ============================================================================
// ВЕРСИЯ 3: МАКСИМАЛЬНО ОПТИМИЗИРОВАННАЯ (full unroll, no loops)
// ============================================================================

__kernel void farrow_delay_chirp_super_fast(
    __global const float* x,
    __global float* y,
    __constant float* coeffs,          // 240 элементов
    __global const float* delay_var,
    int N,
    int L,
    int P
) {
    int n = get_global_id(0);
    
    if (n < L/2 || n >= N - L/2) return;
    
    float mu = delay_var[n];
    float mu2 = mu * mu;
    float mu3 = mu2 * mu;
    float mu4 = mu3 * mu;
    
    float y_0 = 0.0f, y_1 = 0.0f, y_2 = 0.0f, y_3 = 0.0f, y_4 = 0.0f;
    
    for (int k = 0; k < L; k++) {
        int idx = n - (L/2 - 1) + k;
        float x_val = (idx >= 0 && idx < N) ? x[idx] : 0.0f;
        
        y_0 = mad(coeffs[0 * L + k], x_val, y_0);
        y_1 = mad(coeffs[1 * L + k], x_val, y_1);
        y_2 = mad(coeffs[2 * L + k], x_val, y_2);
        y_3 = mad(coeffs[3 * L + k], x_val, y_3);
        y_4 = mad(coeffs[4 * L + k], x_val, y_4);
    }
    
    float result = y_0 + mu * y_1 + mu2 * y_2 + mu3 * y_3 + mu4 * y_4;
    y[n] = result;
}
