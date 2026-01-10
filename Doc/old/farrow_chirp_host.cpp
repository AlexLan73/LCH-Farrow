#include <CL/cl.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <random>

const int N = 1300000;
const int L = 48;
const int P = 4;
const int COEFF_SIZE = L * (P + 1);

const char* cl_error_to_string(cl_int err) {
    switch (err) {
        case CL_SUCCESS: return "CL_SUCCESS";
        case CL_DEVICE_NOT_FOUND: return "CL_DEVICE_NOT_FOUND";
        case CL_OUT_OF_RESOURCES: return "CL_OUT_OF_RESOURCES";
        default: return "UNKNOWN_ERROR";
    }
}

#define CHECK_CL(err, msg) \
    if ((err) != CL_SUCCESS) { \
        std::cerr << "❌ " << (msg) << " -> " << cl_error_to_string(err) << std::endl; \
        return -1; \
    }

void generate_chirp_signal(std::vector<float>& signal, float f_start, float f_end, float fs) {
    float T = N / fs;
    float k = (f_end - f_start) / T;
    
    std::cout << "Generating Chirp signal:" << std::endl;
    std::cout << "  f_start = " << f_start << " Hz" << std::endl;
    std::cout << "  f_end = " << f_end << " Hz" << std::endl;
    
    for (int n = 0; n < N; n++) {
        float t = n / fs;
        float phase = 2 * M_PI * (f_start * t + k * t * t / 2.0f);
        signal[n] = std::sin(phase);
    }
    
    std::cout << "✓ Chirp signal generated" << std::endl;
}

enum DelayMode { LINEAR = 0, SINUSOIDAL = 1, RANDOM = 2 };

void generate_variable_delay(std::vector<float>& delay_var, float mu_min, float mu_max, DelayMode mode) {
    std::cout << "\nGenerating variable delay μ[n]:" << std::endl;
    std::cout << "  μ_min = " << mu_min << ", μ_max = " << mu_max << std::endl;
    
    if (mode == LINEAR) {
        for (int n = 0; n < N; n++) {
            float t = (float)n / N;
            delay_var[n] = mu_min + (mu_max - mu_min) * t;
        }
        std::cout << "  Mode: LINEAR" << std::endl;
    } else if (mode == SINUSOIDAL) {
        float mu_center = (mu_min + mu_max) / 2.0f;
        float mu_amp = (mu_max - mu_min) / 2.0f;
        for (int n = 0; n < N; n++) {
            float t = (float)n / N;
            delay_var[n] = mu_center + mu_amp * std::sin(2.0f * M_PI * 10.0f * t);
        }
        std::cout << "  Mode: SINUSOIDAL" << std::endl;
    }
    
    std::cout << "✓ Variable delay generated" << std::endl;
}

bool load_coeffs_from_file(const char* filename, std::vector<float>& coeffs) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "⚠ File '" << filename << "' not found" << std::endl;
        coeffs.assign(COEFF_SIZE, 0.0f);
        for (int k = 0; k < L; k++) {
            if (k == L/2 - 1) coeffs[0 * L + k] = 1.0f;
        }
        return false;
    }
    
    int L_file = 0, P_file = 0;
    file >> L_file >> P_file;
    
    if (L_file != L || P_file != (P + 1)) {
        std::cerr << "❌ Error: expected " << L << "×" << (P+1) << std::endl;
        file.close();
        return false;
    }
    
    coeffs.resize(COEFF_SIZE);
    for (int p = 0; p <= P; p++) {
        for (int k = 0; k < L; k++) {
            file >> coeffs[p * L + k];
        }
    }
    
    file.close();
    std::cout << "✓ Coefficients loaded from " << filename << std::endl;
    return true;
}

int main() {
    std::cout << "Farrow Filter for Chirp Signals (48×5)" << std::endl;
    std::cout << "N=" << N << " samples, L=" << L << ", P=" << P << std::endl << std::endl;
    
    cl_int err = 0;
    
    std::vector<float> signal(N);
    generate_chirp_signal(signal, 100.0f, 500.0f, 1000.0f);
    
    std::vector<float> delay_var(N);
    generate_variable_delay(delay_var, 0.1f, 0.9f, SINUSOIDAL);
    std::cout << std::endl;
    
    std::vector<float> coeffs(COEFF_SIZE);
    load_coeffs_from_file("farrow_coeffs_48x5.txt", coeffs);
    std::cout << std::endl;
    
    std::cout << "Initializing OpenCL..." << std::endl;
    cl_platform_id platform = nullptr;
    err = clGetPlatformIDs(1, &platform, nullptr);
    CHECK_CL(err, "clGetPlatformIDs");
    
    cl_device_id device = nullptr;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    if (err != CL_SUCCESS) {
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, nullptr);
        CHECK_CL(err, "clGetDeviceIDs");
    }
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
    std::cout << "✓ Device: " << device_name << std::endl << std::endl;
    
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    CHECK_CL(err, "clCreateContext");
    
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    std::cout << "Allocating GPU buffers..." << std::endl;
    cl_mem buf_x = clCreateBuffer(context, CL_MEM_READ_ONLY, N * sizeof(float), nullptr, &err);
    CHECK_CL(err, "clCreateBuffer(input)");
    
    cl_mem buf_y = clCreateBuffer(context, CL_MEM_WRITE_ONLY, N * sizeof(float), nullptr, &err);
    CHECK_CL(err, "clCreateBuffer(output)");
    
    cl_mem buf_coeffs = clCreateBuffer(context, CL_MEM_READ_ONLY, COEFF_SIZE * sizeof(float), nullptr, &err);
    CHECK_CL(err, "clCreateBuffer(coeffs)");
    
    cl_mem buf_delay_var = clCreateBuffer(context, CL_MEM_READ_ONLY, N * sizeof(float), nullptr, &err);
    CHECK_CL(err, "clCreateBuffer(delay_var)");
    
    std::cout << "✓ Allocated " << (N * 2 * sizeof(float) / (1024*1024)) << " MB" << std::endl << std::endl;
    
    std::cout << "Uploading data to GPU..." << std::endl;
    err = clEnqueueWriteBuffer(queue, buf_x, CL_TRUE, 0, N * sizeof(float), signal.data(), 0, nullptr, nullptr);
    CHECK_CL(err, "clEnqueueWriteBuffer(signal)");
    
    err = clEnqueueWriteBuffer(queue, buf_coeffs, CL_TRUE, 0, COEFF_SIZE * sizeof(float), 
                               coeffs.data(), 0, nullptr, nullptr);
    CHECK_CL(err, "clEnqueueWriteBuffer(coeffs)");
    
    err = clEnqueueWriteBuffer(queue, buf_delay_var, CL_TRUE, 0, N * sizeof(float),
                               delay_var.data(), 0, nullptr, nullptr);
    CHECK_CL(err, "clEnqueueWriteBuffer(delay_var)");
    std::cout << "✓ Data uploaded" << std::endl << std::endl;
    
    std::cout << "Compiling kernel..." << std::endl;
    const char* kernel_source = R"(
__kernel void farrow_delay_chirp_super_fast(
    __global const float* x, __global float* y, __constant float* coeffs,
    __global const float* delay_var, int N, int L, int P
) {
    int n = get_global_id(0);
    if (n < L/2 || n >= N - L/2) return;
    
    float mu = delay_var[n];
    float mu2 = mu * mu, mu3 = mu2 * mu, mu4 = mu3 * mu;
    float y_0 = 0, y_1 = 0, y_2 = 0, y_3 = 0, y_4 = 0;
    
    for (int k = 0; k < L; k++) {
        int idx = n - (L/2 - 1) + k;
        float x_val = (idx >= 0 && idx < N) ? x[idx] : 0.0f;
        
        y_0 = mad(coeffs[0*L+k], x_val, y_0);
        y_1 = mad(coeffs[1*L+k], x_val, y_1);
        y_2 = mad(coeffs[2*L+k], x_val, y_2);
        y_3 = mad(coeffs[3*L+k], x_val, y_3);
        y_4 = mad(coeffs[4*L+k], x_val, y_4);
    }
    
    y[n] = y_0 + mu*y_1 + mu2*y_2 + mu3*y_3 + mu4*y_4;
}
)";
    
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, nullptr, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-mad-enable", nullptr, nullptr);
    if (err != CL_SUCCESS) {
        size_t log_size = 0;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        std::cerr << "❌ Build error:\n" << log.data() << std::endl;
        return -1;
    }
    std::cout << "✓ Kernel compiled" << std::endl << std::endl;
    
    cl_kernel kernel = clCreateKernel(program, "farrow_delay_chirp_super_fast", &err);
    CHECK_CL(err, "clCreateKernel");
    
    std::cout << "Running kernel..." << std::endl;
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_x);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_y);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_coeffs);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_delay_var);
    err |= clSetKernelArg(kernel, 4, sizeof(int), &N);
    err |= clSetKernelArg(kernel, 5, sizeof(int), &L);
    err |= clSetKernelArg(kernel, 6, sizeof(int), &P);
    CHECK_CL(err, "clSetKernelArg");
    
    size_t global_work_size = N;
    size_t local_work_size = 256;
    
    cl_event event;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, &event);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    clWaitForEvents(1, &event);
    
    cl_ulong start, end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, nullptr);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, nullptr);
    
    float elapsed_ms = (end - start) / 1e6f;
    std::cout << "✓ Kernel time: " << std::fixed << std::setprecision(2) << elapsed_ms << " ms" << std::endl << std::endl;
    clReleaseEvent(event);
    
    std::vector<float> output(N);
    std::cout << "Downloading results..." << std::endl;
    err = clEnqueueReadBuffer(queue, buf_y, CL_TRUE, 0, N * sizeof(float), output.data(), 0, nullptr, nullptr);
    CHECK_CL(err, "clEnqueueReadBuffer");
    std::cout << "✓ Done!" << std::endl << std::endl;
    
    std::cout << "=== VERIFICATION ===" << std::endl;
    float max_in = 0, max_out = 0;
    for (int n = L; n < N - L; n++) {
        max_in = std::max(max_in, std::abs(signal[n]));
        max_out = std::max(max_out, std::abs(output[n]));
    }
    std::cout << "Input max: " << max_in << std::endl;
    std::cout << "Output max: " << max_out << std::endl;
    std::cout << "Error: " << std::abs(max_in - max_out) / max_in * 100 << "%" << std::endl;
    
    clReleaseMemObject(buf_x);
    clReleaseMemObject(buf_y);
    clReleaseMemObject(buf_coeffs);
    clReleaseMemObject(buf_delay_var);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}
