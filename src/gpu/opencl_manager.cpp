#include "opencl_manager.h"
#include <functional>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace radar {
namespace gpu {

// ═══════════════════════════════════════════════════════════════════
// DESTRUCTOR
// ═══════════════════════════════════════════════════════════════════

OpenCLManager::~OpenCLManager() {
    // Освобождение программ из кэша
    for (auto& [hash, program] : program_cache_) {
        if (program != nullptr) {
            clReleaseProgram(program);
        }
    }
    program_cache_.clear();
    
    // Освобождение OpenCL ресурсов
    if (queue_ != nullptr) {
        clReleaseCommandQueue(queue_);
        queue_ = nullptr;
    }
    
    if (context_ != nullptr) {
        clReleaseContext(context_);
        context_ = nullptr;
    }
    
    std::cout << "✓ OpenCLManager destructed (resources cleaned up)" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════
// SINGLETON ACCESS
// ═══════════════════════════════════════════════════════════════════

OpenCLManager& OpenCLManager::GetInstance() {
    // C++11 гарантирует thread-safe инициализацию static local
    static OpenCLManager instance;
    return instance;
}

// ═══════════════════════════════════════════════════════════════════
// INITIALIZATION
// ═══════════════════════════════════════════════════════════════════

void OpenCLManager::Initialize(cl_device_type device_type) {
    OpenCLManager& mgr = GetInstance();
    
    if (mgr.is_initialized_) {
        std::cout << "⚠ OpenCLManager already initialized, skipping" << std::endl;
        return;
    }
    
    try {
        mgr.InitializeOpenCL(device_type);
        mgr.is_initialized_ = true;
        
        std::cout << "✓ OpenCLManager initialized successfully" << std::endl;
        std::cout << "  Device: " << mgr.GetDeviceName() << std::endl;
        std::cout << "  Memory: " << mgr.GetDeviceMemoryMB() << " MB" << std::endl;
        std::cout << "  Compute Units: " << mgr.GetComputeUnits() << std::endl;
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("OpenCLManager::Initialize failed: ") + e.what());
    }
}

// ═══════════════════════════════════════════════════════════════════
// PRIVATE: OPENCL INITIALIZATION
// ═══════════════════════════════════════════════════════════════════

void OpenCLManager::InitializeOpenCL(cl_device_type device_type) {
    cl_int err = CL_SUCCESS;
    
    device_type_ = device_type;
    
    // 1️⃣ Получить платформы
    cl_uint num_platforms = 0;
    err = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        throw std::runtime_error("No OpenCL platforms found");
    }
    
    std::vector<cl_platform_id> platforms(num_platforms);
    err = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to get platform IDs");
    }
    
    platform_ = platforms[0];
    
    // 2️⃣ Получить устройства
    cl_uint num_devices = 0;
    err = clGetDeviceIDs(platform_, device_type, 0, nullptr, &num_devices);
    if (err != CL_SUCCESS || num_devices == 0) {
        throw std::runtime_error("No OpenCL devices found for specified type");
    }
    
    std::vector<cl_device_id> devices(num_devices);
    err = clGetDeviceIDs(platform_, device_type, num_devices, devices.data(), nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to get device IDs");
    }
    
    device_ = devices[0];
    
    // 3️⃣ Создать контекст
    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to create OpenCL context");
    }
    
    // 4️⃣ Создать очередь команд
    queue_ = clCreateCommandQueue(context_, device_, CL_QUEUE_PROFILING_ENABLE, &err);
    if (err != CL_SUCCESS) {
        clReleaseContext(context_);
        context_ = nullptr;
        throw std::runtime_error("Failed to create OpenCL command queue");
    }
}

// ═══════════════════════════════════════════════════════════════════
// PROGRAM COMPILATION & CACHING
// ═══════════════════════════════════════════════════════════════════

std::string OpenCLManager::GetSourceHash(const std::string& source) {
    // Простой хеш: использовать std::hash
    std::hash<std::string> hasher;
    size_t hash = hasher(source);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

cl_program OpenCLManager::CompileProgram(const std::string& source) {
    if (!is_initialized_) {
        throw std::runtime_error("OpenCLManager not initialized. Call Initialize() first.");
    }
    
    cl_int err = CL_SUCCESS;
    
    const char* source_str = source.c_str();
    size_t source_len = source.length();
    
    // Создать программу
    cl_program program = clCreateProgramWithSource(context_, 1, &source_str, &source_len, &err);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Failed to create OpenCL program");
    }
    
    // Скомпилировать
    err = clBuildProgram(program, 1, &device_, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // Получить лог ошибки
        size_t log_size = 0;
        clGetProgramBuildInfo(program, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        
        std::string log(log_size, '\0');
        clGetProgramBuildInfo(program, device_, CL_PROGRAM_BUILD_LOG, log_size, &log[0], nullptr);
        
        clReleaseProgram(program);
        
        throw std::runtime_error("OpenCL program build failed:\n" + log);
    }
    
    return program;
}

cl_program OpenCLManager::GetOrCompileProgram(const std::string& source) {
    std::string hash = GetSourceHash(source);
    
    {
        std::unique_lock<std::mutex> lock(cache_mutex_);
        
        // Проверить кэш
        auto it = program_cache_.find(hash);
        if (it != program_cache_.end()) {
            // Cache hit!
            return it->second;
        }
    }
    
    // Cache miss - компилировать (без блокировки mutex, так как операция долгая)
    cl_program program = CompileProgram(source);
    
    // Сохранить в кэш
    {
        std::unique_lock<std::mutex> lock(cache_mutex_);
        program_cache_[hash] = program;
    }
    
    return program;
}

// ═══════════════════════════════════════════════════════════════════
// DEVICE INFORMATION
// ═══════════════════════════════════════════════════════════════════

std::string OpenCLManager::GetDeviceName() const {
    if (!is_initialized_) return "Not initialized";
    
    char device_name[256] = {0};
    clGetDeviceInfo(device_, CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
    return std::string(device_name);
}

size_t OpenCLManager::GetDeviceMemoryMB() const {
    if (!is_initialized_) return 0;
    
    cl_ulong mem_size = 0;
    clGetDeviceInfo(device_, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem_size), &mem_size, nullptr);
    return mem_size / (1024 * 1024);
}

size_t OpenCLManager::GetComputeUnits() const {
    if (!is_initialized_) return 0;
    
    cl_uint cu = 0;
    clGetDeviceInfo(device_, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, nullptr);
    return cu;
}

std::string OpenCLManager::GetDeviceInfo() const {
    if (!is_initialized_) return "OpenCL not initialized";
    
    std::ostringstream oss;
    
    char device_name[256] = {0};
    clGetDeviceInfo(device_, CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
    
    char vendor[256] = {0};
    clGetDeviceInfo(device_, CL_DEVICE_VENDOR, sizeof(vendor), vendor, nullptr);
    
    cl_ulong mem_size = 0;
    clGetDeviceInfo(device_, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem_size), &mem_size, nullptr);
    
    cl_uint cu = 0;
    clGetDeviceInfo(device_, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cu), &cu, nullptr);
    
    size_t max_workgroup = 0;
    clGetDeviceInfo(device_, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_workgroup), &max_workgroup, nullptr);
    
    oss << "╔════════════════════════════════════════╗" << std::endl;
    oss << "║        OpenCL Device Information       ║" << std::endl;
    oss << "╠════════════════════════════════════════╣" << std::endl;
    oss << "║ Device: " << std::setw(31) << std::left << device_name << "║" << std::endl;
    oss << "║ Vendor: " << std::setw(31) << std::left << vendor << "║" << std::endl;
    oss << "║ Global Memory: " << std::setw(25) << std::left << (mem_size / (1024*1024)) << " MB║" << std::endl;
    oss << "║ Compute Units: " << std::setw(25) << std::left << cu << "║" << std::endl;
    oss << "║ Max Work Group Size: " << std::setw(19) << std::left << max_workgroup << "║" << std::endl;
    oss << "╚════════════════════════════════════════╝" << std::endl;
    
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════
// CLEANUP
// ═══════════════════════════════════════════════════════════════════

void OpenCLManager::Cleanup() {
    OpenCLManager& mgr = GetInstance();
    // Деструктор вызовется автоматически при выходе из программы
    // Этот метод для явного управления (опционально)
}

void OpenCLManager::ClearProgramCache() {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    
    for (auto& [hash, program] : program_cache_) {
        if (program != nullptr) {
            clReleaseProgram(program);
        }
    }
    program_cache_.clear();
    
    std::cout << "✓ Program cache cleared" << std::endl;
}

std::string OpenCLManager::GetCacheStatistics() const {
    std::unique_lock<std::mutex> lock(cache_mutex_);
    
    std::ostringstream oss;
    oss << "Program Cache Statistics:" << std::endl;
    oss << "  Total programs cached: " << program_cache_.size() << std::endl;
    oss << "  Cache size estimate: " << (program_cache_.size() * 50) << " KB" << std::endl;
    
    return oss.str();
}

} // namespace gpu
} // namespace radar
