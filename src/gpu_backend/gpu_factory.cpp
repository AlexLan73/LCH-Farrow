#include "gpu_backend/gpu_factory.h"
#include "gpu_backend/opencl_backend.h"
#include <CL/cl.hpp>
#include <iostream>

std::unique_ptr<IGPUBackend> GPUFactory::CreateBackend() {
    // Приоритет: OpenCL
    if (IsOpenCLAvailable()) {
        return CreateOpenCLBackend();
    }
    
    std::cerr << "Ошибка: не найдено доступных GPU backend" << std::endl;
    return nullptr;
}

std::unique_ptr<IGPUBackend> GPUFactory::CreateOpenCLBackend() {
    auto backend = std::make_unique<OpenCLBackend>();
    if (backend->Initialize()) {
        return backend;
    }
    return nullptr;
}

bool GPUFactory::IsOpenCLAvailable() {
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        
        if (platforms.empty()) {
            return false;
        }
        
        // Проверяем наличие устройств
        for (const auto& platform : platforms) {
            std::vector<cl::Device> devices;
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
            if (!devices.empty()) {
                return true;
            }
        }
        
        return false;
    } catch (const cl::Error& e) {
        std::cerr << "Ошибка OpenCL при проверке доступности: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

