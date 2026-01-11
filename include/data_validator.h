#pragma once

#include "result_comparator.h"
#include "validator.h"
#include "signal_buffer.h"

namespace radar {

/**
 * @brief Класс для валидации данных, сравнивающий результаты CPU и GPU
 *
 * Интегрируется с существующими компонентами ResultComparator и Validator
 * для обеспечения полного цикла валидации данных.
 */
class DataValidator {
public:
    /**
     * @brief Конструктор по умолчанию
     */
    DataValidator() = default;

    /**
     * @brief Деструктор
     */
    ~DataValidator() = default;

    /**
     * @brief Выполнить валидацию данных CPU и GPU
     *
     * @param cpu_results Результаты CPU обработки
     * @param gpu_results Результаты GPU обработки
     * @param tolerance Допустимая погрешность
     * @param metrics Указатель на структуру для метрик сравнения (может быть nullptr)
     * @return true если валидация прошла успешно, false при ошибке
     */
    bool ValidateData(
        const SignalBuffer& cpu_results,
        const SignalBuffer& gpu_results,
        float tolerance,
        ComparisonMetrics* metrics = nullptr
    );

    /**
     * @brief Выполнить валидацию с использованием внешнего валидатора
     *
     * @param validator Внешний валидатор
     * @param cpu_results Результаты CPU обработки
     * @param gpu_results Результаты GPU обработки
     * @param tolerance Допустимая погрешность
     * @param metrics Указатель на структуру для метрик сравнения (может быть nullptr)
     * @return true если валидация прошла успешно, false при ошибке
     */
    bool ValidateWithExternalValidator(
        Validator& validator,
        const SignalBuffer& cpu_results,
        const SignalBuffer& gpu_results,
        float tolerance,
        ComparisonMetrics* metrics = nullptr
    );

    /**
     * @brief Установить флаг подробного вывода
     *
     * @param verbose Включить подробный вывод
     */
    void SetVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * @brief Получить флаг подробного вывода
     *
     * @return true если подробный вывод включен
     */
    bool GetVerbose() const { return verbose_; }

private:
    bool verbose_ = false;  // Флаг подробного вывода

    /**
     * @brief Вывести метрики сравнения
     *
     * @param metrics Метрики для вывода
     */
    void PrintMetrics(const ComparisonMetrics& metrics) const;
};

} // namespace radar