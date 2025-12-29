#ifndef PROCESSING_PIPELINE_H
#define PROCESSING_PIPELINE_H

#include "signal_buffer.h"
#include "filter_bank.h"
#include "gpu_backend/igpu_backend.h"
#include "profiling_engine.h"
#include <memory>

/**
 * @brief Класс для координации pipeline обработки сигнала
 * 
 * Управляет этапами обработки до формирования матрицы с задержанными сигналами:
 * 1. H2D Transfer (загрузка данных на GPU)
 * 2. Дробная задержка (формирование матрицы с задержанными сигналами)
 * 3. Опционально: D2H Transfer (вывод с GPU для анализа)
 */
class ProcessingPipeline {
public:
    /**
     * @brief Конструктор
     * @param signal_buffer Указатель на буфер сигналов
     * @param filter_bank Указатель на банк фильтров (может быть nullptr)
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
     * @brief Выполнить pipeline обработки до формирования матрицы с задержанными сигналами
     * @param copy_to_host Опционально скопировать результат с GPU на хост для анализа
     * @return true если успешно
     */
    bool ExecuteFull(bool copy_to_host = false);
    
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
    size_t device_buffer_size_;
    
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

