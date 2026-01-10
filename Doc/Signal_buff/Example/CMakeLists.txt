cmake_minimum_required(VERSION 3.10)
project(SignalBuffer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции оптимизации
if(MSVC)
    add_compile_options(/O2 /arch:AVX2)
else()
    add_compile_options(-O3 -march=native -ffast-math)
    if(NOT APPLE)
        add_compile_options(-flto)  # Link Time Optimization
    endif()
endif()

# Основная библиотека
add_library(signal_buffer STATIC
    src/signal_buffer.cpp
    src/lfm_signal_generator.cpp
)

target_include_directories(signal_buffer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/data
)

# Исполняемый файл
add_executable(signal_demo
    src/main.cpp
)

target_link_libraries(signal_demo
    signal_buffer
)

target_include_directories(signal_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/data
)

# Копирование матрицы интерполяции в директорию сборки
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/interpolation_matrix.h
    ${CMAKE_CURRENT_BINARY_DIR}/interpolation_matrix.h
    COPYONLY
)

# Опциональная поддержка OpenCL для GPU
option(ENABLE_OPENCL "Enable OpenCL GPU support" OFF)

if(ENABLE_OPENCL)
    find_package(OpenCL REQUIRED)
    target_link_libraries(signal_demo PRIVATE OpenCL::OpenCL)
    target_compile_definitions(signal_demo PRIVATE USE_OPENCL=1)
endif()

# Интеграция с Google Test (опционально)
option(ENABLE_TESTING "Build tests" OFF)

if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Вывод информации о сборке
message(STATUS "SignalBuffer Project Configuration")
message(STATUS "=================================")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenCL Support: ${ENABLE_OPENCL}")
message(STATUS "Testing: ${ENABLE_TESTING}")
message(STATUS "=================================")
