# ⚡ БЫСТРЫЙ СПРАВОЧНИК

## КОМАНДЫ CMAKE

### WINDOWS (VS2022)
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -DCUDA_TOOLKIT_ROOT_DIR="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.0"
cmake --build . --config Release -j
.\Release\radar_convolver.exe --input ..\data\lfm_signal.bin
```

### UBUNTU/LINUX
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
make -j8
./radar_convolver --input ../data/lfm_signal.bin
```

---

## КЛАССЫ КОТОРЫЕ ВЫ НАПИШЕТЕ

### SignalBuffer
```cpp
class SignalBuffer {
    void LoadFromFile(const string& filename);
    void SaveToFile(const string& filename);
    complex<float>* GetBeam(int beam_id);
    int GetNumBeams() const;
    int GetNumSamples() const;
};
```

### FilterBank
```cpp
class FilterBank {
    void LoadCoefficients(const vector<float>& coeffs);
    void PrecomputeReferenceFft();
    const complex<float>* GetReferenceFft() const;
    int GetNumCoefficients() const;
};
```

### ProcessingPipeline
```cpp
class ProcessingPipeline {
    void ExecuteFull();
    void ProfileKernel();
    void ValidateResults();
    const ProfilingMetrics& GetMetrics() const;
};
```

### GPUFactory
```cpp
class GPUFactory {
    static IGPUBackend* CreateBackend();
};
```

### ProfilingEngine
```cpp
class ProfilingEngine {
    void StartTimer();
    void StopTimer();
    float GetElapsedMs() const;
    void ReportMetrics();
};
```

---

## PERFORMANCE TARGETS

| Метрика | RTX 2080 Ti | RTX 3060 |
|---------|------------|---------|
| Total E2E | 4.65 сек | 7.3 сек |
| H2D transfer | 170 мс | 170 мс |
| GPU kernel (delay) | 4200 мс | 6600 мс |
| cuFFT forward | 60 мс | 95 мс |
| Hadamard multiply | 5 мс | 8 мс |
| cuFFT inverse | 60 мс | 95 мс |
| D2H transfer | 170 мс | 170 мс |

---

## ВАЖНЫЕ ПАТТЕРНЫ

### In-place обработка
```cpp
// НЕ так:
float* input = allocate(2.66GB);
float* output = allocate(2.66GB);
kernel<<<>>>(input, output);  // Тратит 8 ГБ!

// ТАК:
float* buffer = allocate(2.66GB);
kernel<<<>>>(buffer, buffer);  // 2.66 ГБ, in-place
```

### Batch FFT
```cpp
// План для 256 пакета, 1.3M размер
cufftPlan1d(&plan, 1300000, CUFFT_C2C, 256);
cufftExecC2C(plan, d_buffer, d_buffer, CUFFT_FORWARD);
// Все 256 лучей в одном вызове!
```

### Предвычисленная опорная FFT
```cpp
// В инициализации:
complex<float>* reference_fft = new complex<float>[1300000];
cufftExecC2C(plan, d_reference, reference_fft, CUFFT_FORWARD);
cudaMemcpy(d_reference_fft, reference_fft, ...);

// В цикле:
for (int b = 0; b < 256; b++) {
    hadamard_kernel<<<>>>(d_beams[b], d_reference_fft);
}
```

---

## ПРОФИЛИРОВАНИЕ

```cpp
cudaEvent_t start, stop;
cudaEventCreate(&start);
cudaEventCreate(&stop);

cudaEventRecord(start);
kernel<<<blocks, threads>>>();
cudaEventRecord(stop);
cudaEventSynchronize(stop);

float ms = 0;
cudaEventElapsedTime(&ms, start, stop);
printf("Kernel time: %.2f ms\n", ms);
```

---

## ПРОВЕРКА КОРРЕКТНОСТИ

```cpp
// 1. Синхронизация обязательна
kernel<<<>>>();
cudaDeviceSynchronize();  // ЖДИТЕ!

// 2. Pinned memory для H2D/D2H
cudaMallocHost(&host_input, size);
cudaMemcpyAsync(d_input, host_input, size, cudaMemcpyHostToDevice);

// 3. Проверьте ошибки CUDA
cudaError_t err = cudaGetLastError();
if (err != cudaSuccess) {
    fprintf(stderr, "CUDA error: %s\n", cudaGetErrorString(err));
}
```

---

## GIT WORKFLOW

```bash
git init
git add .gitignore README.md CMakeLists.txt
git commit -m "Initial setup"
git push origin main

# Weekly:
git commit -m "Week 1: Pure C++ foundation"
git commit -m "Week 2: GPU kernel + FFT"
git commit -m "Week 3: Multi-GPU + Ubuntu"
```

---

## ВАЖНЫЕ ССЫЛКИ

- CUDA Docs: https://docs.nvidia.com/cuda
- cuFFT: https://docs.nvidia.com/cuda/cufft
- CMake: https://cmake.org