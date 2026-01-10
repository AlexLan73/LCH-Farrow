#include "fractional_delay_cpu.h"
#include <iostream>
#include <cmath>
#include <algorithm>

bool ExecuteFractionalDelayCPU(
    SignalBuffer* input_output,
    const LagrangeMatrix* lagrange_matrix,
    const float* delay_coefficients,
    size_t num_beams,
    size_t num_samples) {
    
    if (!input_output || !lagrange_matrix || !delay_coefficients) {
        std::cerr << "Ошибка: неверные параметры для ExecuteFractionalDelayCPU" << std::endl;
        return false;
    }
    
    if (!lagrange_matrix->IsValid()) {
        std::cerr << "Ошибка: матрица Лагранжа не валидна" << std::endl;
        return false;
    }
    
    if (input_output->GetNumBeams() != num_beams || 
        input_output->GetNumSamples() != num_samples) {
        std::cerr << "Ошибка: несоответствие размеров буфера" << std::endl;
        return false;
    }
    
    const size_t LAGRANGE_ROWS = 48;
    const size_t LAGRANGE_COLS = 5;
    
    // Вычисляем параметры задержки для каждого луча
    struct DelayParams {
        int delay_integer;
        int lagrange_row;
    };
    
    std::vector<DelayParams> delay_params(num_beams);
    for (size_t beam = 0; beam < num_beams; ++beam) {
        float delay = delay_coefficients[beam];
        delay_params[beam].delay_integer = static_cast<int>(std::floor(delay));
        float delay_fraction = delay - delay_params[beam].delay_integer;
        
        // Обработка отрицательной дробной части
        if (delay_fraction < 0.0f) {
            delay_fraction += 1.0f;
            delay_params[beam].delay_integer -= 1;
        }
        
        // Вычисляем индекс строки матрицы Лагранжа
        delay_params[beam].lagrange_row = static_cast<int>(delay_fraction * LAGRANGE_ROWS);
        if (delay_params[beam].lagrange_row >= static_cast<int>(LAGRANGE_ROWS)) {
            delay_params[beam].lagrange_row = LAGRANGE_ROWS - 1;
        }
    }
    
    // Создаём временный буфер для результатов (нужен для правильной in-place обработки)
    std::vector<SignalBuffer::ComplexType> output_buffer(num_beams * num_samples);
    
    // Для каждого луча
    for (size_t beam = 0; beam < num_beams; ++beam) {
        const SignalBuffer::ComplexType* input_data = input_output->GetBeamData(beam);
        if (!input_data) {
            std::cerr << "Ошибка: не удалось получить данные для луча " << beam << std::endl;
            return false;
        }
        
        const DelayParams& params = delay_params[beam];
        int delay_integer = params.delay_integer;
        int lagrange_row = params.lagrange_row;
        
        // Для каждого отсчёта в луче
        for (size_t sample = 0; sample < num_samples; ++sample) {
            // Индекс для интерполяции (с целой частью задержки)
            // Используем 5 точек: [n-2, n-1, n, n+1, n+2]
            int interp_idx = static_cast<int>(sample) - delay_integer - 2;
            
            // Интерполяция Лагранжа (5 точек)
            SignalBuffer::ComplexType result(0.0f, 0.0f);
            
            for (int i = 0; i < static_cast<int>(LAGRANGE_COLS); ++i) {
                int idx = interp_idx + i;
                
                // Обработка граничных условий (отражение, как в GPU kernel)
                if (idx < 0) {
                    idx = -idx;  // Отражение от начала
                }
                if (idx >= static_cast<int>(num_samples)) {
                    idx = 2 * static_cast<int>(num_samples) - idx - 2;  // Отражение от конца
                }
                
                // Безопасное извлечение отсчёта
                if (idx >= 0 && idx < static_cast<int>(num_samples)) {
                    SignalBuffer::ComplexType sample_data = input_data[idx];
                    
                    // Коэффициент из матрицы Лагранжа
                    float coeff = lagrange_matrix->GetCoefficient(
                        static_cast<size_t>(lagrange_row), 
                        static_cast<size_t>(i)
                    );
                    
                    // Применить коэффициент к комплексному числу
                    result += coeff * sample_data;
                }
            }
            
            // Сохранить результат во временный буфер
            size_t output_idx = beam * num_samples + sample;
            output_buffer[output_idx] = result;
        }
    }
    
    // Копировать результаты обратно в SignalBuffer (in-place)
    for (size_t beam = 0; beam < num_beams; ++beam) {
        SignalBuffer::ComplexType* output_data = input_output->GetBeamData(beam);
        if (!output_data) {
            std::cerr << "Ошибка: не удалось получить выходной буфер для луча " << beam << std::endl;
            return false;
        }
        
        for (size_t sample = 0; sample < num_samples; ++sample) {
            size_t idx = beam * num_samples + sample;
            output_data[sample] = output_buffer[idx];
        }
    }
    
    return true;
}

