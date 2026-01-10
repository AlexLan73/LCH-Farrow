#include "processing_pipeline.h"
#include "lagrange_matrix.h"
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
      device_buffer_size_(0) {
}

ProcessingPipeline::~ProcessingPipeline() {
    FreeDeviceMemory();
}

bool ProcessingPipeline::ExecuteFull(bool copy_to_host) {
    if (!signal_buffer_ || !gpu_backend_ || !profiler_) {
        std::cerr << "Ошибка: не все компоненты инициализированы" << std::endl;
        return false;
    }
    
    // 1. Выделить память на GPU
    if (!AllocateDeviceMemory()) {
        return false;
    }
    
    // 2. H2D Transfer (загрузка данных на GPU)
    profiler_->StartTimer("H2D_Transfer");
    if (!CopyHostToDevice()) {
        profiler_->StopTimer("H2D_Transfer");
        return false;
    }
    profiler_->StopTimer("H2D_Transfer");
    
    // 3. Дробная задержка (формирование матрицы с задержанными сигналами)
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
    
    // 4. Опционально: D2H Transfer (вывод с GPU для анализа)
    if (copy_to_host) {
        profiler_->StartTimer("D2H_Transfer");
        if (!CopyDeviceToHost()) {
            profiler_->StopTimer("D2H_Transfer");
            return false;
        }
        profiler_->StopTimer("D2H_Transfer");
        std::cout << "✅ Данные скопированы с GPU на хост для анализа" << std::endl;
    } else {
        std::cout << "✅ Матрица с задержанными сигналами осталась на GPU для дальнейшей обработки" << std::endl;
    }
    
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
    
    return true;
}

void ProcessingPipeline::FreeDeviceMemory() {
    if (device_buffer_ != nullptr) {
        gpu_backend_->FreeDeviceMemory(device_buffer_);
        device_buffer_ = nullptr;
    }
    
    device_buffer_size_ = 0;
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

