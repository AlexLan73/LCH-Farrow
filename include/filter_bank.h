#ifndef FILTER_BANK_H
#define FILTER_BANK_H

#include <vector>
#include <complex>
#include <string>
#include <cstddef>

/**
 * @brief Класс для управления FIR коэффициентами и опорным ЛЧМ сигналом
 * 
 * Хранит FIR коэффициенты фильтра и опорный ЛЧМ (линейно-частотная модуляция) сигнал.
 * Предвычисляет FFT опорного сигнала для оптимизации свёртки.
 */
class FilterBank {
public:
    using ComplexType = std::complex<float>;
    
    /**
     * @brief Конструктор по умолчанию
     */
    FilterBank();
    
    /**
     * @brief Деструктор
     */
    ~FilterBank() = default;
    
    /**
     * @brief Загрузить FIR коэффициенты
     * @param coeffs Вектор коэффициентов
     */
    void LoadCoefficients(const std::vector<float>& coeffs);
    
    /**
     * @brief Загрузить FIR коэффициенты из файла
     * @param filename Путь к файлу
     * @return true если успешно
     */
    bool LoadCoefficientsFromFile(const std::string& filename);
    
    /**
     * @brief Установить опорный сигнал
     * @param signal Вектор комплексных отсчётов
     */
    void SetReferenceSignal(const std::vector<ComplexType>& signal);
    
    /**
     * @brief Сгенерировать опорный ЛЧМ сигнал
     * @param num_samples Количество отсчётов
     * @param bandwidth Полоса частот (Гц)
     * @param duration Длительность сигнала (сек)
     * @param sample_rate Частота дискретизации (Гц, по умолчанию 1.0)
     */
    void GenerateLFMReference(
        size_t num_samples,
        float bandwidth,
        float duration,
        float sample_rate = 1.0f
    );
    
    /**
     * @brief Предвычислить FFT опорного сигнала
     * 
     * Вычисляет FFT опорного сигнала один раз для использования в свёртке.
     * Должно быть вызвано после установки опорного сигнала.
     */
    void PrecomputeReferenceFft();
    
    /**
     * @brief Получить предвычисленную FFT опорного сигнала
     * @return Константный указатель на FFT данные
     */
    const ComplexType* GetReferenceFft() const;
    
    /**
     * @brief Проверить, вычислена ли опорная FFT
     * @return true если FFT вычислена
     */
    bool IsReferenceFftComputed() const { return reference_fft_computed_; }
    
    /**
     * @brief Получить FIR коэффициенты
     * @return Константная ссылка на вектор коэффициентов
     */
    const std::vector<float>& GetCoefficients() const { return fir_coefficients_; }
    
    /**
     * @brief Получить опорный сигнал
     * @return Константная ссылка на вектор сигнала
     */
    const std::vector<ComplexType>& GetReferenceSignal() const { return reference_signal_; }
    
    /**
     * @brief Получить количество коэффициентов
     * @return Количество коэффициентов
     */
    size_t GetNumCoefficients() const { return fir_coefficients_.size(); }
    
    /**
     * @brief Получить размер опорного сигнала
     * @return Размер сигнала
     */
    size_t GetReferenceSize() const { return reference_signal_.size(); }

private:
    std::vector<float> fir_coefficients_;              // FIR коэффициенты
    std::vector<ComplexType> reference_signal_;        // Опорный ЛЧМ сигнал
    std::vector<ComplexType> reference_fft_;           // Предвычисленная FFT опорного
    bool reference_fft_computed_;                       // Флаг вычисления FFT
    
    /**
     * @brief Вычислить FFT на CPU (временная реализация, позже заменим на GPU)
     * @param input Входной сигнал
     * @param output Выходной FFT
     * @param size Размер сигнала
     */
    void ComputeFFT_CPU(
        const std::vector<ComplexType>& input,
        std::vector<ComplexType>& output,
        size_t size
    ) const;
};

#endif // FILTER_BANK_H

