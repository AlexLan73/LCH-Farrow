#ifndef OPENCL_BACKEND_H
#define OPENCL_BACKEND_H

#include "igpu_backend.h"
#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>
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

