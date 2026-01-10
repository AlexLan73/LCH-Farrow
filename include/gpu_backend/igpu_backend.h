#ifndef IGPU_BACKEND_H
#define IGPU_BACKEND_H

#include <string>
#include <cstddef>
#include <complex>

/**
 * @brief Абстрактный интерфейс для GPU backend
 * 
 * Определяет общий интерфейс для работы с GPU независимо от платформы
 * (OpenCL, CUDA, HIP и т.д.)
 */
class IGPUBackend {
public:
    using ComplexType = std::complex<float>;
    
    virtual ~IGPUBackend() = default;
    
    /**
     * @brief Инициализировать GPU backend
     * @return true если успешно
     */
    virtual bool Initialize() = 0;
    
    /**
     * @brief Очистить ресурсы
     */
    virtual void Cleanup() = 0;
    
    /**
     * @brief Выделить память на устройстве
     * @param size_bytes Размер в байтах
     * @return Указатель на выделенную память или nullptr при ошибке
     */
    virtual void* AllocateDeviceMemory(size_t size_bytes) = 0;
    
    /**
     * @brief Освободить память на устройстве
     * @param ptr Указатель на память
     */
    virtual void FreeDeviceMemory(void* ptr) = 0;
    
    /**
     * @brief Копировать данные с хоста на устройство
     * @param dst Указатель на память устройства
     * @param src Указатель на память хоста
     * @param size_bytes Размер в байтах
     * @return true если успешно
     */
    virtual bool CopyHostToDevice(void* dst, const void* src, size_t size_bytes) = 0;
    
    /**
     * @brief Копировать данные с устройства на хост
     * @param dst Указатель на память хоста
     * @param src Указатель на память устройства
     * @param size_bytes Размер в байтах
     * @return true если успешно
     */
    virtual bool CopyDeviceToHost(void* dst, const void* src, size_t size_bytes) = 0;
    
    /**
     * @brief Выполнить дробную задержку сигнала
     * @param device_buffer Указатель на буфер на устройстве
     * @param delay_coefficients Коэффициенты задержки для каждого луча
     * @param num_beams Количество лучей
     * @param num_samples Количество отсчётов на луч
     * @return true если успешно
     */
    virtual bool ExecuteFractionalDelay(
        void* device_buffer,
        const float* delay_coefficients,
        size_t num_beams,
        size_t num_samples
    ) = 0;
    
    /**
     * @brief Выполнить FFT или IFFT
     * @param device_buffer Указатель на буфер на устройстве (in-place)
     * @param num_beams Количество лучей (batch size)
     * @param num_samples Количество отсчётов на луч
     * @param forward true для FFT, false для IFFT
     * @return true если успешно
     */
    virtual bool ExecuteFFT(
        void* device_buffer,
        size_t num_beams,
        size_t num_samples,
        bool forward
    ) = 0;
    
    /**
     * @brief Выполнить поэлементное умножение (Hadamard)
     * @param device_buffer Указатель на буфер лучей на устройстве (in-place)
     * @param reference_fft Указатель на опорную FFT на устройстве
     * @param num_beams Количество лучей
     * @param num_samples Количество отсчётов на луч
     * @return true если успешно
     */
    virtual bool ExecuteHadamardMultiply(
        void* device_buffer,
        const void* reference_fft,
        size_t num_beams,
        size_t num_samples
    ) = 0;
    
    /**
     * @brief Получить имя backend
     * @return Строка с именем (например, "OpenCL (NVIDIA)")
     */
    virtual std::string GetBackendName() const = 0;
    
    /**
     * @brief Получить имя устройства
     * @return Строка с именем устройства
     */
    virtual std::string GetDeviceName() const = 0;
    
    /**
     * @brief Получить размер памяти устройства
     * @return Размер в байтах
     */
    virtual size_t GetDeviceMemorySize() const = 0;
    
    /**
     * @brief Загрузить матрицу Лагранжа на GPU (опционально)
     * @param lagrange_data Указатель на данные матрицы [48*5]
     * @return true если успешно (или если не поддерживается)
     */
    virtual bool UploadLagrangeMatrix(const float* lagrange_data) {
        // Базовая реализация - ничего не делает
        (void)lagrange_data;
        return true;
    }
};

#endif // IGPU_BACKEND_H

