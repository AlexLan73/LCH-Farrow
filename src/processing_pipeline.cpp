#include "processing_pipeline.h"
#include <iostream>
#include <cstring>

ProcessingPipeline::ProcessingPipeline(
    SignalBuffer* signal_buffer,
    FilterBank* filter_bank,
    IGPUBackend* gpu_backend,
    ProfilingEngine* profiler)
    : signal_buffer_(signal_buffer),
      filter_bank_(filter_bank),
      gpu_backend_(gpu_backend),
      profiler_(profiler),
      device_buffer_(nullptr),
      device_reference_fft_(nullptr),
      device_buffer_size_(0),
      device_reference_fft_size_(0) {
}

ProcessingPipeline::~ProcessingPipeline() {
    FreeDeviceMemory();
}

bool ProcessingPipeline::ExecuteFull() {
    if (!signal_buffer_ || !filter_bank_ || !gpu_backend_ || !profiler_) {
        std::cerr << "Ошибка: не все компоненты инициализированы" << std::endl;
        return false;
    }
    
    // 1. Предвычислить опорную FFT (если ещё не вычислена)
    if (!filter_bank_->IsReferenceFftComputed()) {
        profiler_->StartTimer("PrecomputeReferenceFft");
        filter_bank_->PrecomputeReferenceFft();
        profiler_->StopTimer("PrecomputeReferenceFft");
    }
    
    // 2. Выделить память на GPU
    if (!AllocateDeviceMemory()) {
        return false;
    }
    
    // 3. H2D Transfer
    profiler_->StartTimer("H2D_Transfer");
    if (!CopyHostToDevice()) {
        profiler_->StopTimer("H2D_Transfer");
        return false;
    }
    profiler_->StopTimer("H2D_Transfer");
    
    // 4. Дробная задержка
    profiler_->StartTimer("FractionalDelay");
    // TODO: Получить коэффициенты задержки (пока используем нули)
    std::vector<float> delay_coeffs(signal_buffer_->GetNumBeams(), 0.0f);
    if (!gpu_backend_->ExecuteFractionalDelay(
            device_buffer_,
            delay_coeffs.data(),
            signal_buffer_->GetNumBeams(),
            signal_buffer_->GetNumSamples())) {
        profiler_->StopTimer("FractionalDelay");
        return false;
    }
    profiler_->StopTimer("FractionalDelay");
    
    // 5. FFT Forward
    profiler_->StartTimer("FFT_Forward");
    if (!gpu_backend_->ExecuteFFT(
            device_buffer_,
            signal_buffer_->GetNumBeams(),
            signal_buffer_->GetNumSamples(),
            true)) {
        profiler_->StopTimer("FFT_Forward");
        return false;
    }
    profiler_->StopTimer("FFT_Forward");
    
    // 6. Hadamard Multiply
    profiler_->StartTimer("HadamardMultiply");
    if (!gpu_backend_->ExecuteHadamardMultiply(
            device_buffer_,
            device_reference_fft_,
            signal_buffer_->GetNumBeams(),
            signal_buffer_->GetNumSamples())) {
        profiler_->StopTimer("HadamardMultiply");
        return false;
    }
    profiler_->StopTimer("HadamardMultiply");
    
    // 7. IFFT Inverse
    profiler_->StartTimer("IFFT_Inverse");
    if (!gpu_backend_->ExecuteFFT(
            device_buffer_,
            signal_buffer_->GetNumBeams(),
            signal_buffer_->GetNumSamples(),
            false)) {
        profiler_->StopTimer("IFFT_Inverse");
        return false;
    }
    profiler_->StopTimer("IFFT_Inverse");
    
    // 8. D2H Transfer
    profiler_->StartTimer("D2H_Transfer");
    if (!CopyDeviceToHost()) {
        profiler_->StopTimer("D2H_Transfer");
        return false;
    }
    profiler_->StopTimer("D2H_Transfer");
    
    return true;
}

bool ProcessingPipeline::ExecuteStepByStep() {
    // Реализация для пошаговой отладки
    return ExecuteFull();  // Пока используем полный pipeline
}

bool ProcessingPipeline::ValidateResults(float tolerance) {
    // TODO: Реализовать валидацию результатов
    // Сравнение с reference implementation
    return true;
}

const ProfilingMetrics& ProcessingPipeline::GetMetrics() const {
    return profiler_->GetAllMetrics();
}

bool ProcessingPipeline::AllocateDeviceMemory() {
    if (device_buffer_ != nullptr) {
        // Память уже выделена
        return true;
    }
    
    // Размер буфера: num_beams * num_samples * sizeof(complex<float>)
    size_t num_beams = signal_buffer_->GetNumBeams();
    size_t num_samples = signal_buffer_->GetNumSamples();
    device_buffer_size_ = num_beams * num_samples * sizeof(SignalBuffer::ComplexType);
    
    device_buffer_ = gpu_backend_->AllocateDeviceMemory(device_buffer_size_);
    if (device_buffer_ == nullptr) {
        std::cerr << "Ошибка: не удалось выделить память для буфера сигналов" << std::endl;
        return false;
    }
    
    // Выделяем память для опорной FFT
    device_reference_fft_size_ = num_samples * sizeof(FilterBank::ComplexType);
    device_reference_fft_ = gpu_backend_->AllocateDeviceMemory(device_reference_fft_size_);
    if (device_reference_fft_ == nullptr) {
        std::cerr << "Ошибка: не удалось выделить память для опорной FFT" << std::endl;
        FreeDeviceMemory();
        return false;
    }
    
    // Копируем опорную FFT на устройство
    const FilterBank::ComplexType* ref_fft = filter_bank_->GetReferenceFft();
    if (ref_fft != nullptr) {
        if (!gpu_backend_->CopyHostToDevice(device_reference_fft_, ref_fft, device_reference_fft_size_)) {
            std::cerr << "Ошибка: не удалось скопировать опорную FFT на устройство" << std::endl;
            return false;
        }
    }
    
    return true;
}

void ProcessingPipeline::FreeDeviceMemory() {
    if (device_buffer_ != nullptr) {
        gpu_backend_->FreeDeviceMemory(device_buffer_);
        device_buffer_ = nullptr;
    }
    
    if (device_reference_fft_ != nullptr) {
        gpu_backend_->FreeDeviceMemory(device_reference_fft_);
        device_reference_fft_ = nullptr;
    }
    
    device_buffer_size_ = 0;
    device_reference_fft_size_ = 0;
}

bool ProcessingPipeline::CopyHostToDevice() {
    // Копируем все лучи в один буфер
    size_t num_beams = signal_buffer_->GetNumBeams();
    size_t num_samples = signal_buffer_->GetNumSamples();
    
    // Создаём временный буфер на хосте
    std::vector<SignalBuffer::ComplexType> host_buffer(num_beams * num_samples);
    
    for (size_t beam = 0; beam < num_beams; ++beam) {
        const SignalBuffer::ComplexType* beam_data = signal_buffer_->GetBeamData(beam);
        if (beam_data == nullptr) {
            return false;
        }
        std::memcpy(
            host_buffer.data() + beam * num_samples,
            beam_data,
            num_samples * sizeof(SignalBuffer::ComplexType)
        );
    }
    
    return gpu_backend_->CopyHostToDevice(
        device_buffer_,
        host_buffer.data(),
        device_buffer_size_
    );
}

bool ProcessingPipeline::CopyDeviceToHost() {
    size_t num_beams = signal_buffer_->GetNumBeams();
    size_t num_samples = signal_buffer_->GetNumSamples();
    
    // Создаём временный буфер на хосте
    std::vector<SignalBuffer::ComplexType> host_buffer(num_beams * num_samples);
    
    if (!gpu_backend_->CopyDeviceToHost(host_buffer.data(), device_buffer_, device_buffer_size_)) {
        return false;
    }
    
    // Копируем данные обратно в SignalBuffer
    for (size_t beam = 0; beam < num_beams; ++beam) {
        SignalBuffer::ComplexType* beam_data = signal_buffer_->GetBeamData(beam);
        if (beam_data == nullptr) {
            return false;
        }
        std::memcpy(
            beam_data,
            host_buffer.data() + beam * num_samples,
            num_samples * sizeof(SignalBuffer::ComplexType)
        );
    }
    
    return true;
}

