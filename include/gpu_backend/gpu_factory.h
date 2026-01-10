#ifndef GPU_FACTORY_H
#define GPU_FACTORY_H

#include "igpu_backend.h"
#include <memory>

/**
 * @brief Фабрика для создания GPU backend
 * 
 * Автоматически определяет доступные GPU и создаёт оптимальный backend.
 */
class GPUFactory {
public:
    /**
     * @brief Создать GPU backend
     * 
     * Приоритет:
     * 1. OpenCL (NVIDIA RTX3060)
     * 2. OpenCL (AMD GPU)
     * 3. Другие OpenCL устройства
     * 
     * @return Умный указатель на backend или nullptr при ошибке
     */
    static std::unique_ptr<IGPUBackend> CreateBackend();
    
    /**
     * @brief Создать OpenCL backend
     * @return Умный указатель на OpenCL backend или nullptr при ошибке
     */
    static std::unique_ptr<IGPUBackend> CreateOpenCLBackend();
    
    /**
     * @brief Проверить доступность OpenCL
     * @return true если OpenCL доступен
     */
    static bool IsOpenCLAvailable();

private:
    GPUFactory() = delete;  // Статический класс
};

#endif // GPU_FACTORY_H

