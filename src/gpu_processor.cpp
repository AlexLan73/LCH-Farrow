#include "gpu_processor.h"
#include "gpu_backend/gpu_factory.h"
#include "gpu_backend/opencl_backend.h"
#include <iostream>
#include <cstring>

using namespace std;

namespace radar {

struct GPUProcessor::Impl {
    std::unique_ptr<IGPUBackend> backend;
};

GPUProcessor::GPUProcessor()
    : impl_(new Impl()) {
    impl_->backend = GPUFactory::CreateBackend();
    if (impl_->backend) impl_->backend->Initialize();
}

GPUProcessor::~GPUProcessor() {
    if (impl_ && impl_->backend) {
        impl_->backend->Cleanup();
    }
}

bool GPUProcessor::ProcessFractionalDelay(const SignalBuffer& input,
                                          const float* delay_coeffs,
                                          SignalBuffer& output) {
    if (!impl_ || !impl_->backend) return false;
    auto& backend = impl_->backend;

    size_t num_beams = input.GetNumBeams();
    size_t num_samples = input.GetNumSamples();
    size_t total = num_beams * num_samples;
    size_t buffer_size = total * sizeof(IGPUBackend::ComplexType);

    void* device_ptr = backend->AllocateDeviceMemory(buffer_size);
    if (!device_ptr) {
        cerr << "GPUProcessor: AllocateDeviceMemory failed\n";
        return false;
    }

    // Prepare contiguous host buffer
    std::vector<IGPUBackend::ComplexType> host_buf(total);
    for (size_t b = 0; b < num_beams; ++b) {
        const auto* src = input.GetBeamData(b);
        if (!src) { backend->FreeDeviceMemory(device_ptr); return false; }
        std::memcpy(host_buf.data() + b * num_samples, src, num_samples * sizeof(IGPUBackend::ComplexType));
    }

    if (!backend->CopyHostToDevice(device_ptr, host_buf.data(), buffer_size)) {
        cerr << "GPUProcessor: CopyHostToDevice failed\n";
        backend->FreeDeviceMemory(device_ptr);
        return false;
    }

    if (!backend->ExecuteFractionalDelay(device_ptr, delay_coeffs, num_beams, num_samples)) {
        cerr << "GPUProcessor: ExecuteFractionalDelay failed\n";
        backend->FreeDeviceMemory(device_ptr);
        return false;
    }

    std::vector<IGPUBackend::ComplexType> out_host(total);
    if (!backend->CopyDeviceToHost(out_host.data(), device_ptr, buffer_size)) {
        cerr << "GPUProcessor: CopyDeviceToHost failed\n";
        backend->FreeDeviceMemory(device_ptr);
        return false;
    }

    // Copy into output SignalBuffer
    for (size_t b = 0; b < num_beams; ++b) {
        auto* dst = output.GetBeamData(b);
        if (!dst) { backend->FreeDeviceMemory(device_ptr); return false; }
        std::memcpy(dst, out_host.data() + b * num_samples, num_samples * sizeof(IGPUBackend::ComplexType));
    }

    backend->FreeDeviceMemory(device_ptr);
    return true;
}

} // namespace radar
