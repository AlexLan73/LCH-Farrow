cmake_minimum_required(VERSION 3.20)

# Create example executable
add_executable(fractional_delay_gpu_example
    fractional_delay_gpu_example.cpp
)

# Link with required libraries
target_link_libraries(fractional_delay_gpu_example PRIVATE
    OpenCL::OpenCL
    ${CLFFT_LIBRARY}
)

# Include directories
target_include_directories(fractional_delay_gpu_example PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/examples
    ${OpenCL_INCLUDE_DIRS}
    ${CLFFT_INCLUDE_DIR}
)

# Add compile definitions
target_compile_definitions(fractional_delay_gpu_example PRIVATE
    OPENCL_ENABLED=1
    CLFFT_FOUND=${CLFFT_FOUND}
    OPENCL_KERNEL_DIR="${OPENCL_KERNEL_DIR}"
)

# Copy example to build directory
add_custom_command(TARGET fractional_delay_gpu_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:fractional_delay_gpu_example>"
        "${CMAKE_BINARY_DIR}"
)

message(STATUS "✅ Example 'fractional_delay_gpu_example' configured")