#include "gpu_backend/opencl_backend.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

OpenCLBackend::OpenCLBackend()
    : device_memory_size_(0), initialized_(false) {
}

OpenCLBackend::~OpenCLBackend() {
    Cleanup();
}

bool OpenCLBackend::Initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!SelectDevice()) {
        std::cerr << "Ошибка: не удалось выбрать OpenCL устройство" << std::endl;
        return false;
    }
    
    try {
        // Создаём context
        context_ = cl::Context(device_);
        
        // Создаём command queue с профилированием
        cl_int err = CL_SUCCESS;
        queue_ = cl::CommandQueue(context_, device_, CL_QUEUE_PROFILING_ENABLE, &err);
        if (!CheckError(err, "создание command queue")) {
            return false;
        }
        
        // Загружаем и компилируем программы
        if (!BuildProgram()) {
            return false;
        }
        
        // Получаем информацию об устройстве
        device_.getInfo(CL_DEVICE_NAME, &device_name_);
        cl_ulong mem_size;
        device_.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &mem_size);
        device_memory_size_ = static_cast<size_t>(mem_size);
        
        initialized_ = true;
        return true;
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка OpenCL при инициализации: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

void OpenCLBackend::Cleanup() {
    if (!initialized_) {
        return;
    }
    
    // OpenCL автоматически освобождает ресурсы при уничтожении объектов
    initialized_ = false;
}

void* OpenCLBackend::AllocateDeviceMemory(size_t size_bytes) {
    if (!initialized_) {
        std::cerr << "Ошибка: backend не инициализирован" << std::endl;
        return nullptr;
    }
    
    try {
        cl::Buffer* buffer = new cl::Buffer(
            context_,
            CL_MEM_READ_WRITE,
            size_bytes
        );
        return static_cast<void*>(buffer);
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при выделении памяти: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return nullptr;
    }
}

void OpenCLBackend::FreeDeviceMemory(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    
    cl::Buffer* buffer = static_cast<cl::Buffer*>(ptr);
    delete buffer;
}

bool OpenCLBackend::CopyHostToDevice(void* dst, const void* src, size_t size_bytes) {
    if (!initialized_ || dst == nullptr || src == nullptr) {
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(dst);
        cl_int err = queue_.enqueueWriteBuffer(
            *buffer,
            CL_TRUE,  // blocking
            0,
            size_bytes,
            src
        );
        return CheckError(err, "копирование H2D");
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при копировании H2D: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::CopyDeviceToHost(void* dst, const void* src, size_t size_bytes) {
    if (!initialized_ || dst == nullptr || src == nullptr) {
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(src);
        cl_int err = queue_.enqueueReadBuffer(
            *buffer,
            CL_TRUE,  // blocking
            0,
            size_bytes,
            dst
        );
        return CheckError(err, "копирование D2H");
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при копировании D2H: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::ExecuteFractionalDelay(
    void* device_buffer,
    const float* delay_coefficients,
    size_t num_beams,
    size_t num_samples) {
    
    if (!initialized_ || device_buffer == nullptr || delay_coefficients == nullptr) {
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(device_buffer);
        
        // Создаём буфер для коэффициентов задержки
        cl::Buffer delay_buf(
            context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            num_beams * sizeof(float),
            const_cast<float*>(delay_coefficients)
        );
        
        // Устанавливаем аргументы kernel
        cl_int err = kernel_fractional_delay_.setArg(0, *buffer);
        err |= kernel_fractional_delay_.setArg(1, delay_buf);
        err |= kernel_fractional_delay_.setArg(2, static_cast<cl_uint>(num_beams));
        err |= kernel_fractional_delay_.setArg(3, static_cast<cl_uint>(num_samples));
        
        if (!CheckError(err, "установка аргументов fractional_delay")) {
            return false;
        }
        
        // Запускаем kernel
        size_t global_size = num_beams * num_samples;
        err = queue_.enqueueNDRangeKernel(
            kernel_fractional_delay_,
            cl::NullRange,
            cl::NDRange(global_size),
            cl::NullRange
        );
        
        if (!CheckError(err, "запуск kernel fractional_delay")) {
            return false;
        }
        
        // Синхронизируем
        queue_.finish();
        return true;
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при выполнении fractional_delay: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::ExecuteFFT(
    void* device_buffer,
    size_t num_beams,
    size_t num_samples,
    bool forward) {
    
    // TODO: Реализация будет добавлена после интеграции FFT библиотеки
    std::cerr << "ExecuteFFT: пока не реализовано" << std::endl;
    return false;
}

bool OpenCLBackend::ExecuteHadamardMultiply(
    void* device_buffer,
    const void* reference_fft,
    size_t num_beams,
    size_t num_samples) {
    
    if (!initialized_ || device_buffer == nullptr || reference_fft == nullptr) {
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(device_buffer);
        cl::Buffer* ref_buffer = static_cast<cl::Buffer*>(const_cast<void*>(reference_fft));
        
        // Устанавливаем аргументы kernel
        cl_int err = kernel_hadamard_.setArg(0, *buffer);
        err |= kernel_hadamard_.setArg(1, *ref_buffer);
        err |= kernel_hadamard_.setArg(2, static_cast<cl_uint>(num_beams));
        err |= kernel_hadamard_.setArg(3, static_cast<cl_uint>(num_samples));
        
        if (!CheckError(err, "установка аргументов hadamard_multiply")) {
            return false;
        }
        
        // Запускаем kernel
        size_t global_size = num_beams * num_samples;
        err = queue_.enqueueNDRangeKernel(
            kernel_hadamard_,
            cl::NullRange,
            cl::NDRange(global_size),
            cl::NullRange
        );
        
        if (!CheckError(err, "запуск kernel hadamard_multiply")) {
            return false;
        }
        
        // Синхронизируем
        queue_.finish();
        return true;
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при выполнении hadamard_multiply: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

std::string OpenCLBackend::GetBackendName() const {
    return "OpenCL";
}

std::string OpenCLBackend::GetDeviceName() const {
    return device_name_;
}

size_t OpenCLBackend::GetDeviceMemorySize() const {
    return device_memory_size_;
}

bool OpenCLBackend::SelectDevice() {
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        
        if (platforms.empty()) {
            std::cerr << "Ошибка: не найдено OpenCL платформ" << std::endl;
            return false;
        }
        
        // Ищем GPU устройства, приоритет NVIDIA RTX3060
        cl::Device selected_device;
        bool found = false;
        
        for (const auto& plat : platforms) {
            std::vector<cl::Device> devices;
            plat.getDevices(CL_DEVICE_TYPE_GPU, &devices);
            
            for (const auto& dev : devices) {
                std::string dev_name;
                dev.getInfo(CL_DEVICE_NAME, &dev_name);
                
                // Приоритет: NVIDIA RTX3060
                if (dev_name.find("RTX 3060") != std::string::npos ||
                    dev_name.find("GeForce RTX 3060") != std::string::npos) {
                    selected_device = dev;
                    platform_ = plat;
                    found = true;
                    break;
                }
                
                // Если ещё не выбрано, берём первое доступное GPU
                if (!found) {
                    selected_device = dev;
                    platform_ = plat;
                    found = true;
                }
            }
            
            if (found && (selected_device.getInfo<CL_DEVICE_NAME>().find("RTX 3060") != std::string::npos)) {
                break;
            }
        }
        
        if (!found) {
            std::cerr << "Ошибка: не найдено GPU устройств" << std::endl;
            return false;
        }
        
        device_ = selected_device;
        std::string dev_name = device_.getInfo<CL_DEVICE_NAME>();
        std::cout << "Выбрано устройство: " << dev_name << std::endl;
        
        return true;
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при выборе устройства: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::BuildProgram() {
    try {
        // Загружаем источники kernel'ов
        std::string kernel_source = LoadKernelSource("kernel_fractional_delay.cl");
        kernel_source += "\n" + LoadKernelSource("kernel_hadamard.cl");
        
        if (kernel_source.empty()) {
            std::cerr << "Ошибка: не удалось загрузить kernel источники" << std::endl;
            return false;
        }
        
        // Создаём программу
        cl::Program::Sources sources;
        sources.push_back({kernel_source.c_str(), kernel_source.length()});
        program_ = cl::Program(context_, sources);
        
        // Компилируем
        cl_int err = program_.build({device_});
        if (err != CL_SUCCESS) {
            std::string build_log;
            program_.getBuildInfo(device_, CL_PROGRAM_BUILD_LOG, &build_log);
            std::cerr << "Ошибка компиляции OpenCL программы:\n" << build_log << std::endl;
            return false;
        }
        
        // Создаём kernel объекты
        kernel_fractional_delay_ = cl::Kernel(program_, "fractional_delay", &err);
        if (!CheckError(err, "создание kernel fractional_delay")) {
            return false;
        }
        
        kernel_hadamard_ = cl::Kernel(program_, "hadamard_multiply", &err);
        if (!CheckError(err, "создание kernel hadamard_multiply")) {
            return false;
        }
        
        return true;
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка при компиляции программы: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

std::string OpenCLBackend::LoadKernelSource(const std::string& filename) const {
    // Пробуем несколько путей
    std::vector<std::string> possible_paths = {
        "kernels/" + filename,
        "../kernels/" + filename,
        "../../kernels/" + filename,
        std::string(OPENCL_KERNEL_DIR) + "/" + filename
    };
    
    for (const auto& path : possible_paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            return buffer.str();
        }
    }
    
    std::cerr << "Ошибка: не удалось найти kernel файл " << filename << std::endl;
    std::cerr << "Пробовались пути:" << std::endl;
    for (const auto& path : possible_paths) {
        std::cerr << "  - " << path << std::endl;
    }
    return "";
}

bool OpenCLBackend::CheckError(cl_int err, const std::string& context) const {
    if (err != CL_SUCCESS) {
        std::cerr << "Ошибка OpenCL в " << context << ": код " << err << std::endl;
        return false;
    }
    return true;
}

