#include "signal_buffer.h"
#include <stdexcept>
#include <fstream>
#include <cstring>

SignalBuffer::SignalBuffer()
    : num_beams_(0), num_samples_(0) {
}

SignalBuffer::SignalBuffer(size_t num_beams, size_t num_samples)
    : num_beams_(num_beams), num_samples_(num_samples) {
    if (num_beams == 0 || num_samples == 0) {
        throw std::invalid_argument("num_beams and num_samples must be > 0");
    }
    data_.resize(num_beams * num_samples, ComplexType(0.0f, 0.0f));
}

bool SignalBuffer::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Читаем метаданные
    file.read(reinterpret_cast<char*>(&num_beams_), sizeof(num_beams_));
    file.read(reinterpret_cast<char*>(&num_samples_), sizeof(num_samples_));

    // Читаем данные
    size_t total_size = num_beams_ * num_samples_;
    data_.resize(total_size);
    file.read(reinterpret_cast<char*>(data_.data()),
              total_size * sizeof(ComplexType));

    file.close();
    return file.gcount() == static_cast<std::streamsize>(total_size * sizeof(ComplexType));
}

bool SignalBuffer::SaveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Пишем метаданные
    file.write(reinterpret_cast<const char*>(&num_beams_), sizeof(num_beams_));
    file.write(reinterpret_cast<const char*>(&num_samples_), sizeof(num_samples_));

    // Пишем данные
    size_t total_size = num_beams_ * num_samples_;
    file.write(reinterpret_cast<const char*>(data_.data()),
               total_size * sizeof(ComplexType));

    file.close();
    return file.good();
}

SignalBuffer::ComplexType* SignalBuffer::GetBeamData(size_t beam_id) {
    if (!ValidateBeamIndex(beam_id)) {
        return nullptr;
    }
    return data_.data() + GetLinearIndex(beam_id, 0);
}

const SignalBuffer::ComplexType* SignalBuffer::GetBeamData(size_t beam_id) const {
    if (!ValidateBeamIndex(beam_id)) {
        return nullptr;
    }
    return data_.data() + GetLinearIndex(beam_id, 0);
}

SignalBuffer::ComplexType SignalBuffer::GetElement(size_t beam_id, size_t sample_id) const {
    if (!ValidateBeamIndex(beam_id) || !ValidateSampleIndex(sample_id)) {
        return ComplexType(0.0f, 0.0f);
    }
    return data_[GetLinearIndex(beam_id, sample_id)];
}

void SignalBuffer::SetElement(size_t beam_id, size_t sample_id, const ComplexType& value) {
    if (!ValidateBeamIndex(beam_id) || !ValidateSampleIndex(sample_id)) {
        return;
    }
    data_[GetLinearIndex(beam_id, sample_id)] = value;
}

size_t SignalBuffer::GetNumBeams() const {
    return num_beams_;
}

size_t SignalBuffer::GetNumSamples() const {
    return num_samples_;
}

void SignalBuffer::Resize(size_t num_beams, size_t num_samples) {
    if (num_beams == 0 || num_samples == 0) {
        throw std::invalid_argument("num_beams and num_samples must be > 0");
    }
    num_beams_ = num_beams;
    num_samples_ = num_samples;
    data_.clear();
    data_.resize(num_beams * num_samples, ComplexType(0.0f, 0.0f));
}

void SignalBuffer::Clear() {
    data_.clear();
    num_beams_ = 0;
    num_samples_ = 0;
}

bool SignalBuffer::IsValid() const {
    return !data_.empty() && num_beams_ > 0 && num_samples_ > 0;
}

bool SignalBuffer::ValidateBeamIndex(size_t beam_id) const {
    return beam_id < num_beams_;
}

bool SignalBuffer::ValidateSampleIndex(size_t sample_id) const {
    return sample_id < num_samples_;
}
