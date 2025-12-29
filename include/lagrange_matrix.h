#ifndef LAGRANGE_MATRIX_H
#define LAGRANGE_MATRIX_H

#include <vector>
#include <string>

/**
 * @brief Класс для работы с матрицей коэффициентов Лагранжа
 * 
 * Матрица 48×5 для интерполяции дробной задержки 5-го порядка.
 * Используется для точной дробной задержки сигнала.
 */
class LagrangeMatrix {
public:
    static constexpr size_t ROWS = 48;  // Количество дробных задержек
    static constexpr size_t COLS = 5;   // Количество коэффициентов (порядок полинома)
    
    /**
     * @brief Конструктор
     */
    LagrangeMatrix();
    
    /**
     * @brief Загрузить матрицу из JSON файла
     * @param filename Путь к файлу lagrange_matrix.json
     * @return true если успешно
     */
    bool LoadFromJson(const std::string& filename);
    
    /**
     * @brief Получить указатель на данные матрицы
     * @return Указатель на массив [ROWS][COLS] или nullptr
     */
    const float* GetData() const { return matrix_.data(); }
    
    /**
     * @brief Получить коэффициент матрицы
     * @param row Индекс строки [0, ROWS-1]
     * @param col Индекс столбца [0, COLS-1]
     * @return Коэффициент или 0.0f при ошибке
     */
    float GetCoefficient(size_t row, size_t col) const;
    
    /**
     * @brief Получить строку матрицы для заданной дробной задержки
     * @param delay_fraction Дробная часть задержки [0.0, 1.0)
     * @return Индекс строки [0, ROWS-1]
     */
    size_t GetRowIndex(float delay_fraction) const;
    
    /**
     * @brief Проверить валидность матрицы
     * @return true если матрица загружена и валидна
     */
    bool IsValid() const { return matrix_.size() == ROWS * COLS; }
    
    /**
     * @brief Получить размер матрицы в байтах
     * @return Размер в байтах
     */
    size_t GetSizeBytes() const { return ROWS * COLS * sizeof(float); }

private:
    std::vector<float> matrix_;  // Плоский массив [ROWS * COLS]
    
    /**
     * @brief Парсинг JSON массива (простая реализация)
     * @param json_content Содержимое JSON файла
     * @return true если успешно распарсено
     */
    bool ParseJson(const std::string& json_content);
};

#endif // LAGRANGE_MATRIX_H

