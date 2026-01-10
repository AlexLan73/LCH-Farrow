# 🚨 14 ЧАСТЫХ ПРОБЛЕМ И РЕШЕНИЯ

## ПРОБЛЕМА 1: CMake не находит CUDA
**Симптом:** CMake Error: Could not find CUDA
**Решение:**
```bash
cmake .. -DCUDA_TOOLKIT_ROOT_DIR="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.0"
```

## ПРОБЛЕМА 2: cuFFT не линкуется
**Симптом:** undefined reference to cufftExecC2C
**Решение в CMakeLists.txt:**
```cmake
find_package(CUDAToolkit REQUIRED COMPONENTS cufft)
target_link_libraries(radar_convolver CUDA::cufft CUDA::cudart)
```

## ПРОБЛЕМА 3: Недостаточно GPU памяти
**Симптом:** cudaErrorMemoryAllocation
**Причина:** Отдельные input/output буферы
**Решение:** Используйте in-place обработку
```cpp
kernel<<<>>>(d_buffer, d_buffer);  // in-place!
```

## ПРОБЛЕМА 4: Результаты мусор / случайные
**Симптом:** L2 ошибка > 1e-3, разные результаты при каждом запуске
**Причина:** Missing cudaDeviceSynchronize()
**Решение:**
```cpp
kernel<<<>>>(...);
cudaDeviceSynchronize();  // ЖДИТЕ!
cudaMemcpy(...);
```

## ПРОБЛЕМА 5: cuFFT batch не работает
**Симптом:** Неправильные результаты после FFT
**Причина:** Неправильный batch size в плане
**Решение:**
```cpp
cufftPlan1d(&plan, 1300000, CUFFT_C2C, 256);  // batch=256!
cufftExecC2C(plan, d_buffer, d_buffer, CUFFT_FORWARD);
```

## ПРОБЛЕМА 6: Программа медленная (15+ сек)
**Симптом:** Ожидали 4.65 сек, получили 15+
**Причина:** FFT опорного сигнала в цикле
**Решение:**
```cpp
// Один раз:
reference_fft = PrecomputeReferenceFft();

// В цикле - переиспользуем:
for (int b = 0; b < 256; b++) {
    multiply_kernel(beam[b], reference_fft);
}
```

## ПРОБЛЕМА 7: Pinned memory allocation fails
**Симптом:** cudaErrorMemoryAllocation
**Решение:**
```cpp
cudaMallocHost(&host_input, size);  // Pinned
// ... use ...
cudaFreeHost(host_input);            // Unpinned
```

## ПРОБЛЕМА 8: Kernel timeout (TDR на Windows)
**Симптом:** GPU hangs, приложение зависает
**Решение:**
```
Registry: HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\GraphicsDrivers
Key: TdrDelay (DWORD)
Value: 60  (увеличьте если нужно)
```

## ПРОБЛЕМА 9: VSCode красные волнистые линии (IntelliSense)
**Симптом:** Red squiggles в CUDA коде, но код работает
**Решение в .vscode/settings.json:**
```json
{
    "cmake.configureArgs": [
        "-DCUDA_TOOLKIT_ROOT_DIR=C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.0"
    ]
}
```

## ПРОБЛЕМА 10: Memory leak (утечка памяти)
**Симптом:** Память GPU растёт при каждом запуске
**Решение:**
```bash
cuda-memcheck ./radar_convolver
```
Убедитесь что всё освобождается в destructor:
```cpp
~SignalBuffer() {
    cudaFree(d_buffer);
    cudaFreeHost(h_buffer);
}
```

## ПРОБЛЕМА 11: H2D transfer очень медленный
**Симптом:** 170 мс → 500+ мс
**Причина:** Host memory не pinned
**Решение:**
```cpp
cudaMallocHost(&host_input, size);  // Pinned = быстрый transfer
cudaMemcpyAsync(d_input, host_input, size, cudaMemcpyHostToDevice);
```

## ПРОБЛЕМА 12: Compilation time очень долгая
**Симптом:** cmake --build занимает 10+ минут
**Причина:** CUDA kernel compilation
**Решение:** Увеличьте параллелизм
```bash
cmake --build . -j 8  # Используйте 8 потоков
```

## ПРОБЛЕМА 13: GitHub Actions fails на Linux
**Симптом:** Ubuntu builder не находит CUDA
**Решение в workflow:**
```yaml
- name: Install CUDA
  run: |
    sudo apt-get update
    sudo apt-get install -y nvidia-cuda-toolkit
```

## ПРОБЛЕМА 14: Floating point precision issues
**Симптом:** Результаты немного отличаются от CPU версии
**Причина:** GPU использует другой порядок вычислений
**Решение:** Используйте relative tolerance вместо absolute
```cpp
float relative_error = abs(gpu_result - cpu_result) / abs(cpu_result);
if (relative_error < 1e-5) OK();  // ✅
```

---

## ТАБЛИЦА ОТЛАДКИ

| Ошибка | Файл | Строка | Решение |
|--------|------|--------|---------|
| CUDA not found | CMakeLists.txt | find_package | Установить CUDA 13.0 |
| cuFFT undefined | CMakeLists.txt | target_link | Добавить CUDA::cufft |
| OOM | signal_buffer.cpp | allocate | Использовать in-place |
| Garbage output | kernel.cu | launch | Добавить sync после |
| 15+ sec | processing.cpp | loop | Вычислить reference FFT один раз |
| Pinned fails | signal_buffer.cpp | cudaMallocHost | Проверить доступную память |
| TDR timeout | kernel.cu | all | Сократить kernel время |
| Memory leak | destructor | cudaFree | Добавить cudaFreeHost |

---

## STATUS: ГОТОВЫ К ОТЛАДКЕ ✅