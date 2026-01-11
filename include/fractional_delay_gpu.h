#ifndef FRACTIONAL_DELAY_GPU_H
#define FRACTIONAL_DELAY_GPU_H

#include "signal_buffer.h"
#include "gpu_backend/igpu_backend.h"
#include "gpu_profiling.h"
#include "profiling_engine.h"
#include <memory>
#include <vector>

namespace radar {

/**
 * @brief Класс для реализации дробной задержки на GPU
 *
 * Интегрирует GPUProcessor и OpenCLBackend для выполнения дробной задержки
 * с поддержкой профилирования и управления ресурсами.
 */
class FractionalDelayGPU {
public:
    /**
     * @brief Конструктор
     * @param backend Указатель на GPU backend (OpenCLBackend или другой)
     */
    explicit FractionalDelayGPU(IGPUBackend* backend);
    
    /**
     * @brief Деструктор
     */
    ~FractionalDelayGPU();
    
    /**
     * @brief Выполнить дробную задержку на GPU
     * @param input Входной буфер сигналов
     * @param delay_coeffs Коэффициенты задержки для каждого луча
     * @param output Выходной буфер сигналов
     * @param profiling_engine Указатель на движок профилирования (опционально)
     * @return true если успешно
     */
    bool ProcessFractionalDelay(
        const SignalBuffer& input,
        const float* delay_coeffs,
        SignalBuffer& output,
        ProfilingEngine* profiling_engine = nullptr
    );
    
    /**
     * @brief Выполнить дробную задержку с детальным профилированием
     * @param input Входной буфер сигналов
     * @param delay_coeffs Коэффициенты задержки для каждого луча
     * @param output Выходной буфер сигналов
     * @param detailed_profiling Структура для детального профилирования
     * @return true если успешно
     */
    bool ProcessFractionalDelayWithDetailedProfiling(
        const SignalBuffer& input,
        const float* delay_coeffs,
        SignalBuffer& output,
        DetailedGPUProfiling& detailed_profiling
    );
    
    /**
     * @brief Загрузить матрицу Лагранжа на GPU
     * @param lagrange_data Указатель на данные матрицы [48*5]
     * @return true если успешно
     */
    bool UploadLagrangeMatrix(const float* lagrange_data);
    
    /**
     * @brief Получить информацию о системе
     * @return Структура с системной информацией
     */
    SystemInfo GetSystemInfo() const;
    
    /**
     * @brief Включить/выключить профилирование
     * @param enable true для включения профилирования
     */
    void EnableProfiling(bool enable);
    
    /**
     * @brief Проверить, инициализирован ли backend
     * @return true если backend инициализирован
     */
    bool IsInitialized() const;

private:
    IGPUBackend* backend_;  // Указатель на GPU backend
    bool profiling_enabled_; // Флаг профилирования
    bool is_initialized_;    // Флаг инициализации
    
    /**
     * @brief Проверить валидность входных данных
     * @param input Входной буфер
     * @param delay_coeffs Коэффициенты задержки
     * @return true если данные валидны
     */
    bool ValidateInput(const SignalBuffer& input, const float* delay_coeffs) const;
    
    /**
     * @brief Выполнить копирование данных с хоста на устройство
     * @param device_ptr Указатель на память устройства
     * @param host_data Указатель на данные хоста
     * @param size_bytes Размер данных в байтах
     * @param event_out Ссылка на событие для профилирования (опционально)
     * @return true если успешно
     */
    bool CopyHostToDeviceWithProfiling(
        void* device_ptr,
        const void* host_data,
        size_t size_bytes,
        cl::Event* event_out = nullptr
    );
    
    /**
     * @brief Выполнить копирование данных с устройства на хост
     * @param host_ptr Указатель на память хоста
     * @param device_data Указатель на данные устройства
     * @param size_bytes Размер данных в байтах
     * @param event_out Ссылка на событие для профилирования (опционально)
     * @return true если успешно
     */
    bool CopyDeviceToHostWithProfiling(
        void* host_ptr,
        const void* device_data,
        size_t size_bytes,
        cl::Event* event_out = nullptr
    );
    
    /**
     * @brief Выполнить дробную задержку с профилированием
     * @param device_buffer Указатель на буфер на устройстве
     * @param delay_coeffs Коэффициенты задержки
     * @param num_beams Количество лучей
     * @param num_samples Количество отсчётов на луч
     * @param event_out Ссылка на событие для профилирования (опционально)
     * @return true если успешно
     */
    bool ExecuteFractionalDelayWithProfiling(
        void* device_buffer,
        const float* delay_coeffs,
        size_t num_beams,
        size_t num_samples,
        cl::Event* event_out = nullptr
    );
};

} // namespace radar

#endif // FRACTIONAL_DELAY_GPU_H