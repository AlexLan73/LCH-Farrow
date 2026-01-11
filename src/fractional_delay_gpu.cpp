#include "fractional_delay_gpu.h"
#include "gpu_backend/opencl_backend.h"
#include "gpu_profiling.h"
#include <iostream>
#include <cstring>
#include <vector>

using namespace std;

namespace radar {

FractionalDelayGPU::FractionalDelayGPU(IGPUBackend* backend)
    : backend_(backend), profiling_enabled_(false), is_initialized_(false) {
    if (backend_) {
        is_initialized_ = backend_->Initialize();
        if (!is_initialized_) {
            cerr << "FractionalDelayGPU: Failed to initialize backend" << endl;
        }
    }
}

FractionalDelayGPU::~FractionalDelayGPU() {
    if (backend_) {
        backend_->Cleanup();
    }
}

bool FractionalDelayGPU::ProcessFractionalDelay(
    const SignalBuffer& input,
    const float* delay_coeffs,
    SignalBuffer& output,
    ProfilingEngine* profiling_engine) {
    
    if (!IsInitialized()) {
        cerr << "FractionalDelayGPU: Backend not initialized" << endl;
        return false;
    }
    
    if (!ValidateInput(input, delay_coeffs)) {
        cerr << "FractionalDelayGPU: Invalid input data" << endl;
        return false;
    }
    
    size_t num_beams = input.GetNumBeams();
    size_t num_samples = input.GetNumSamples();
    size_t total = num_beams * num_samples;
    size_t buffer_size = total * sizeof(IGPUBackend::ComplexType);
    
    // Allocate device memory
    void* device_ptr = backend_->AllocateDeviceMemory(buffer_size);
    if (!device_ptr) {
        cerr << "FractionalDelayGPU: AllocateDeviceMemory failed" << endl;
        return false;
    }
    
    // Prepare contiguous host buffer
    vector<IGPUBackend::ComplexType> host_buf(total);
    for (size_t b = 0; b < num_beams; ++b) {
        const auto* src = input.GetBeamData(b);
        if (!src) {
            backend_->FreeDeviceMemory(device_ptr);
            return false;
        }
        memcpy(host_buf.data() + b * num_samples, src, num_samples * sizeof(IGPUBackend::ComplexType));
    }
    
    // Copy to device
    if (profiling_engine) {
        profiling_engine->StartTimer("H2D_Copy");
    }
    
    if (!backend_->CopyHostToDevice(device_ptr, host_buf.data(), buffer_size)) {
        cerr << "FractionalDelayGPU: CopyHostToDevice failed" << endl;
        backend_->FreeDeviceMemory(device_ptr);
        return false;
    }
    
    if (profiling_engine) {
        profiling_engine->StopTimer("H2D_Copy");
        profiling_engine->StartTimer("FractionalDelay_Kernel");
    }
    
    // Execute fractional delay
    if (!backend_->ExecuteFractionalDelay(device_ptr, delay_coeffs, num_beams, num_samples)) {
        cerr << "FractionalDelayGPU: ExecuteFractionalDelay failed" << endl;
        backend_->FreeDeviceMemory(device_ptr);
        return false;
    }
    
    if (profiling_engine) {
        profiling_engine->StopTimer("FractionalDelay_Kernel");
        profiling_engine->StartTimer("D2H_Copy");
    }
    
    // Copy back to host
    vector<IGPUBackend::ComplexType> out_host(total);
    if (!backend_->CopyDeviceToHost(out_host.data(), device_ptr, buffer_size)) {
        cerr << "FractionalDelayGPU: CopyDeviceToHost failed" << endl;
        backend_->FreeDeviceMemory(device_ptr);
        return false;
    }
    
    if (profiling_engine) {
        profiling_engine->StopTimer("D2H_Copy");
    }
    
    // Copy into output SignalBuffer
    for (size_t b = 0; b < num_beams; ++b) {
        auto* dst = output.GetBeamData(b);
        if (!dst) {
            backend_->FreeDeviceMemory(device_ptr);
            return false;
        }
        memcpy(dst, out_host.data() + b * num_samples, num_samples * sizeof(IGPUBackend::ComplexType));
    }
    
    backend_->FreeDeviceMemory(device_ptr);
    return true;
}

bool FractionalDelayGPU::ProcessFractionalDelayWithDetailedProfiling(
    const SignalBuffer& input,
    const float* delay_coeffs,
    SignalBuffer& output,
    DetailedGPUProfiling& detailed_profiling) {
    
    if (!IsInitialized()) {
        cerr << "FractionalDelayGPU: Backend not initialized" << endl;
        return false;
    }
    
    if (!ValidateInput(input, delay_coeffs)) {
        cerr << "FractionalDelayGPU: Invalid input data" << endl;
        return false;
    }
    
    size_t num_beams = input.GetNumBeams();
    size_t num_samples = input.GetNumSamples();
    size_t total = num_beams * num_samples;
    size_t buffer_size = total * sizeof(IGPUBackend::ComplexType);
    
    // Get system info
    detailed_profiling.system_info = GetSystemInfo();
    
    // Allocate device memory
    void* device_ptr = backend_->AllocateDeviceMemory(buffer_size);
    if (!device_ptr) {
        cerr << "FractionalDelayGPU: AllocateDeviceMemory failed" << endl;
        return false;
    }
    
    // Prepare contiguous host buffer
    vector<IGPUBackend::ComplexType> host_buf(total);
    for (size_t b = 0; b < num_beams; ++b) {
        const auto* src = input.GetBeamData(b);
        if (!src) {
            backend_->FreeDeviceMemory(device_ptr);
            return false;
        }
        memcpy(host_buf.data() + b * num_samples, src, num_samples * sizeof(IGPUBackend::ComplexType));
    }
    
    // Copy to device with profiling
    cl::Event h2d_event;
    if (!CopyHostToDeviceWithProfiling(device_ptr, host_buf.data(), buffer_size, &h2d_event)) {
        cerr << "FractionalDelayGPU: CopyHostToDeviceWithProfiling failed" << endl;
        backend_->FreeDeviceMemory(device_ptr);
        return false;
    }
    
    // Execute fractional delay with profiling
    cl::Event kernel_event;
    if (!ExecuteFractionalDelayWithProfiling(device_ptr, delay_coeffs, num_beams, num_samples, &kernel_event)) {
        cerr << "FractionalDelayGPU: ExecuteFractionalDelayWithProfiling failed" << endl;
        backend_->FreeDeviceMemory(device_ptr);
        return false;
    }
    
    // Copy back to host with profiling
    cl::Event d2h_event;
    vector<IGPUBackend::ComplexType> out_host(total);
    if (!CopyDeviceToHostWithProfiling(out_host.data(), device_ptr, buffer_size, &d2h_event)) {
        cerr << "FractionalDelayGPU: CopyDeviceToHostWithProfiling failed" << endl;
        backend_->FreeDeviceMemory(device_ptr);
        return false;
    }
    
    // Wait for all operations to complete
    h2d_event.wait();
    kernel_event.wait();
    d2h_event.wait();
    
    // Get event profiling data
    cl_ulong queued, submit, start, end;
    
    h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
    h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submit);
    h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    h2d_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    detailed_profiling.gpu_events.push_back(CalculateEventMetrics("H2D_Copy", queued, submit, start, end));
    
    kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
    kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submit);
    kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    detailed_profiling.gpu_events.push_back(CalculateEventMetrics("FractionalDelay_Kernel", queued, submit, start, end));
    
    d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_QUEUED, &queued);
    d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_SUBMIT, &submit);
    d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    d2h_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    detailed_profiling.gpu_events.push_back(CalculateEventMetrics("D2H_Copy", queued, submit, start, end));
    
    // Calculate total GPU time
    double total_gpu_time = 0.0;
    for (const auto& event : detailed_profiling.gpu_events) {
        total_gpu_time += event.total_time_ms;
    }
    detailed_profiling.total_gpu_time_ms = total_gpu_time;
    
    // Copy into output SignalBuffer
    for (size_t b = 0; b < num_beams; ++b) {
        auto* dst = output.GetBeamData(b);
        if (!dst) {
            backend_->FreeDeviceMemory(device_ptr);
            return false;
        }
        memcpy(dst, out_host.data() + b * num_samples, num_samples * sizeof(IGPUBackend::ComplexType));
    }
    
    backend_->FreeDeviceMemory(device_ptr);
    return true;
}

bool FractionalDelayGPU::UploadLagrangeMatrix(const float* lagrange_data) {
    if (!IsInitialized()) {
        cerr << "FractionalDelayGPU: Backend not initialized" << endl;
        return false;
    }
    
    return backend_->UploadLagrangeMatrix(lagrange_data);
}

SystemInfo FractionalDelayGPU::GetSystemInfo() const {
    SystemInfo info;
    if (backend_) {
        info = GetSystemInfo(backend_);
    }
    return info;
}

void FractionalDelayGPU::EnableProfiling(bool enable) {
    profiling_enabled_ = enable;
}

bool FractionalDelayGPU::IsInitialized() const {
    return is_initialized_;
}

bool FractionalDelayGPU::ValidateInput(const SignalBuffer& input, const float* delay_coeffs) const {
    if (!input.IsValid()) {
        cerr << "FractionalDelayGPU: Invalid input buffer" << endl;
        return false;
    }
    
    if (delay_coeffs == nullptr) {
        cerr << "FractionalDelayGPU: Delay coefficients is null" << endl;
        return false;
    }
    
    if (input.GetNumBeams() == 0 || input.GetNumSamples() == 0) {
        cerr << "FractionalDelayGPU: Zero beams or samples" << endl;
        return false;
    }
    
    return true;
}

bool FractionalDelayGPU::CopyHostToDeviceWithProfiling(
    void* device_ptr,
    const void* host_data,
    size_t size_bytes,
    cl::Event* event_out) {
    
    if (!backend_) {
        return false;
    }
    
    // Try to use OpenCL-specific method if available
    OpenCLBackend* opencl_backend = dynamic_cast<OpenCLBackend*>(backend_);
    if (opencl_backend && event_out) {
        return opencl_backend->CopyHostToDeviceWithProfiling(device_ptr, host_data, size_bytes, *event_out);
    }
    
    // Fallback to regular copy
    return backend_->CopyHostToDevice(device_ptr, host_data, size_bytes);
}

bool FractionalDelayGPU::CopyDeviceToHostWithProfiling(
    void* host_ptr,
    const void* device_data,
    size_t size_bytes,
    cl::Event* event_out) {
    
    if (!backend_) {
        return false;
    }
    
    // Try to use OpenCL-specific method if available
    OpenCLBackend* opencl_backend = dynamic_cast<OpenCLBackend*>(backend_);
    if (opencl_backend && event_out) {
        return opencl_backend->CopyDeviceToHostWithProfiling(host_ptr, device_data, size_bytes, *event_out);
    }
    
    // Fallback to regular copy
    return backend_->CopyDeviceToHost(host_ptr, device_data, size_bytes);
}

bool FractionalDelayGPU::ExecuteFractionalDelayWithProfiling(
    void* device_buffer,
    const float* delay_coeffs,
    size_t num_beams,
    size_t num_samples,
    cl::Event* event_out) {
    
    if (!backend_) {
        return false;
    }
    
    // Try to use OpenCL-specific method if available
    OpenCLBackend* opencl_backend = dynamic_cast<OpenCLBackend*>(backend_);
    if (opencl_backend && event_out) {
        return opencl_backend->ExecuteFractionalDelayWithProfiling(device_buffer, delay_coeffs, num_beams, num_samples, *event_out);
    }
    
    // Fallback to regular execution
    return backend_->ExecuteFractionalDelay(device_buffer, delay_coeffs, num_beams, num_samples);
}

} // namespace radar