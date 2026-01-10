#include "gpu_backend/opencl_backend.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

OpenCLBackend::OpenCLBackend()
    : device_memory_size_(0), initialized_(false)
#if CLFFT_FOUND
    , fft_plan_forward_(0), fft_plan_inverse_(0), fft_plans_created_(false)
#endif
    , lagrange_matrix_uploaded_(false)
{
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
        
        // Инициализируем clFFT
#if CLFFT_FOUND
        cl_int clfft_err = clfftSetup(nullptr);
        if (clfft_err != CLFFT_SUCCESS) {
            std::cerr << "Ошибка инициализации clFFT: " << clfft_err << std::endl;
            return false;
        }
#endif
        
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
    } catch (cl::Error& e) {
        std::cerr << "Ошибка OpenCL при инициализации: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

void OpenCLBackend::Cleanup() {
    if (!initialized_) {
        return;
    }
    
    // Удаляем FFT планы
    DestroyFFTPlans();
    
    // Освобождаем матрицу Лагранжа
    if (lagrange_matrix_uploaded_) {
        lagrange_matrix_buffer_ = cl::Buffer();  // Освобождаем
        lagrange_matrix_uploaded_ = false;
    }
    
#if CLFFT_FOUND
    // Завершаем работу clFFT
    clfftTeardown();
#endif
    
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
    } catch (cl::Error& e) {
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
    } catch (cl::Error& e) {
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
        cl::Buffer* buffer = static_cast<cl::Buffer*>(const_cast<void*>(src));
        cl_int err = queue_.enqueueReadBuffer(
            *buffer,
            CL_TRUE,  // blocking
            0,
            size_bytes,
            dst
        );
        return CheckError(err, "копирование D2H");
    } catch (cl::Error& e) {
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
    
    if (!lagrange_matrix_uploaded_) {
        std::cerr << "Ошибка: матрица Лагранжа не загружена на GPU" << std::endl;
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(device_buffer);
        
        // Создаём буфер для коэффициентов задержки (целая и дробная часть)
        // Для каждого луча нужно: delay_integer и lagrange_row
        struct DelayParams {
            int delay_integer;
            int lagrange_row;
        };
        
        std::vector<DelayParams> delay_params(num_beams);
        const size_t LAGRANGE_ROWS = 48;
        
        for (size_t beam = 0; beam < num_beams; ++beam) {
            float delay = delay_coefficients[beam];
            delay_params[beam].delay_integer = static_cast<int>(std::floor(delay));
            float delay_fraction = delay - delay_params[beam].delay_integer;
            if (delay_fraction < 0.0f) {
                delay_fraction += 1.0f;
                delay_params[beam].delay_integer -= 1;
            }
            delay_params[beam].lagrange_row = static_cast<int>(delay_fraction * LAGRANGE_ROWS);
            if (delay_params[beam].lagrange_row >= static_cast<int>(LAGRANGE_ROWS)) {
                delay_params[beam].lagrange_row = LAGRANGE_ROWS - 1;
            }
        }
        
        cl::Buffer delay_params_buf(
            context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            num_beams * sizeof(DelayParams),
            delay_params.data()
        );
        
        // Устанавливаем аргументы kernel
        cl_int err = kernel_fractional_delay_.setArg(0, *buffer);
        err |= kernel_fractional_delay_.setArg(1, *buffer);  // in-place: input = output
        err |= kernel_fractional_delay_.setArg(2, lagrange_matrix_buffer_);
        err |= kernel_fractional_delay_.setArg(3, delay_params_buf);
        err |= kernel_fractional_delay_.setArg(4, static_cast<cl_uint>(num_beams));
        err |= kernel_fractional_delay_.setArg(5, static_cast<cl_uint>(num_samples));
        
        if (!CheckError(err, "установка аргументов fractional_delay")) {
            return false;
        }
        
        // Запускаем kernel (1D grid для совместимости)
        // Каждый work item обрабатывает один отсчёт одного луча
        size_t global_size = num_beams * num_samples;
        
        // Определяем оптимальный размер work group
        size_t preferred_work_group_size = 256;  // Оптимально для RTX 3060
        size_t work_group_size = std::min(preferred_work_group_size, global_size);
        
        err = queue_.enqueueNDRangeKernel(
            kernel_fractional_delay_,
            cl::NullRange,
            cl::NDRange(global_size),
            cl::NDRange(work_group_size)
        );
        
        if (!CheckError(err, "запуск kernel fractional_delay")) {
            return false;
        }
        
        // Синхронизируем
        queue_.finish();
        return true;
    } catch (cl::Error& e) {
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
    
#if CLFFT_FOUND
    if (!initialized_ || device_buffer == nullptr) {
        return false;
    }
    
    cl::Buffer* buffer = static_cast<cl::Buffer*>(device_buffer);
    cl_mem cl_buffer = (*buffer)();
    
    // Создаём планы если ещё не созданы
    if (!fft_plans_created_) {
        if (!CreateFFTPlans(num_samples, num_beams)) {
            return false;
        }
    }
    
    clfftPlanHandle plan = forward ? fft_plan_forward_ : fft_plan_inverse_;
    clfftDirection dir = forward ? CLFFT_FORWARD : CLFFT_BACKWARD;
    
    cl_int err = clfftEnqueueTransform(
        plan,
        dir,
        1,
        &queue_(),
        0,
        nullptr,
        nullptr,
        &cl_buffer,
        nullptr,
        nullptr
    );
    
    if (!CheckError(err, forward ? "clFFT forward" : "clFFT inverse")) {
        return false;
    }
    
    queue_.finish();
    return true;
#else
    // Fallback: используем CPU FFT (медленно, но работает)
    std::cerr << "Предупреждение: clFFT не найдена, используем CPU FFT (медленно!)" << std::endl;
    return false;
#endif
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
    } catch (cl::Error& e) {
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

OpenCLBackend::SystemInfo OpenCLBackend::GetSystemInfo() const {
    SystemInfo info;
    
    if (!initialized_) {
        return info;
    }
    
    try {
        // GPU информация
        device_.getInfo(CL_DEVICE_NAME, &info.device_name);
        device_.getInfo(CL_DEVICE_VENDOR, &info.device_vendor);
        device_.getInfo(CL_DEVICE_VERSION, &info.device_version);  // OpenCL API версия (например, "OpenCL 3.0 CUDA")
        device_.getInfo(CL_DRIVER_VERSION, &info.driver_version);
        device_.getInfo(CL_DEVICE_OPENCL_C_VERSION, &info.opencl_c_version);  // OpenCL C язык версия (например, "OpenCL C 1.2")
        
        // Проверка поддержки OpenCL C 3.0 (если доступно)
        bool supports_opencl_c_30 = false;
        if (info.opencl_c_version.find("3.0") != std::string::npos) {
            supports_opencl_c_30 = true;
        }
        
        // Попытка получить расширенную информацию (для OpenCL 3.0)
        #ifdef CL_VERSION_3_0
        try {
            std::vector<cl_name_version> extensions;
            device_.getInfo(CL_DEVICE_EXTENSIONS_WITH_VERSION, &extensions);
            // Можно проверить расширения для OpenCL C 3.0
        } catch (...) {
            // Игнорируем ошибки для старых версий
        }
        #endif
        
        cl_ulong mem_size;
        device_.getInfo(CL_DEVICE_GLOBAL_MEM_SIZE, &mem_size);
        info.device_memory_mb = static_cast<size_t>(mem_size / (1024 * 1024));
        
        size_t max_wg_size;
        device_.getInfo(CL_DEVICE_MAX_WORK_GROUP_SIZE, &max_wg_size);
        info.max_work_group_size = max_wg_size;
        
        cl_uint compute_units;
        device_.getInfo(CL_DEVICE_MAX_COMPUTE_UNITS, &compute_units);
        info.compute_units = static_cast<size_t>(compute_units);
        
        // Платформа информация
        platform_.getInfo(CL_PLATFORM_NAME, &info.platform_name);
        platform_.getInfo(CL_PLATFORM_VERSION, &info.platform_version);
        
        // Операционная система (через OpenCL)
        #ifdef _WIN32
        info.os_name = "Windows";
        // Можно попробовать получить версию через WinAPI, но пока пропустим
        info.os_version = "Windows";
        #elif defined(__linux__)
        info.os_name = "Linux";
        info.os_version = "Linux";
        #elif defined(__APPLE__)
        info.os_name = "macOS";
        info.os_version = "macOS";
        #else
        info.os_name = "Unknown";
        info.os_version = "Unknown";
        #endif
        
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при получении системной информации: " << e.what() << std::endl;
    }
    
    return info;
}

bool OpenCLBackend::CopyHostToDeviceWithProfiling(
    void* dst, 
    const void* src, 
    size_t size_bytes,
    cl::Event& event_out) {
    
    if (!initialized_ || dst == nullptr || src == nullptr) {
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(dst);
        cl_int err = queue_.enqueueWriteBuffer(
            *buffer,
            CL_FALSE,  // non-blocking для профилирования
            0,
            size_bytes,
            src,
            nullptr,
            &event_out
        );
        return CheckError(err, "копирование H2D с профилированием");
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при копировании H2D с профилированием: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::ExecuteFractionalDelayWithProfiling(
    void* device_buffer,
    const float* delay_coefficients,
    size_t num_beams,
    size_t num_samples,
    cl::Event& event_out) {
    
    if (!initialized_ || device_buffer == nullptr || delay_coefficients == nullptr) {
        return false;
    }
    
    if (!lagrange_matrix_uploaded_) {
        std::cerr << "Ошибка: матрица Лагранжа не загружена на GPU" << std::endl;
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(device_buffer);
        
        // Создаём буфер для коэффициентов задержки (как в обычной версии)
        struct DelayParams {
            int delay_integer;
            int lagrange_row;
        };
        
        std::vector<DelayParams> delay_params(num_beams);
        const size_t LAGRANGE_ROWS = 48;
        
        for (size_t beam = 0; beam < num_beams; ++beam) {
            float delay = delay_coefficients[beam];
            delay_params[beam].delay_integer = static_cast<int>(std::floor(delay));
            float delay_fraction = delay - delay_params[beam].delay_integer;
            if (delay_fraction < 0.0f) {
                delay_fraction += 1.0f;
                delay_params[beam].delay_integer -= 1;
            }
            delay_params[beam].lagrange_row = static_cast<int>(delay_fraction * LAGRANGE_ROWS);
            if (delay_params[beam].lagrange_row >= static_cast<int>(LAGRANGE_ROWS)) {
                delay_params[beam].lagrange_row = LAGRANGE_ROWS - 1;
            }
        }
        
        cl::Buffer delay_params_buf(
            context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            num_beams * sizeof(DelayParams),
            delay_params.data()
        );
        
        // Устанавливаем аргументы kernel
        cl_int err = kernel_fractional_delay_.setArg(0, *buffer);
        err |= kernel_fractional_delay_.setArg(1, *buffer);
        err |= kernel_fractional_delay_.setArg(2, lagrange_matrix_buffer_);
        err |= kernel_fractional_delay_.setArg(3, delay_params_buf);
        err |= kernel_fractional_delay_.setArg(4, static_cast<cl_uint>(num_beams));
        err |= kernel_fractional_delay_.setArg(5, static_cast<cl_uint>(num_samples));
        
        if (!CheckError(err, "установка аргументов fractional_delay")) {
            return false;
        }
        
        // Запускаем kernel с профилированием
        size_t global_size = num_beams * num_samples;
        size_t preferred_work_group_size = 256;
        size_t work_group_size = std::min(preferred_work_group_size, global_size);
        
        err = queue_.enqueueNDRangeKernel(
            kernel_fractional_delay_,
            cl::NullRange,
            cl::NDRange(global_size),
            cl::NDRange(work_group_size),
            nullptr,
            &event_out
        );
        
        return CheckError(err, "запуск kernel fractional_delay с профилированием");
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при выполнении fractional_delay с профилированием: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::CopyDeviceToHostWithProfiling(
    void* dst, 
    const void* src, 
    size_t size_bytes,
    cl::Event& event_out) {
    
    if (!initialized_ || dst == nullptr || src == nullptr) {
        return false;
    }
    
    try {
        cl::Buffer* buffer = static_cast<cl::Buffer*>(const_cast<void*>(src));
        cl_int err = queue_.enqueueReadBuffer(
            *buffer,
            CL_FALSE,  // non-blocking для профилирования
            0,
            size_bytes,
            dst,
            nullptr,
            &event_out
        );
        return CheckError(err, "копирование D2H с профилированием");
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при копировании D2H с профилированием: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
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
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при выборе устройства: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

bool OpenCLBackend::BuildProgram() {
    try {
        // Проверяем версию OpenCL C
        std::string opencl_c_version;
        device_.getInfo(CL_DEVICE_OPENCL_C_VERSION, &opencl_c_version);
        bool try_opencl_c_30 = (opencl_c_version.find("3.0") != std::string::npos);
        
        std::cout << "Версия OpenCL C устройства: " << opencl_c_version << std::endl;
        
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
        
        // Опции компиляции: пробуем использовать OpenCL C 3.0, если поддерживается
        std::string build_options;
        if (try_opencl_c_30) {
            // Пробуем OpenCL C 3.0 с оптимизациями
            build_options = "-cl-std=CL3.0 -cl-fast-relaxed-math -cl-mad-enable";
            std::cout << "Попытка компиляции с OpenCL C 3.0 и оптимизациями..." << std::endl;
        } else {
            // OpenCL C 1.2 с оптимизациями
            build_options = "-cl-std=CL1.2 -cl-fast-relaxed-math -cl-mad-enable";
            std::cout << "Компиляция с OpenCL C 1.2 (устройство поддерживает: " << opencl_c_version << ")" << std::endl;
        }
        
        // Компилируем
        cl_int err = program_.build({device_}, build_options.c_str());
        if (err != CL_SUCCESS) {
            std::string build_log;
            program_.getBuildInfo(device_, CL_PROGRAM_BUILD_LOG, &build_log);
            std::cerr << "Ошибка компиляции OpenCL программы:\n" << build_log << std::endl;
            
            // Если не удалось с OpenCL C 3.0, пробуем 1.2
            if (try_opencl_c_30) {
                std::cerr << "\n⚠️  Не удалось скомпилировать с OpenCL C 3.0, пробуем OpenCL C 1.2..." << std::endl;
                build_options = "-cl-std=CL1.2 -cl-fast-relaxed-math -cl-mad-enable";
                err = program_.build({device_}, build_options.c_str());
                if (err != CL_SUCCESS) {
                    std::string build_log2;
                    program_.getBuildInfo(device_, CL_PROGRAM_BUILD_LOG, &build_log2);
                    std::cerr << "Ошибка компиляции с OpenCL C 1.2:\n" << build_log2 << std::endl;
                    return false;
                }
                std::cout << "✅ Компиляция успешна с OpenCL C 1.2" << std::endl;
            } else {
                return false;
            }
        } else {
            if (try_opencl_c_30) {
                std::cout << "✅ Компиляция успешна с OpenCL C 3.0!" << std::endl;
            } else {
                std::cout << "✅ Компиляция успешна с OpenCL C 1.2" << std::endl;
            }
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
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при компиляции программы: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

std::string OpenCLBackend::LoadKernelSource(const std::string& filename) const {
    // Используем абсолютный путь через OPENCL_KERNEL_DIR из CMake
    std::string kernel_dir = OPENCL_KERNEL_DIR;
    std::string full_path = kernel_dir + "/" + filename;
    
    // Пробуем абсолютный путь
    std::ifstream file(full_path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return buffer.str();
    }
    
    // Если не получилось, пробуем относительные пути
    std::vector<std::string> fallback_paths = {
        "kernels/" + filename,
        "../kernels/" + filename,
        "../../kernels/" + filename
    };
    
    for (const auto& path : fallback_paths) {
        std::ifstream fallback_file(path);
        if (fallback_file.is_open()) {
            std::stringstream buffer;
            buffer << fallback_file.rdbuf();
            fallback_file.close();
            std::cout << "Kernel загружен из: " << path << std::endl;
            return buffer.str();
        }
    }
    
    std::cerr << "Ошибка: не удалось найти kernel файл " << filename << std::endl;
    std::cerr << "Пробовались пути:" << std::endl;
    std::cerr << "  - " << full_path << std::endl;
    for (const auto& path : fallback_paths) {
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

bool OpenCLBackend::CreateFFTPlans(size_t num_samples, size_t num_beams) {
#if CLFFT_FOUND
    if (fft_plans_created_) {
        return true;  // Планы уже созданы
    }
    
    cl_context cl_ctx = context_();
    cl_command_queue cl_queue = queue_();
    
    size_t clLengths[1] = {num_samples};
    size_t strides[1] = {1};
    size_t dist = num_samples;
    
    // Создаём план для forward FFT
    cl_int err = clfftCreateDefaultPlan(&fft_plan_forward_, cl_ctx, CLFFT_1D, clLengths);
    if (err != CLFFT_SUCCESS) {
        std::cerr << "Ошибка создания forward FFT плана: " << err << std::endl;
        return false;
    }
    
    clfftSetPlanPrecision(fft_plan_forward_, CLFFT_SINGLE);
    clfftSetLayout(fft_plan_forward_, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
    clfftSetResultLocation(fft_plan_forward_, CLFFT_INPLACE);
    clfftSetPlanBatchSize(fft_plan_forward_, num_beams);
    clfftSetPlanInStride(fft_plan_forward_, CLFFT_1D, strides);
    clfftSetPlanOutStride(fft_plan_forward_, CLFFT_1D, strides);
    clfftSetPlanDistance(fft_plan_forward_, dist, dist);
    
    err = clfftBakePlan(fft_plan_forward_, 1, &cl_queue, nullptr, nullptr);
    if (err != CLFFT_SUCCESS) {
        std::cerr << "Ошибка компиляции forward FFT плана: " << err << std::endl;
        clfftDestroyPlan(&fft_plan_forward_);
        return false;
    }
    
    // Создаём план для inverse FFT (аналогично)
    err = clfftCreateDefaultPlan(&fft_plan_inverse_, cl_ctx, CLFFT_1D, clLengths);
    if (err != CLFFT_SUCCESS) {
        std::cerr << "Ошибка создания inverse FFT плана: " << err << std::endl;
        clfftDestroyPlan(&fft_plan_forward_);
        return false;
    }
    
    clfftSetPlanPrecision(fft_plan_inverse_, CLFFT_SINGLE);
    clfftSetLayout(fft_plan_inverse_, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
    clfftSetResultLocation(fft_plan_inverse_, CLFFT_INPLACE);
    clfftSetPlanBatchSize(fft_plan_inverse_, num_beams);
    clfftSetPlanInStride(fft_plan_inverse_, CLFFT_1D, strides);
    clfftSetPlanOutStride(fft_plan_inverse_, CLFFT_1D, strides);
    clfftSetPlanDistance(fft_plan_inverse_, dist, dist);
    
    err = clfftBakePlan(fft_plan_inverse_, 1, &cl_queue, nullptr, nullptr);
    if (err != CLFFT_SUCCESS) {
        std::cerr << "Ошибка компиляции inverse FFT плана: " << err << std::endl;
        clfftDestroyPlan(&fft_plan_forward_);
        clfftDestroyPlan(&fft_plan_inverse_);
        return false;
    }
    
    fft_plans_created_ = true;
    return true;
#else
    return false;
#endif
}

void OpenCLBackend::DestroyFFTPlans() {
#if CLFFT_FOUND
    if (fft_plans_created_) {
        if (fft_plan_forward_ != 0) {
            clfftDestroyPlan(&fft_plan_forward_);
            fft_plan_forward_ = 0;
        }
        if (fft_plan_inverse_ != 0) {
            clfftDestroyPlan(&fft_plan_inverse_);
            fft_plan_inverse_ = 0;
        }
        fft_plans_created_ = false;
    }
#endif
}

bool OpenCLBackend::UploadLagrangeMatrix(const float* lagrange_data) {
    if (!initialized_ || lagrange_data == nullptr) {
        return false;
    }
    
    try {
        const size_t LAGRANGE_ROWS = 48;
        const size_t LAGRANGE_COLS = 5;
        size_t matrix_size = LAGRANGE_ROWS * LAGRANGE_COLS * sizeof(float);
        
        lagrange_matrix_buffer_ = cl::Buffer(
            context_,
            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            matrix_size,
            const_cast<float*>(lagrange_data)
        );
        
        lagrange_matrix_uploaded_ = true;
        return true;
    } catch (cl::Error& e) {
        std::cerr << "Ошибка при загрузке матрицы Лагранжа: " << e.what() 
                  << " (код: " << e.err() << ")" << std::endl;
        return false;
    }
}

