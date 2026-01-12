#ifndef SIGNAL_BUFFER_H
#define SIGNAL_BUFFER_H

#include <complex>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdint>   

/**
* @brief Класс для управления сигнальными данными (лучами)
*
* Хранит 1-256 лучей, каждый луч содержит 100-1300000 комплексных точек.
* Использует vector<complex<float>> для совместимости с OpenCL.
*
* РАСШИРЕН методами из SignalBufferNew:
* - GetTotalSize()
* - RawData()
* - IsAllocated()
* - MemorySizeBytes()
* - Clear()
*/
class SignalBuffer {
public:
    using ComplexType = std::complex<float>;
    using BeamType = std::vector<ComplexType>;

    /**
    * @brief Конструктор по умолчанию
    */
    SignalBuffer();

    /**
    * @brief Конструктор с заданными размерами
    * @param num_beams Количество лучей (1-256)
    * @param num_samples Количество отсчётов на луч (100-1300000)
    */
    SignalBuffer(size_t num_beams, size_t num_samples);

    /**
    * @brief Деструктор
    */
    ~SignalBuffer() = default;

    /**
    * @brief Загрузить данные из бинарного файла
    * @param filename Путь к файлу
    * @return true если успешно, false при ошибке
    */
    bool LoadFromFile(const std::string& filename);

    /**
    * @brief Сохранить данные в бинарный файл
    * @param filename Путь к файлу
    * @return true если успешно, false при ошибке
    */
    bool SaveToFile(const std::string& filename) const;

    /**
    * @brief Получить указатель на данные луча
    * @param beam_id Индекс луча (0..num_beams-1)
    * @return Указатель на данные или nullptr при ошибке
    */
    ComplexType* GetBeamData(size_t beam_id);

    /**
    * @brief Получить константный указатель на данные луча
    * @param beam_id Индекс луча (0..num_beams-1)
    * @return Константный указатель на данные или nullptr при ошибке
    */
    const ComplexType* GetBeamData(size_t beam_id) const;

    /**
    * @brief Получить количество лучей
    * @return Количество лучей
    */
    size_t GetNumBeams() const { return num_beams_; }

    /**
    * @brief Получить количество отсчётов на луч
    * @return Количество отсчётов
    */
    size_t GetNumSamples() const { return num_samples_; }

    /**
    * @brief Получить общее количество элементов (лучи × отсчёты)
    * @return Общее количество элементов
    */
    size_t GetTotalSize() const noexcept { return num_beams_ * num_samples_; }

    /**
    * @brief Получить указатель на сырые данные
    * @return Указатель на данные или nullptr
    */
    ComplexType* RawData() noexcept {
        return beams_.empty() ? nullptr : beams_[0].data();
    }

    /**
    * @brief Получить константный указатель на сырые данные
    * @return Константный указатель на данные или nullptr
    */
    const ComplexType* RawData() const noexcept {
        return beams_.empty() ? nullptr : beams_[0].data();
    }

    /**
    * @brief Проверить, выделена ли память
    * @return true если память выделена и валидна
    */
    bool IsAllocated() const noexcept {
        return !beams_.empty() && num_beams_ > 0 && num_samples_ > 0;
    }

    /**
    * @brief Получить размер памяти в байтах
    * @return Размер в байтах
    */
    size_t MemorySizeBytes() const noexcept {
        return GetTotalSize() * sizeof(ComplexType);
    }

    /**
    * @brief Очистить все данные (установить нули)
    */
    void Clear();
    /**
    * @brief Изменить размер буфера
    * @param num_beams Новое количество лучей
    * @param num_samples Новое количество отсчётов
    */
    void Resize(size_t num_beams, size_t num_samples);

    /**
    * @brief Проверить валидность данных
    * @return true если данные валидны
    */
    bool IsValid() const;

    std::vector<BeamType> beams_; // [beam_id][sample_id]

private:
    size_t num_beams_;
    size_t num_samples_;

    /**
    * @brief Валидация индекса луча
    * @param beam_id Индекс луча
    * @return true если индекс валиден
    */
    bool ValidateBeamIndex(size_t beam_id) const;
};

#endif // SIGNAL_BUFFER_H
