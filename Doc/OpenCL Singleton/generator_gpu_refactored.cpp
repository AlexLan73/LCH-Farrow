#include "generator_gpu_refactored.h"
#include <iostream>
#include <sstream>

namespace radar {
namespace gpu {

// ═══════════════════════════════════════════════════════════════════
// CONSTRUCTOR / DESTRUCTOR
// ═══════════════════════════════════════════════════════════════════

GeneratorGPU::GeneratorGPU(const LFMParameters& params)
    : manager_(OpenCLManager::GetInstance()),
      context_(manager_.GetContext()),
      queue_(manager_.GetQueue()),
      program_(nullptr),
      kernel_lfm_basic_(nullptr),
      kernel_lfm_delayed_(nullptr),
      params_(params),
      num_samples_(params.GetNumSamples()),
      num_beams_(params.num_beams),
      total_size_(num_beams_ * num_samples_) {
    
    if (!params_.IsValid()) {
        throw std::invalid_argument("Invalid LFMParameters");
    }
    
    if (!manager_.IsInitialized()) {
        throw std::runtime_error(
            "OpenCLManager not initialized. "
            "Call OpenCLManager::Initialize() before creating GeneratorGPU"
        );
    }
    
    try {
        CompileKernels();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("GeneratorGPU initialization failed: ") + e.what());
    }
}

GeneratorGPU::~GeneratorGPU() {
    // Освобождение kernels (контекст и очередь управляются Manager)
    if (kernel_lfm_basic_ != nullptr) {
        clReleaseKernel(kernel_lfm_basic_);
        kernel_lfm_basic_ = nullptr;
    }
    
    if (kernel_lfm_delayed_ != nullptr) {
        clReleaseKernel(kernel_lfm_delayed_);
        kernel_lfm_delayed_ = nullptr;
    }
    
    // Программу НЕ освобождаем - она в кэше Manager
}

// Move semantics
GeneratorGPU::GeneratorGPU(GeneratorGPU&&) noexcept = default;
GeneratorGPU& GeneratorGPU::operator=(GeneratorGPU&&) noexcept = default;

// ═══════════════════════════════════════════════════════════════════
// PRIVATE METHODS
// ═══════════════════════════════════════════════════════════════════

void GeneratorGPU::CompileKernels() {
    cl_int err = CL_SUCCESS;
    
    // Получить исходный код
    std::string source = GetKernelSource();
    
    // Использовать Manager для кэширования программы!
    program_ = manager_.GetOrCompileProgram(source);
    
    if (program_ == nullptr) {
        throw std::runtime_error("Failed to get compiled program");
    }
    
    // Создать kernels
    kernel_lfm_basic_ = clCreateKernel(program_, "kernel_lfm_basic", &err);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to create kernel_lfm_basic");
    }
    
    kernel_lfm_delayed_ = clCreateKernel(program_, "kernel_lfm_delayed", &err);
    if (err != CL_SUCCESS) {
        clReleaseKernel(kernel_lfm_basic_);
        kernel_lfm_basic_ = nullptr;
        throw std::runtime_error("Failed to create kernel_lfm_delayed");
    }
}

std::string GeneratorGPU::GetKernelSource() const {
    return R"(
typedef struct {
    uint beam_index;
    float delay_degrees;
} DelayParam;

__kernel void kernel_lfm_basic(
    __global float2 *output,
    float f_start,
    float f_stop,
    float sample_rate,
    float duration,
    uint num_samples,
    uint num_beams
) {
    uint gid = get_global_id(0);
    
    if (gid >= num_samples * num_beams) return;
    
    uint ray_id = gid / num_samples;
    uint sample_id = gid % num_samples;
    
    if (ray_id >= num_beams || sample_id >= num_samples) return;
    
    float t = (float)sample_id / sample_rate;
    float chirp_rate = (f_stop - f_start) / duration;
    float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
    
    float real = cos(phase);
    float imag = sin(phase);
    
    uint out_idx = ray_id * num_samples + sample_id;
    output[out_idx] = (float2)(real, imag);
}

__kernel void kernel_lfm_delayed(
    __global float2 *output,
    __constant DelayParam *m_delay,
    float f_start,
    float f_stop,
    float sample_rate,
    float duration,
    float speed_of_light,
    uint num_samples,
    uint num_beams,
    uint num_delays
) {
    uint gid = get_global_id(0);
    
    if (gid >= num_samples * num_beams) return;
    
    uint ray_id = gid / num_samples;
    uint sample_id = gid % num_samples;
    
    if (ray_id >= num_beams || sample_id >= num_samples) return;
    
    float delay_degrees = m_delay[ray_id].delay_degrees;
    
    float f_center = (f_start + f_stop) / 2.0f;
    float wavelength = speed_of_light / f_center;
    float delay_rad = delay_degrees * 3.14159265f / 180.0f;
    float delay_time = delay_rad * wavelength / speed_of_light;
    float delay_samples = delay_time * sample_rate;
    
    int delayed_sample_int = (int)sample_id - (int)delay_samples;
    
    float real, imag;
    
    if (delayed_sample_int < 0) {
        real = 0.0f;
        imag = 0.0f;
    } else {
        float t = (float)delayed_sample_int / sample_rate;
        float chirp_rate = (f_stop - f_start) / duration;
        float phase = 2.0f * 3.14159265f * (f_start * t + 0.5f * chirp_rate * t * t);
        
        real = cos(phase);
        imag = sin(phase);
    }
    
    uint out_idx = ray_id * num_samples + sample_id;
    output[out_idx] = (float2)(real, imag);
}
)";
}

// ═══════════════════════════════════════════════════════════════════
// PUBLIC API
// ═══════════════════════════════════════════════════════════════════

cl_mem GeneratorGPU::signal_base() {
    cl_int err = CL_SUCCESS;
    
    // Создать буфер для вывода
    size_t buffer_size = total_size_ * sizeof(std::complex<float>);
    cl_mem output = clCreateBuffer(
        context_,
        CL_MEM_WRITE_ONLY,
        buffer_size,
        nullptr,
        &err
    );
    
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to allocate GPU buffer for signal_base");
    }
    
    // Установить аргументы kernel
    clSetKernelArg(kernel_lfm_basic_, 0, sizeof(cl_mem), &output);
    clSetKernelArg(kernel_lfm_basic_, 1, sizeof(float), &params_.f_start);
    clSetKernelArg(kernel_lfm_basic_, 2, sizeof(float), &params_.f_stop);
    clSetKernelArg(kernel_lfm_basic_, 3, sizeof(float), &params_.sample_rate);
    clSetKernelArg(kernel_lfm_basic_, 4, sizeof(float), &params_.duration);
    
    uint num_samples = static_cast<uint>(num_samples_);
    uint num_beams = static_cast<uint>(num_beams_);
    
    clSetKernelArg(kernel_lfm_basic_, 5, sizeof(uint), &num_samples);
    clSetKernelArg(kernel_lfm_basic_, 6, sizeof(uint), &num_beams);
    
    // Запустить kernel
    size_t global_work_size = total_size_;
    size_t local_work_size = 256;
    
    err = clEnqueueNDRangeKernel(
        queue_,
        kernel_lfm_basic_,
        1,
        nullptr,
        &global_work_size,
        &local_work_size,
        0, nullptr, nullptr
    );
    
    if (err != CL_SUCCESS) {
        clReleaseMemObject(output);
        throw std::runtime_error("Failed to enqueue kernel_lfm_basic");
    }
    
    clFinish(queue_);
    
    std::cout << "✓ signal_base() completed. GPU memory: " 
              << buffer_size / (1024*1024) << " MB" << std::endl;
    
    return output;
}

cl_mem GeneratorGPU::signal_valedation(
    const DelayParameter* m_delay,
    size_t num_delay_params
) {
    if (m_delay == nullptr) {
        throw std::invalid_argument("m_delay array is null");
    }
    
    if (num_delay_params != num_beams_) {
        throw std::invalid_argument(
            "num_delay_params must equal num_beams (" +
            std::to_string(num_beams_) + ")"
        );
    }
    
    cl_int err = CL_SUCCESS;
    
    // Создать буфер для вывода
    size_t buffer_size = total_size_ * sizeof(std::complex<float>);
    cl_mem output = clCreateBuffer(
        context_,
        CL_MEM_WRITE_ONLY,
        buffer_size,
        nullptr,
        &err
    );
    
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to allocate GPU buffer for signal_valedation");
    }
    
    // Создать буфер для параметров задержки
    size_t delay_buffer_size = num_delay_params * sizeof(DelayParameter);
    cl_mem delay_buffer = clCreateBuffer(
        context_,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        delay_buffer_size,
        const_cast<DelayParameter*>(m_delay),
        &err
    );
    
    if (err != CL_SUCCESS) {
        clReleaseMemObject(output);
        throw std::runtime_error("Failed to allocate GPU buffer for delay parameters");
    }
    
    // Установить аргументы kernel
    clSetKernelArg(kernel_lfm_delayed_, 0, sizeof(cl_mem), &output);
    clSetKernelArg(kernel_lfm_delayed_, 1, sizeof(cl_mem), &delay_buffer);
    clSetKernelArg(kernel_lfm_delayed_, 2, sizeof(float), &params_.f_start);
    clSetKernelArg(kernel_lfm_delayed_, 3, sizeof(float), &params_.f_stop);
    clSetKernelArg(kernel_lfm_delayed_, 4, sizeof(float), &params_.sample_rate);
    clSetKernelArg(kernel_lfm_delayed_, 5, sizeof(float), &params_.duration);
    
    float speed_of_light = 3.0e8f;
    clSetKernelArg(kernel_lfm_delayed_, 6, sizeof(float), &speed_of_light);
    
    uint num_samples = static_cast<uint>(num_samples_);
    uint num_beams = static_cast<uint>(num_beams_);
    uint num_delays = static_cast<uint>(num_delay_params);
    
    clSetKernelArg(kernel_lfm_delayed_, 7, sizeof(uint), &num_samples);
    clSetKernelArg(kernel_lfm_delayed_, 8, sizeof(uint), &num_beams);
    clSetKernelArg(kernel_lfm_delayed_, 9, sizeof(uint), &num_delays);
    
    // Запустить kernel
    size_t global_work_size = total_size_;
    size_t local_work_size = 256;
    
    err = clEnqueueNDRangeKernel(
        queue_,
        kernel_lfm_delayed_,
        1,
        nullptr,
        &global_work_size,
        &local_work_size,
        0, nullptr, nullptr
    );
    
    if (err != CL_SUCCESS) {
        clReleaseMemObject(output);
        clReleaseMemObject(delay_buffer);
        throw std::runtime_error("Failed to enqueue kernel_lfm_delayed");
    }
    
    clFinish(queue_);
    clReleaseMemObject(delay_buffer);
    
    std::cout << "✓ signal_valedation() completed. GPU memory: " 
              << buffer_size / (1024*1024) << " MB" << std::endl;
    
    return output;
}

void GeneratorGPU::ClearGPU() {
    clFinish(queue_);
}

} // namespace gpu
} // namespace radar
