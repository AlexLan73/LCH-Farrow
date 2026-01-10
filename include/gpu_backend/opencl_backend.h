#ifndef OPENCL_BACKEND_H
#define OPENCL_BACKEND_H

#include "igpu_backend.h"
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

// Для совместимости с разными системами
#ifndef CL_HPP_TARGET_OPENCL_VERSION
#define CL_HPP_TARGET_OPENCL_VERSION 200
#endif
#ifndef CL_HPP_MINIMUM_OPENCL_VERSION
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#endif
#if CLFFT_FOUND
#include <clFFT.h>
#endif
#include <string>
#include <vector>
#include <memory>

/**
 * @brief Реализация GPU backend через OpenCL
 * 
 * Поддерживает NVIDIA и AMD GPU через OpenCL API.
 */
class OpenCLBackend : public IGPUBackend {
public:
    /**
     * @brief Конструктор
     */
    OpenCLBackend();
    
    /**
     * @brief Деструктор
     */
    ~OpenCLBackend();
    
    // IGPUBackend interface
    bool Initialize() override;
    void Cleanup() override;
    void* AllocateDeviceMemory(size_t size_bytes) override;
    void FreeDeviceMemory(void* ptr) override;
    bool CopyHostToDevice(void* dst, const void* src, size_t size_bytes) override;
    bool CopyDeviceToHost(void* dst, const void* src, size_t size_bytes) override;
    bool ExecuteFractionalDelay(
        void* device_buffer,
        const float* delay_coefficients,
        size_t num_beams,
        size_t num_samples
    ) override;
    bool ExecuteFFT(
        void* device_buffer,
        size_t num_beams,
        size_t num_samples,
        bool forward
    ) override;
    bool ExecuteHadamardMultiply(
        void* device_buffer,
        const void* reference_fft,
        size_t num_beams,
        size_t num_samples
    ) override;
    std::string GetBackendName() const override;
    std::string GetDeviceName() const override;
    size_t GetDeviceMemorySize() const override;
    bool UploadLagrangeMatrix(const float* lagrange_data) override;
    
    /**
     * @brief Получить системную информацию (GPU, драйвер, OpenCL версия)
     */
    struct SystemInfo {
        std::string device_name;
        std::string device_vendor;
        std::string device_version;        // OpenCL API версия устройства (например, "OpenCL 3.0 CUDA")
        std::string driver_version;
        std::string opencl_c_version;      // OpenCL C язык версия (например, "OpenCL C 1.2")
        std::string platform_name;
        std::string platform_version;
        size_t device_memory_mb;
        size_t max_work_group_size;
        size_t compute_units;
        std::string os_name;
        std::string os_version;
    };
    SystemInfo GetSystemInfo() const;
    
    /**
     * @brief Копировать данные с хоста на устройство с профилированием GPU Events
     * @param dst Указатель на память устройства
     * @param src Указатель на память хоста
     * @param size_bytes Размер в байтах
     * @param event_out Ссылка на Event для профилирования
     * @return true если успешно
     */
    bool CopyHostToDeviceWithProfiling(
        void* dst, 
        const void* src, 
        size_t size_bytes,
        cl::Event& event_out
    );
    
    /**
     * @brief Выполнить дробную задержку с профилированием GPU Events
     * @param device_buffer Указатель на буфер на устройстве
     * @param delay_coefficients Коэффициенты задержки для каждого луча
     * @param num_beams Количество лучей
     * @param num_samples Количество отсчётов на луч
     * @param event_out Ссылка на Event для профилирования
     * @return true если успешно
     */
    bool ExecuteFractionalDelayWithProfiling(
        void* device_buffer,
        const float* delay_coefficients,
        size_t num_beams,
        size_t num_samples,
        cl::Event& event_out
    );
    
    /**
     * @brief Копировать данные с устройства на хост с профилированием GPU Events
     * @param dst Указатель на память хоста
     * @param src Указатель на память устройства
     * @param size_bytes Размер в байтах
     * @param event_out Ссылка на Event для профилирования
     * @return true если успешно
     */
    bool CopyDeviceToHostWithProfiling(
        void* dst, 
        const void* src, 
        size_t size_bytes,
        cl::Event& event_out
    );

private:
    cl::Platform platform_;
    cl::Device device_;
    cl::Context context_;
    cl::CommandQueue queue_;
    cl::Program program_;
    
    // Kernels
    cl::Kernel kernel_fractional_delay_;
    cl::Kernel kernel_hadamard_;
    
    // clFFT plans
#if CLFFT_FOUND
    clfftPlanHandle fft_plan_forward_;
    clfftPlanHandle fft_plan_inverse_;
    bool fft_plans_created_;
#endif
    
    // Матрица Лагранжа для дробной задержки
    cl::Buffer lagrange_matrix_buffer_;
    bool lagrange_matrix_uploaded_;
    
    // Информация об устройстве
    std::string device_name_;
    size_t device_memory_size_;
    bool initialized_;
    
    /**
     * @brief Найти и выбрать оптимальное OpenCL устройство
     * @return true если устройство найдено
     */
    bool SelectDevice();
    
    /**
     * @brief Загрузить и скомпилировать OpenCL программы
     * @return true если успешно
     */
    bool BuildProgram();
    
    /**
     * @brief Загрузить kernel из файла
     * @param filename Имя файла .cl
     * @return Содержимое файла или пустая строка при ошибке
     */
    std::string LoadKernelSource(const std::string& filename) const;
    
    /**
     * @brief Создать FFT планы для clFFT
     * @param num_samples Размер FFT
     * @param num_beams Batch size
     * @return true если успешно
     */
    bool CreateFFTPlans(size_t num_samples, size_t num_beams);
    
    /**
     * @brief Удалить FFT планы
     */
    void DestroyFFTPlans();
    
    
    /**
     * @brief Проверить ошибку OpenCL
     * @param err Код ошибки
     * @param context Контекст операции
     * @return true если нет ошибки
     */
    bool CheckError(cl_int err, const std::string& context) const;
};

#endif // OPENCL_BACKEND_H

