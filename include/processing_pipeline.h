#ifndef PROCESSING_PIPELINE_H
#define PROCESSING_PIPELINE_H

#include "signal_buffer.h"
#include "filter_bank.h"
#include "gpu_backend/igpu_backend.h"
#include "profiling_engine.h"
#include <memory>

/**
 * @brief Класс для координации полного pipeline обработки сигнала
 * 
 * Управляет всеми этапами обработки:
 * 1. Дробная задержка
 * 2. FFT Forward
 * 3. Hadamard Multiply
 * 4. IFFT Inverse
 */
class ProcessingPipeline {
public:
    /**
     * @brief Конструктор
     * @param signal_buffer Указатель на буфер сигналов
     * @param filter_bank Указатель на банк фильтров
     * @param gpu_backend Указатель на GPU backend
     * @param profiler Указатель на профилировщик
     */
    ProcessingPipeline(
        SignalBuffer* signal_buffer,
        FilterBank* filter_bank,
        IGPUBackend* gpu_backend,
        ProfilingEngine* profiler
    );
    
    /**
     * @brief Деструктор
     */
    ~ProcessingPipeline();
    
    /**
     * @brief Выполнить полный pipeline обработки
     * @return true если успешно
     */
    bool ExecuteFull();
    
    /**
     * @brief Выполнить пошагово (для отладки)
     * @return true если успешно
     */
    bool ExecuteStepByStep();
    
    /**
     * @brief Валидировать результаты
     * @param tolerance Допустимая погрешность
     * @return true если результаты валидны
     */
    bool ValidateResults(float tolerance = 1e-5f);
    
    /**
     * @brief Получить метрики профилирования
     * @return Константная ссылка на метрики
     */
    const ProfilingMetrics& GetMetrics() const;

private:
    SignalBuffer* signal_buffer_;
    FilterBank* filter_bank_;
    IGPUBackend* gpu_backend_;
    ProfilingEngine* profiler_;
    
    // GPU буферы
    void* device_buffer_;
    void* device_reference_fft_;
    size_t device_buffer_size_;
    size_t device_reference_fft_size_;
    
    /**
     * @brief Выделить память на GPU
     * @return true если успешно
     */
    bool AllocateDeviceMemory();
    
    /**
     * @brief Освободить память на GPU
     */
    void FreeDeviceMemory();
    
    /**
     * @brief Копировать данные с хоста на устройство
     * @return true если успешно
     */
    bool CopyHostToDevice();
    
    /**
     * @brief Копировать данные с устройства на хост
     * @return true если успешно
     */
    bool CopyDeviceToHost();
};

#endif // PROCESSING_PIPELINE_H

