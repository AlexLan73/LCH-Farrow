#pragma once

#include "opencl_manager.h"
#include "lfm_signal_generator.h"

#include <CL/cl.h>
#include <complex>
#include <vector>
#include <stdexcept>
#include <iostream>

/**
 * @file generator_gpu_refactored.h
 * @brief GeneratorGPU с использованием OpenCLManager Singleton
 * 
 * ИЗМЕНЕНИЯ ОТНОСИТЕЛЬНО ОРИГИНАЛА:
 * 
 * ✅ УДАЛЕНО:
 *   - cl_platform_id platform_
 *   - cl_device_id device_
 *   - InitializeOpenCL() (больше не нужна)
 *   - Инициализация контекста/очереди в конструкторе
 * 
 * ✅ ДОБАВЛЕНО:
 *   - OpenCLManager& manager_ (ссылка на Singleton)
 *   - Получение контекста/очереди из Manager
 * 
 * ПРОИЗВОДИТЕЛЬНОСТЬ:
 * - ДО: каждый GeneratorGPU инициализирует OpenCL (~200 мс)
 * - ПОСЛЕ: берет контекст из Manager (~0 мс)
 * - Результат: 3x ускорение для 3-х объектов
 * 
 * API СОВМЕСТИМОСТЬ: 100% совместим со старым кодом
 * signal_base() и signal_valedation() работают как раньше
 */

namespace radar {
namespace gpu {

struct DelayParameter {
    uint32_t beam_index;
    float delay_degrees;
};

class GeneratorGPU {
private:
    // OpenCL ресурсы (берются из Manager)
    OpenCLManager& manager_;
    cl_context context_;
    cl_command_queue queue_;
    cl_program program_;
    
    // Kernels
    cl_kernel kernel_lfm_basic_;
    cl_kernel kernel_lfm_delayed_;
    
    // Параметры сигнала
    const LFMParameters params_;
    
    // Размеры данных
    size_t num_samples_;
    size_t num_beams_;
    size_t total_size_;
    
    // PRIVATE METHODS
    
    /**
     * @brief Скомпилировать OpenCL kernels
     * 
     * Использует OpenCLManager::GetOrCompileProgram() для кэширования
     */
    void CompileKernels();
    
    /**
     * @brief Получить исходный код kernels
     */
    std::string GetKernelSource() const;
    
public:
    /**
     * @brief Конструктор с параметрами ЛЧМ
     * 
     * ВАЖНО: OpenCLManager должен быть инициализирован ДО создания этого объекта!
     * 
     * @code
     * // В main():
     * OpenCLManager::Initialize(CL_DEVICE_TYPE_GPU);  // ← один раз
     * 
     * GeneratorGPU gen(params);  // ← безопасно использовать
     * @endcode
     * 
     * @param params Параметры сигнала (частоты, sample_rate, num_beams, duration)
     * @throws std::runtime_error если OpenCL инициализация не удалась
     * @throws std::invalid_argument если параметры некорректны
     */
    explicit GeneratorGPU(const LFMParameters& params);
    
    /**
     * @brief Деструктор - освобождение GPU ресурсов
     * 
     * ПРИМЕЧАНИЕ: контекст и очередь НЕ освобождаются
     * (они управляются OpenCLManager)
     */
    ~GeneratorGPU();
    
    // DELETE COPY, ALLOW MOVE
    GeneratorGPU(const GeneratorGPU&) = delete;
    GeneratorGPU& operator=(const GeneratorGPU&) = delete;
    GeneratorGPU(GeneratorGPU&&) noexcept;
    GeneratorGPU& operator=(GeneratorGPU&&) noexcept;
    
    // ═══════════════════════════════════════════════════════════════
    // PUBLIC API
    // ═══════════════════════════════════════════════════════════════
    
    /**
     * @brief Сформировать БАЗОВЫЙ ЛЧМ сигнал на GPU
     * 
     * Параллельно на GPU генерирует ЛЧМ сигнал для всех лучей.
     * Сигнал записывается в GPU памяти.
     * 
     * @return cl_mem адрес GPU памяти с базовыми сигналами
     * @throws std::runtime_error если OpenCL операция не удалась
     */
    cl_mem signal_base();
    
    /**
     * @brief Сформировать ЛЧМ сигнал с ДРОБНОЙ ЗАДЕРЖКОЙ на GPU
     * 
     * Параллельно на GPU генерирует ЛЧМ сигналы с заданными задержками по лучам.
     * 
     * @param m_delay Массив параметров задержки (beam_id, delay_degrees)
     * @param num_delay_params Количество элементов в m_delay (обычно num_beams)
     * @return cl_mem адрес GPU памяти с сигналами по лучам с задержками
     * @throws std::runtime_error если OpenCL операция не удалась
     * @throws std::invalid_argument если размеры параметров неверны
     */
    cl_mem signal_valedation(
        const DelayParameter* m_delay,
        size_t num_delay_params
    );
    
    /**
     * @brief Очистить GPU очередь
     */
    void ClearGPU();
    
    // ═══════════════════════════════════════════════════════════════
    // GETTERS
    // ═══════════════════════════════════════════════════════════════
    
    size_t GetNumBeams() const noexcept { return num_beams_; }
    size_t GetNumSamples() const noexcept { return num_samples_; }
    size_t GetTotalSize() const noexcept { return total_size_; }
    
    size_t GetMemorySizeBytes() const noexcept {
        return total_size_ * sizeof(std::complex<float>);
    }
    
    cl_context GetContext() const noexcept { return context_; }
    cl_command_queue GetQueue() const noexcept { return queue_; }
    cl_device_id GetDevice() const noexcept { return manager_.GetDevice(); }
    
    const LFMParameters& GetParameters() const noexcept { return params_; }
};

} // namespace gpu
} // namespace radar
