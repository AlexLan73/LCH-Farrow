#pragma once

#include <CL/cl.h>
#include <complex>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace radar {
namespace gpu {

// ═════════════════════════════════════════════════════════════════════════════
// GPU COMPLEX VECTOR - RAII WRAPPER ДЛЯ КОМПЛЕКСНЫХ ДАННЫХ НА GPU
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief GPU вектор для комплексных чисел (std::complex<T>)
 * 
 * НАЗНАЧЕНИЕ:
 * - Обеспечивает удобный доступ к GPU памяти для комплексных данных
 * - Автоматическое управление памятью (RAII)
 * - Двусторонний трансфер CPU↔GPU
 * - Готов для FFT и других GPU алгоритмов
 * 
 * АРХИТЕКТУРА:
 * GPU MEMORY: [real0, imag0, real1, imag1, real2, imag2, ...]
 *             └── float2 (8 байт) ──┘
 * 
 * CPU ACCESS: std::complex<float> = {real, imag}
 *             ComplexVector[i] ← автоматическое преобразование
 * 
 * ИСПОЛЬЗОВАНИЕ:
 * ComplexVector<float> vec(context, queue, num_elements);
 * vec.SetData(cpu_data, size);        // CPU → GPU
 * std::vector<std::complex<float>> result = vec.GetData(); // GPU → CPU
 * cl_mem gpu_mem = vec.GetMemObject(); // Для kernel
 * 
 * @tparam T Тип компонента (float, double)
 */
template<typename T>
class ComplexVector {
public:
    static_assert(std::is_floating_point<T>::value, 
                  "ComplexVector requires floating-point type (float, double)");
    
    using value_type = std::complex<T>;
    using component_type = T;

private:
    // OpenCL ресурсы
    cl_context context_;
    cl_command_queue queue_;
    cl_mem gpu_buffer_;
    
    // Метаданные
    size_t num_elements_;      // Количество комплексных элементов
    size_t buffer_size_bytes_; // Размер буфера в байтах
    
    // Флаги
    bool is_allocated_;

public:
    // ═════════════════════════════════════════════════════════════════
    // CONSTRUCTOR / DESTRUCTOR
    // ═════════════════════════════════════════════════════════════════

    /**
     * @brief Конструктор
     * @param context OpenCL контекст
     * @param queue OpenCL очередь команд
     * @param num_elements Количество комплексных элементов
     * @throws std::runtime_error если выделение памяти GPU не удалось
     */
    explicit ComplexVector(
        cl_context context,
        cl_command_queue queue,
        size_t num_elements
    );

    /**
     * @brief Деструктор - освобождение GPU памяти
     */
    ~ComplexVector();

    // DELETE COPY, ALLOW MOVE
    ComplexVector(const ComplexVector&) = delete;
    ComplexVector& operator=(const ComplexVector&) = delete;
    
    /**
     * @brief Move конструктор
     */
    ComplexVector(ComplexVector&&) noexcept;
    
    /**
     * @brief Move оператор присваивания
     */
    ComplexVector& operator=(ComplexVector&&) noexcept;

    // ═════════════════════════════════════════════════════════════════
    // DATA TRANSFER (CPU ↔ GPU)
    // ═════════════════════════════════════════════════════════════════

    /**
     * @brief Загрузить данные с CPU на GPU
     * 
     * @param cpu_data Указатель на массив std::complex<T>
     * @param count Количество элементов для копирования
     * @throws std::invalid_argument если count > num_elements_
     * @throws std::runtime_error если трансфер не удался
     */
    void SetData(const value_type* cpu_data, size_t count);

    /**
     * @brief Загрузить данные с CPU на GPU (вектор)
     * 
     * @param cpu_data Вектор std::complex<T>
     * @throws std::invalid_argument если размер вектора > num_elements_
     */
    void SetData(const std::vector<value_type>& cpu_data);

    /**
     * @brief Скачать данные с GPU на CPU
     * 
     * @param offset Смещение в элементах (по умолчанию 0)
     * @param count Количество элементов (по умолчанию все)
     * @return std::vector<std::complex<T>> с данными с GPU
     * @throws std::runtime_error если трансфер не удался
     */
    std::vector<value_type> GetData(size_t offset = 0, size_t count = 0) const;

    /**
     * @brief Скачать N первых элементов с GPU (для отладки)
     * 
     * @param count Количество элементов
     * @return std::vector<std::complex<T>>
     */
    std::vector<value_type> GetDataFirst(size_t count) const;

    /**
     * @brief Скачать N последних элементов с GPU (для отладки)
     * 
     * @param count Количество элементов
     * @return std::vector<std::complex<T>>
     */
    std::vector<value_type> GetDataLast(size_t count) const;

    // ═════════════════════════════════════════════════════════════════
    // GPU MEMORY ACCESS
    // ═════════════════════════════════════════════════════════════════

    /**
     * @brief Получить cl_mem объект для использования в kernel
     * 
     * @return cl_mem адрес GPU буфера
     */
    cl_mem GetMemObject() const noexcept { return gpu_buffer_; }

    /**
     * @brief Получить contexT OpenCL
     */
    cl_context GetContext() const noexcept { return context_; }

    /**
     * @brief Получить очередь команд OpenCL
     */
    cl_command_queue GetQueue() const noexcept { return queue_; }

    // ═════════════════════════════════════════════════════════════════
    // METADATA
    // ═════════════════════════════════════════════════════════════════

    /**
     * @brief Получить количество комплексных элементов
     */
    size_t Size() const noexcept { return num_elements_; }

    /**
     * @brief Получить размер буфера в байтах
     */
    size_t SizeBytes() const noexcept { return buffer_size_bytes_; }

    /**
     * @brief Получить размер одного элемента (sizeof(std::complex<T>))
     */
    static constexpr size_t ElementSize() noexcept {
        return sizeof(value_type);
    }

    /**
     * @brief Проверить, выделена ли память
     */
    bool IsAllocated() const noexcept { return is_allocated_; }

    // ═════════════════════════════════════════════════════════════════
    // UTILITY
    // ═════════════════════════════════════════════════════════════════

    /**
     * @brief Очистить GPU очередь команд
     */
    void Flush() const noexcept { clFlush(queue_); }

    /**
     * @brief Дождаться завершения всех команд в очереди
     */
    void Finish() const noexcept { clFinish(queue_); }

    /**
     * @brief Получить информацию о буфере (для отладки)
     */
    std::string GetInfo() const;
};

// ═════════════════════════════════════════════════════════════════════════════
// CONVENIENCE ALIASES
// ═════════════════════════════════════════════════════════════════════════════

using ComplexVectorF = ComplexVector<float>;    ///< float-based complex vector
using ComplexVectorD = ComplexVector<double>;   ///< double-based complex vector

} // namespace gpu
} // namespace radar
