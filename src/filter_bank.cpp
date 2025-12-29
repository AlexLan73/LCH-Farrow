#include "filter_bank.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

FilterBank::FilterBank()
    : reference_fft_computed_(false) {
}

void FilterBank::LoadCoefficients(const std::vector<float>& coeffs) {
    fir_coefficients_ = coeffs;
}

bool FilterBank::LoadCoefficientsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
        return false;
    }
    
    fir_coefficients_.clear();
    float value;
    while (file >> value) {
        fir_coefficients_.push_back(value);
    }
    
    file.close();
    return !fir_coefficients_.empty();
}

void FilterBank::SetReferenceSignal(const std::vector<ComplexType>& signal) {
    reference_signal_ = signal;
    reference_fft_computed_ = false;  // Нужно пересчитать FFT
    reference_fft_.clear();
}

void FilterBank::GenerateLFMReference(
    size_t num_samples,
    float bandwidth,
    float duration,
    float sample_rate) {
    
    reference_signal_.clear();
    reference_signal_.resize(num_samples);
    
    const float pi = 3.14159265358979323846f;
    const float dt = duration / static_cast<float>(num_samples);
    const float chirp_rate = bandwidth / duration;  // Скорость изменения частоты
    
    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) * dt;
        // ЛЧМ сигнал: s(t) = exp(j * 2π * (f0*t + (chirp_rate/2)*t²))
        // Для упрощения f0 = 0
        float phase = pi * chirp_rate * t * t;
        reference_signal_[i] = ComplexType(std::cos(phase), std::sin(phase));
    }
    
    reference_fft_computed_ = false;  // Нужно пересчитать FFT
    reference_fft_.clear();
}

void FilterBank::PrecomputeReferenceFft() {
    if (reference_signal_.empty()) {
        std::cerr << "Ошибка: опорный сигнал не установлен" << std::endl;
        return;
    }
    
    reference_fft_.resize(reference_signal_.size());
    ComputeFFT_CPU(reference_signal_, reference_fft_, reference_signal_.size());
    reference_fft_computed_ = true;
}

const FilterBank::ComplexType* FilterBank::GetReferenceFft() const {
    if (!reference_fft_computed_) {
        std::cerr << "Предупреждение: опорная FFT не вычислена" << std::endl;
        return nullptr;
    }
    return reference_fft_.data();
}

void FilterBank::ComputeFFT_CPU(
    const std::vector<ComplexType>& input,
    std::vector<ComplexType>& output,
    size_t size) const {
    
    // Временная простая реализация FFT на CPU
    // Позже будет заменена на GPU версию через OpenCL
    // Используем простой алгоритм Cooley-Tukey FFT
    
    if (size == 0) {
        return;
    }
    
    output.resize(size);
    
    // Для упрощения используем прямое вычисление DFT
    // В production версии нужно использовать оптимизированную FFT библиотеку
    const float pi = 3.14159265358979323846f;
    
    for (size_t k = 0; k < size; ++k) {
        ComplexType sum(0.0f, 0.0f);
        for (size_t n = 0; n < size; ++n) {
            float angle = -2.0f * pi * static_cast<float>(k) * static_cast<float>(n) / static_cast<float>(size);
            ComplexType twiddle(std::cos(angle), std::sin(angle));
            sum += input[n] * twiddle;
        }
        output[k] = sum;
    }
}

