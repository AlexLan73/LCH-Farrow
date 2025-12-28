cmake_minimum_required(VERSION 3.20)
project(LCH-Farrow VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# ЧАСТЬ 0: ДЕТЕКЦИЯ GPU И CUDA
# ============================================================================

# Параметры для выбора GPU
option(ENABLE_CUDA "Enable CUDA support" ON)
option(ENABLE_OPENCL "Enable OpenCL support" OFF)
option(CUDA_ARCH "CUDA architecture (default: auto-detect)" "")

message(STATUS "")
message(STATUS "🎮 GPU SUPPORT OPTIONS:")
message(STATUS "   ENABLE_CUDA: ${ENABLE_CUDA}")
message(STATUS "   ENABLE_OPENCL: ${ENABLE_OPENCL}")
message(STATUS "")

# Попытка найти CUDA
if(ENABLE_CUDA)
    find_package(CUDA QUIET)
    if(CUDA_FOUND)
        message(STATUS "✅ CUDA НАЙДЕНА!")
        message(STATUS "   CUDA Version: ${CUDA_VERSION}")
        message(STATUS "   CUDA Toolkit: ${CUDA_TOOLKIT_ROOT_DIR}")
        message(STATUS "   CUDA Include: ${CUDA_INCLUDE_DIRS}")
        
        # Определить GPU архитектуру
        if(CUDA_ARCH STREQUAL "")
            # Попробовать определить автоматически
            message(STATUS "🔍 Автоматическое определение GPU архитектуры...")
            execute_process(
                COMMAND ${CUDA_TOOLKIT_ROOT_DIR}/extras/demo_suite/deviceQuery
                OUTPUT_VARIABLE CUDA_DEVICE_QUERY
                ERROR_QUIET
            )
            
            # Попробовать альтернативный способ
            if(NOT CUDA_DEVICE_QUERY)
                message(STATUS "⚠️  deviceQuery не найден, используем стандартные архитектуры")
                set(CUDA_ARCH "70;75;80")  # V100, RTX 2080, A100
            else()
                message(STATUS "📊 GPU Info:")
                message(STATUS "${CUDA_DEVICE_QUERY}")
            endif()
        endif()
        
        message(STATUS "   Target CUDA Arch: ${CUDA_ARCH}")
        set(CUDA_ENABLED TRUE)
    else()
        message(STATUS "⚠️  CUDA НЕ НАЙДЕНА")
        message(STATUS "   Установите CUDA Toolkit: https://developer.nvidia.com/cuda-toolkit")
        message(STATUS "   Или отключите CUDA: -DENABLE_CUDA=OFF")
        set(CUDA_ENABLED FALSE)
    endif()
else()
    message(STATUS "⏭️  CUDA отключена (ENABLE_CUDA=OFF)")
    set(CUDA_ENABLED FALSE)
endif()

# Попытка найти OpenCL
if(ENABLE_OPENCL)
    find_package(OpenCL QUIET)
    if(OpenCL_FOUND)
        message(STATUS "✅ OpenCL НАЙДЕНА!")
        message(STATUS "   OpenCL Version: ${OpenCL_VERSION_STRING}")
        message(STATUS "   OpenCL Include: ${OpenCL_INCLUDE_DIRS}")
        set(OPENCL_ENABLED TRUE)
    else()
        message(STATUS "⚠️  OpenCL НЕ НАЙДЕНА")
        set(OPENCL_ENABLED FALSE)
    endif()
else()
    message(STATUS "⏭️  OpenCL отключена (ENABLE_OPENCL=OFF)")
    set(OPENCL_ENABLED FALSE)
endif()

# ============================================================================
# ЧАСТЬ 1: ОПРЕДЕЛЕНИЕ ОПЕРАЦИОННОЙ СИСТЕМЫ
# ============================================================================

if(WIN32)
    set(IS_WINDOWS TRUE)
    set(IS_LINUX FALSE)
    set(PLATFORM_NAME "Windows")
    message(STATUS "🪟 Операционная система: WINDOWS")
    
elseif(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
    set(IS_WINDOWS FALSE)
    set(PLATFORM_NAME "Linux")
    message(STATUS "🐧 Операционная система: LINUX")
    
elseif(APPLE)
    set(IS_LINUX TRUE)
    set(IS_WINDOWS FALSE)
    set(PLATFORM_NAME "macOS")
    message(STATUS "🍎 Операционная система: macOS")
    
else()
    message(FATAL_ERROR "❌ Неизвестная операционная система!")
endif()

# ============================================================================
# ЧАСТЬ 2: ВЕРСИЯ И СТАНДАРТЫ C++
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ Standard: C++17")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# ============================================================================
# ЧАСТЬ 3: ПАРАМЕТРЫ СБОРКИ
# ============================================================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# ============================================================================
# ЧАСТЬ 4: КОМПИЛЯТОР И ФЛАГИ ОПТИМИЗАЦИИ
# ============================================================================

if(MSVC)
    # ===== WINDOWS (MSVC компилятор) =====
    message(STATUS "🔧 Конфигурация: MSVC (Visual Studio)")
    
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Oi /arch:AVX2 /DNDEBUG")
    
    add_compile_options(
        /W4
        /EHsc
        /permissive-
        /Zc:inline
    )
    
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
    )
    
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
    message(STATUS "Optimization: /O2 /arch:AVX2")
    
else()
    # ===== LINUX / WSL / macOS (GCC / Clang) =====
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "🔧 Конфигурация: GCC")
        set(COMPILER_NAME "GCC")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "🔧 Конфигурация: Clang")
        set(COMPILER_NAME "Clang")
    else()
        message(STATUS "🔧 Конфигурация: ${CMAKE_CXX_COMPILER_ID}")
        set(COMPILER_NAME "${CMAKE_CXX_COMPILER_ID}")
    endif()
    
    set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb3 -O0 -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -Wall")
    
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -ffast-math
        -funroll-loops
    )
    
    message(STATUS "Compiler: ${COMPILER_NAME}")
    message(STATUS "Optimization: -O3 -march=native")
    
endif()

# ============================================================================
# ЧАСТЬ 5: СТРУКТУРА ПРОЕКТА
# ============================================================================

message(STATUS "")
message(STATUS "📁 Структура проекта:")
message(STATUS "   Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "   Build dir: ${CMAKE_BINARY_DIR}")
message(STATUS "")

set(SOURCES
    src/main.cpp
    src/mylib.cpp
)

set(HEADERS
    include/mylib.h
)

# Добавить CUDA файлы если включена
if(CUDA_ENABLED)
    list(APPEND SOURCES
        src/gpu_kernel.cu
    )
    list(APPEND HEADERS
        include/gpu_kernel.h
    )
    message(STATUS "✅ CUDA источники добавлены")
endif()

# Добавить OpenCL файлы если включена
if(OPENCL_ENABLED)
    list(APPEND SOURCES
        src/opencl_kernel.cpp
    )
    list(APPEND HEADERS
        include/opencl_kernel.h
    )
    message(STATUS "✅ OpenCL источники добавлены")
endif()

# ============================================================================
# ЧАСТЬ 6: СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА
# ============================================================================

if(CUDA_ENABLED)
    # Использовать CUDA для компиляции
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
    
    # Установить CUDA архитектуру
    if(NOT CUDA_ARCH STREQUAL "")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            CUDA_ARCHITECTURES ${CUDA_ARCH}
        )
    endif()
    
    message(STATUS "✅ Используется CUDA компилятор для GPU кода")
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

# Подключить include директорию
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# ЧАСТЬ 7: ЛИНКОВКА И ЗАВИСИМОСТИ
# ============================================================================

if(NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

# Линковка CUDA
if(CUDA_ENABLED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CUDA_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${CUDA_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE CUDA_ENABLED=1)
    message(STATUS "✅ CUDA библиотеки добавлены в линковку")
endif()

# Линковка OpenCL
if(OPENCL_ENABLED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL::OpenCL)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCL_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE OPENCL_ENABLED=1)
    message(STATUS "✅ OpenCL библиотеки добавлены в линковку")
endif()

# ============================================================================
# ЧАСТЬ 8: ОПЦИИ КОМПИЛЯЦИИ ДЛЯ ЦЕЛИ
# ============================================================================

if(MSVC)
    target_compile_options(${PROJECT_NAME} 
        PRIVATE 
        /arch:AVX2
        /Gy
        /fp:precise
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
endif()

# ============================================================================
# ЧАСТЬ 9: ВЫВОД ИНФОРМАЦИИ О СБОРКЕ
# ============================================================================

message(STATUS "")
message(STATUS "📊 Информация о сборке:")
message(STATUS "   Platform: ${PLATFORM_NAME}")
message(STATUS "   Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "   C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "   Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "   CUDA: ${CUDA_ENABLED}")
message(STATUS "   OpenCL: ${OPENCL_ENABLED}")
message(STATUS "")

# ============================================================================
# ЧАСТЬ 10: ИНСТРУКЦИИ ДЛЯ ПОЛЬЗОВАТЕЛЯ
# ============================================================================

if(IS_WINDOWS)
    message(STATUS "✨ Команды для сборки на Windows:")
    message(STATUS "")
    message(STATUS "   С CUDA:")
    message(STATUS "   cmake -B build -G \"Visual Studio 17 2022\" -DENABLE_CUDA=ON")
    message(STATUS "   cmake --build build --config Release")
    message(STATUS "")
    message(STATUS "   Без CUDA:")
    message(STATUS "   cmake -B build -G \"Visual Studio 17 2022\" -DENABLE_CUDA=OFF")
    message(STATUS "   cmake --build build --config Release")
    message(STATUS "")
else()
    message(STATUS "✨ Команды для сборки на Linux/macOS:")
    message(STATUS "")
    message(STATUS "   С CUDA и Ninja:")
    message(STATUS "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=ON")
    message(STATUS "   ninja -C build")
    message(STATUS "")
    message(STATUS "   С OpenCL:")
    message(STATUS "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENCL=ON")
    message(STATUS "   ninja -C build")
    message(STATUS "")
    message(STATUS "   Без GPU:")
    message(STATUS "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=OFF")
    message(STATUS "   ninja -C build")
    message(STATUS "")
endif()

# ============================================================================
# КОНЕЦ КОНФИГУРАЦИИ
# ============================================================================

message(STATUS "✅ CMake конфигурация успешна!")
