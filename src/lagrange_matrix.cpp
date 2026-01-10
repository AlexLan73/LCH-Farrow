#include "lagrange_matrix.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

LagrangeMatrix::LagrangeMatrix() {
    matrix_.resize(ROWS * COLS, 0.0f);
}

bool LagrangeMatrix::LoadFromJson(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return ParseJson(buffer.str());
}

bool LagrangeMatrix::ParseJson(const std::string& json_content) {
    // Простой парсер JSON для массива массивов
    // Формат: [[val, val, ...], [val, val, ...], ...]
    
    matrix_.clear();
    matrix_.reserve(ROWS * COLS);
    
    size_t pos = 0;
    size_t row = 0;
    
    // Пропускаем начальный '['
    pos = json_content.find('[', pos);
    if (pos == std::string::npos) {
        std::cerr << "Ошибка: неверный формат JSON" << std::endl;
        return false;
    }
    pos++;
    
    while (row < ROWS && pos < json_content.length()) {
        // Ищем начало строки '['
        pos = json_content.find('[', pos);
        if (pos == std::string::npos) {
            break;
        }
        pos++;
        
        // Читаем 5 значений
        for (size_t col = 0; col < COLS; ++col) {
            // Пропускаем пробелы и запятые
            while (pos < json_content.length() && 
                   (json_content[pos] == ' ' || json_content[pos] == '\n' || 
                    json_content[pos] == '\t' || json_content[pos] == ',')) {
                pos++;
            }
            
            if (pos >= json_content.length()) {
                std::cerr << "Ошибка: неожиданный конец файла на строке " << row << std::endl;
                return false;
            }
            
            // Читаем число
            size_t num_end = pos;
            while (num_end < json_content.length() && 
                   (json_content[num_end] == '.' || 
                    (json_content[num_end] >= '0' && json_content[num_end] <= '9') ||
                    json_content[num_end] == '-' || json_content[num_end] == '+' ||
                    json_content[num_end] == 'e' || json_content[num_end] == 'E')) {
                num_end++;
            }
            
            if (num_end == pos) {
                std::cerr << "Ошибка: не удалось прочитать число на строке " << row 
                          << ", столбец " << col << std::endl;
                return false;
            }
            
            std::string num_str = json_content.substr(pos, num_end - pos);
            float value = std::stof(num_str);
            matrix_.push_back(value);
            
            pos = num_end;
        }
        
        // Ищем конец строки ']'
        pos = json_content.find(']', pos);
        if (pos == std::string::npos) {
            std::cerr << "Ошибка: не найден конец строки " << row << std::endl;
            return false;
        }
        pos++;
        
        row++;
    }
    
    if (matrix_.size() != ROWS * COLS) {
        std::cerr << "Ошибка: неверный размер матрицы. Ожидалось " << (ROWS * COLS)
                  << ", получено " << matrix_.size() << std::endl;
        return false;
    }
    
    return true;
}

float LagrangeMatrix::GetCoefficient(size_t row, size_t col) const {
    if (row >= ROWS || col >= COLS) {
        std::cerr << "Ошибка: индекс вне границ [" << row << "][" << col << "]" << std::endl;
        return 0.0f;
    }
    return matrix_[row * COLS + col];
}

size_t LagrangeMatrix::GetRowIndex(float delay_fraction) const {
    // delay_fraction должен быть в [0.0, 1.0)
    delay_fraction = std::fmod(delay_fraction, 1.0f);
    if (delay_fraction < 0.0f) {
        delay_fraction += 1.0f;
    }
    
    size_t row = static_cast<size_t>(delay_fraction * ROWS);
    return (row >= ROWS) ? ROWS - 1 : row;
}

