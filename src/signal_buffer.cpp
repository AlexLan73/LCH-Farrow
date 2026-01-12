#include "signal_buffer.h"
#include <cstdint>
#include <iostream>
#include <cstring>
#include <algorithm>


SignalBuffer::SignalBuffer()
    : num_beams_(0), num_samples_(0) {
}

SignalBuffer::SignalBuffer(size_t num_beams, size_t num_samples)
    : num_beams_(num_beams), num_samples_(num_samples) {
    Resize(num_beams, num_samples);
}

bool SignalBuffer::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return false;
    }

    // Читаем заголовок: num_beams, num_samples
    uint32_t num_beams_u32, num_samples_u32;
    file.read(reinterpret_cast<char*>(&num_beams_u32), sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(&num_samples_u32), sizeof(uint32_t));
    if (!file) {
        std::cerr << "Ошибка: не удалось прочитать заголовок файла" << std::endl;
        return false;
    }

    num_beams_ = static_cast<size_t>(num_beams_u32);
    num_samples_ = static_cast<size_t>(num_samples_u32);

    // Валидация размеров
    if (num_beams_ == 0 || num_beams_ > 256) {
        std::cerr << "Ошибка: неверное количество лучей: " << num_beams_ << std::endl;
        return false;
    }

    if (num_samples_ < 100 || num_samples_ > 1300000) {
        std::cerr << "Ошибка: неверное количество отсчётов: " << num_samples_ << std::endl;
        return false;
    }

    // Изменяем размер буфера
    Resize(num_beams_, num_samples_);

    // Читаем данные: для каждого комплексного числа 2 float (real, imag)
    for (size_t beam = 0; beam < num_beams_; ++beam) {
        for (size_t sample = 0; sample < num_samples_; ++sample) {
            float real, imag;
            file.read(reinterpret_cast<char*>(&real), sizeof(float));
            file.read(reinterpret_cast<char*>(&imag), sizeof(float));
            if (!file) {
                std::cerr << "Ошибка: не удалось прочитать данные для луч " << beam
                    << ", отсчёт " << sample << std::endl;
                return false;
            }

            beams_[beam][sample] = ComplexType(real, imag);
        }
    }

    file.close();
    return true;
}

bool SignalBuffer::SaveToFile(const std::string& filename) const {
    if (!IsValid()) {
        std::cerr << "Ошибка: буфер не валиден для сохранения" << std::endl;
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось создать файл " << filename << std::endl;
        return false;
    }

    // Записываем заголовок
    uint32_t num_beams_u32 = static_cast<uint32_t>(num_beams_);
    uint32_t num_samples_u32 = static_cast<uint32_t>(num_samples_);
    file.write(reinterpret_cast<const char*>(&num_beams_u32), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&num_samples_u32), sizeof(uint32_t));

    // Записываем данные
    for (size_t beam = 0; beam < num_beams_; ++beam) {
        for (size_t sample = 0; sample < num_samples_; ++sample) {
            float real = beams_[beam][sample].real();
            float imag = beams_[beam][sample].imag();
            file.write(reinterpret_cast<const char*>(&real), sizeof(float));
            file.write(reinterpret_cast<const char*>(&imag), sizeof(float));
        }
    }

    file.close();
    return true;
}

SignalBuffer::ComplexType* SignalBuffer::GetBeamData(size_t beam_id) {
    if (!ValidateBeamIndex(beam_id)) {
        return nullptr;
    }

    return beams_[beam_id].data();
}

const SignalBuffer::ComplexType* SignalBuffer::GetBeamData(size_t beam_id) const {
    if (!ValidateBeamIndex(beam_id)) {
        return nullptr;
    }

    return beams_[beam_id].data();
}

void SignalBuffer::Resize(size_t num_beams, size_t num_samples) {
    num_beams_ = num_beams;
    num_samples_ = num_samples;
    beams_.clear();
    beams_.resize(num_beams_);
    for (auto& beam : beams_) {
        beam.resize(num_samples_);
    }
}

void SignalBuffer::Clear() {
    for (auto& beam : beams_) {
        std::fill(beam.begin(), beam.end(), ComplexType(0.0f, 0.0f));
    }
}

bool SignalBuffer::IsValid() const {
    if (num_beams_ == 0 || num_beams_ > 256) {
        return false;
    }

    if (num_samples_ < 100 || num_samples_ > 1300000) {
        return false;
    }

    if (beams_.size() != num_beams_) {
        return false;
    }

    for (const auto& beam : beams_) {
        if (beam.size() != num_samples_) {
            return false;
        }
    }

    return true;
}

bool SignalBuffer::ValidateBeamIndex(size_t beam_id) const {
    if (beam_id >= num_beams_) {
        std::cerr << "Ошибка: неверный индекс луча: " << beam_id
            << " (максимум: " << num_beams_ - 1 << ")" << std::endl;
        return false;
    }

    return true;
}
