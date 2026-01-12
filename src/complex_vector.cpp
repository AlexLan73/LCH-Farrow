#include "complex_vector.h"

#include <cstring>
#include <sstream>
#include <algorithm>

namespace radar {
namespace gpu {

// ═════════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR / DESTRUCTOR
// ═════════════════════════════════════════════════════════════════════════════

template<typename T>
ComplexVector<T>::ComplexVector(
    cl_context context,
    cl_command_queue queue,
    size_t num_elements
)
    : context_(context),
      queue_(queue),
      gpu_buffer_(nullptr),
      num_elements_(num_elements),
      buffer_size_bytes_(num_elements * sizeof(value_type)),
      is_allocated_(false)
{
    if (num_elements == 0) {
        throw std::invalid_argument("ComplexVector: num_elements must be > 0");
    }

    if (context == nullptr || queue == nullptr) {
        throw std::invalid_argument("ComplexVector: context and queue cannot be nullptr");
    }

    // Выделить GPU память
    cl_int err = CL_SUCCESS;
    gpu_buffer_ = clCreateBuffer(
        context_,
        CL_MEM_READ_WRITE,
        buffer_size_bytes_,
        nullptr,
        &err
    );

    if (err != CL_SUCCESS) {
        throw std::runtime_error(
            std::string("Failed to allocate GPU buffer: error code ") +
            std::to_string(err)
        );
    }

    is_allocated_ = true;

    std::cout << "✓ ComplexVector<"
              << (std::is_same<T, float>::value ? "float" : "double")
              << "> allocated: "
              << buffer_size_bytes_ / (1024 * 1024) << " MB ("
              << num_elements_ << " elements)"
              << std::endl;
}

template<typename T>
ComplexVector<T>::~ComplexVector() {
    if (is_allocated_ && gpu_buffer_ != nullptr) {
        clReleaseMemObject(gpu_buffer_);
        is_allocated_ = false;
    }
}

// Move semantics
template<typename T>
ComplexVector<T>::ComplexVector(ComplexVector&&) noexcept = default;

template<typename T>
ComplexVector<T>& ComplexVector<T>::operator=(ComplexVector&&) noexcept = default;

// ═════════════════════════════════════════════════════════════════════════════
// DATA TRANSFER
// ═════════════════════════════════════════════════════════════════════════════

template<typename T>
void ComplexVector<T>::SetData(const value_type* cpu_data, size_t count) {
    if (cpu_data == nullptr) {
        throw std::invalid_argument("SetData: cpu_data is nullptr");
    }

    if (count > num_elements_) {
        throw std::invalid_argument(
            std::string("SetData: count (") + std::to_string(count) +
            ") exceeds vector size (" + std::to_string(num_elements_) + ")"
        );
    }

    size_t transfer_bytes = count * sizeof(value_type);

    cl_int err = clEnqueueWriteBuffer(
        queue_,
        gpu_buffer_,
        CL_TRUE,  // blocking write
        0,        // offset
        transfer_bytes,
        cpu_data,
        0, nullptr, nullptr
    );

    if (err != CL_SUCCESS) {
        throw std::runtime_error(
            std::string("Failed to transfer data to GPU: error code ") +
            std::to_string(err)
        );
    }

    std::cout << "✓ SetData: transferred " << transfer_bytes / (1024 * 1024)
              << " MB (" << count << " elements) to GPU"
              << std::endl;
}

template<typename T>
void ComplexVector<T>::SetData(const std::vector<value_type>& cpu_data) {
    SetData(cpu_data.data(), cpu_data.size());
}

template<typename T>
std::vector<typename ComplexVector<T>::value_type>
ComplexVector<T>::GetData(size_t offset, size_t count) const {
    if (offset >= num_elements_) {
        throw std::invalid_argument(
            std::string("GetData: offset (") + std::to_string(offset) +
            ") exceeds vector size (" + std::to_string(num_elements_) + ")"
        );
    }

    // Если count == 0, получить все оставшиеся элементы
    if (count == 0) {
        count = num_elements_ - offset;
    }

    if (offset + count > num_elements_) {
        throw std::invalid_argument(
            std::string("GetData: offset + count exceeds vector size")
        );
    }

    std::vector<value_type> result(count);
    size_t transfer_bytes = count * sizeof(value_type);

    cl_int err = clEnqueueReadBuffer(
        queue_,
        gpu_buffer_,
        CL_TRUE,  // blocking read
        offset * sizeof(value_type),
        transfer_bytes,
        result.data(),
        0, nullptr, nullptr
    );

    if (err != CL_SUCCESS) {
        throw std::runtime_error(
            std::string("Failed to transfer data from GPU: error code ") +
            std::to_string(err)
        );
    }

    return result;
}

template<typename T>
std::vector<typename ComplexVector<T>::value_type>
ComplexVector<T>::GetDataFirst(size_t count) const {
    return GetData(0, std::min(count, num_elements_));
}

template<typename T>
std::vector<typename ComplexVector<T>::value_type>
ComplexVector<T>::GetDataLast(size_t count) const {
    size_t actual_count = std::min(count, num_elements_);
    size_t offset = num_elements_ - actual_count;
    return GetData(offset, actual_count);
}

// ═════════════════════════════════════════════════════════════════════════════
// UTILITY
// ═════════════════════════════════════════════════════════════════════════════

template<typename T>
std::string ComplexVector<T>::GetInfo() const {
    std::ostringstream oss;
    oss << "ComplexVector<"
        << (std::is_same<T, float>::value ? "float" : "double")
        << "> Info:\n"
        << "  Elements: " << num_elements_ << "\n"
        << "  Element Size: " << sizeof(value_type) << " bytes\n"
        << "  Total Memory: " << buffer_size_bytes_ / (1024 * 1024) << " MB\n"
        << "  GPU Buffer: " << (is_allocated_ ? "allocated" : "not allocated") << "\n"
        << "  GPU Address: 0x" << std::hex << (uintptr_t)gpu_buffer_ << std::dec;
    return oss.str();
}

// ═════════════════════════════════════════════════════════════════════════════
// EXPLICIT INSTANTIATION
// ═════════════════════════════════════════════════════════════════════════════

// Явная инстанциация для float и double
template class ComplexVector<float>;
template class ComplexVector<double>;

} // namespace gpu
} // namespace radar
