# ⚡ 5-MINUTE CRASH COURSE: RadarConvolver

**Everything you need to know in 5 minutes. Go!**

---

## THE PROBLEM (What you're solving)

```
Input:  256 radar beams × 1.3 million samples each = 2.66 GB
Task 1: Apply fractional delay (drobная zaderjka)
Task 2: Convolve with reference signal
Output: Same 2.66 GB, processed

Challenge: Do this in ~5 seconds on GPU
```

---

## THE SOLUTION (High-level)

```
GPU PIPELINE (all on GPU, nothing goes back to CPU until end):

[Load 2.66 GB] 
    ↓
[GPU Kernel: Fractional Delay] — 4.2 seconds
    ↓ (result stays on GPU)
[FFT forward] — 60 ms
    ↓ (still on GPU)
[Multiply with reference] — 5 ms  
    ↓ (still on GPU)
[FFT inverse] — 60 ms
    ↓ (still on GPU)
[Transfer back to CPU] — 0.17 seconds

TOTAL: 4.65 seconds ✅
```

---

## WHY THIS IS BRILLIANT

### 1. Memory Magic 💾
- You have: 11 GB GPU RAM
- You need: 2.66 GB (input only)
- You use: **In-place processing** (reuse same buffer)
- Result: 50% memory savings, very clean

### 2. Speed Magic ⚡
- FIR convolution: 312 million operations → slow
- FFT convolution: 41 million operations → fast (7.6× fewer!)
- But you can't do fractional delay in frequency domain
- Solution: Fractional delay THEN FFT (sequential, optimal)

### 3. Architecture Magic 🏛️
- **One codebase** works on:
  - Windows (RTX 2080 Ti)
  - Ubuntu (RTX 3060)
  - Future: RX 6900 XT (HIP backend)
  - Future: MI300X (tensor ops)
- How? **Virtual backend pattern** = no code duplication

---

## YOUR HARDWARE (Today)

```
HOME (Windows, VS2022):
└─ RTX 2080 Ti
   ├─ 11 GB VRAM ✅
   ├─ 616 GB/s bandwidth ✅
   └─ Expected time: 4.65 seconds

WORK (Ubuntu, later):
└─ RTX 3060
   ├─ 12 GB VRAM ✅
   ├─ 360 GB/s bandwidth (slower)
   └─ Expected time: 7.3 seconds

FUTURE (maybe):
├─ RX 6900 XT (AMD RDNA2)
├─ MI300X (AMD CDNA3, mega fast!)
└─ Both use HIP instead of CUDA (different but same pattern)
```

---

## THE ARCHITECTURE (3 Layers)

```
Layer 1: Application (Pure C++, works everywhere)
├─ SignalBuffer (load/save 2.66 GB)
├─ FilterBank (240 FIR coefficients)
├─ ProcessingPipeline (orchestrates GPU)
└─ ProfilingEngine (measures timing)

Layer 2: GPU Abstraction (Virtual interface)
├─ IGPUBackend (abstract class)
├─ CUDABackend (for NVIDIA)
└─ HIPBackend (for AMD, future)

Layer 3: GPU Code (Platform-specific)
├─ kernel_fractional_delay.cu
├─ kernel_hadamard.cu
├─ cuFFT library calls
└─ Similar for HIP kernels (future)

Magic: Application layer doesn't care which GPU!
```

---

## TECH STACK (You'll use)

```
Language:      C++17 (modern, fast)
GPU:           CUDA 13.0 (NVIDIA)
Build:         CMake + VSCode (cross-platform)
FFT:           cuFFT library (included with CUDA)
Version:       Git + GitHub
Testing:       Custom validation + stress tests
```

---

## THE 3-WEEK PLAN

```
WEEK 1: Foundation
├─ Days 1-2: CMake setup + VSCode config
├─ Days 3-4: Pure C++ classes (buffer, bank, factory)
└─ Days 5-7: First GPU kernel (fractional delay, 4.2 sec)

WEEK 2: Pipeline
├─ Days 1-2: Reference signal FFT (precompute once!)
├─ Days 3-4: cuFFT integration + Hadamard kernel
└─ Days 5-7: Full E2E test (4.65 sec total)

WEEK 3: Validation
├─ Days 1-2: Optimize performance
├─ Days 3-4: Port to Ubuntu + test RTX 3060
└─ Days 5-7: Stress test + documentation
```

---

## 3 CRITICAL INSIGHTS (Must understand before coding!)

### #1: In-Place Processing
```
NAIVE (uses 8 GB):
  input[2.66GB] → kernel → output[2.66GB]  ❌ wastes memory

SMART (uses 2.66 GB):
  buffer[2.66GB] → kernel (writes to same buffer)
  buffer → FFT forward (in-place!)
  buffer → multiply (in-place!)
  buffer → FFT inverse (in-place!)
  ✅ Elegant, efficient, clean
```

### #2: Batch FFT (All 256 beams at once)
```
SLOW (256 separate FFT calls):
  for (int b = 0; b < 256; b++) {
      cufftExec(...);  // 60ms × 256 = 15 seconds!! ❌
  }

FAST (batch of 256):
  cufftPlan1d(..., batch_size=256);
  cufftExec(...);  // 60 ms total ✅
  All 256 beams processed in PARALLEL!
```

### #3: Reference FFT Precomputed
```
NAIVE (compute every time):
  for (int beam = 0; beam < 256; beam++) {
      FFT(reference)  // 60ms × 256 = 15 sec ❌
  }

SMART (compute once):
  FFT(reference)  // 60 ms, ONE TIME
  cache_it()
  
  for (int beam = 0; beam < 256; beam++) {
      multiply_with_cached(beam)  // 5ms × 256 = 1.3 sec ✅
  }
```

---

## WHAT YOU'LL BUILD (Files you create)

```
src/
├─ signal_buffer.h/cpp         (load 2.66 GB from disk)
├─ filter_bank.h/cpp           (manage 240 FIR coefficients)
├─ gpu_factory.h/cpp           (auto-detect GPU, pick best)
├─ processing_pipeline.h/cpp   (orchestrate: delay→FFT→output)
├─ profiling_engine.h/cpp      (measure timing)
├─ gpu_backend/
│  └─ cuda/
│     ├─ cuda_backend.h/cpp    (dispatch to GPU)
│     ├─ kernel_fractional_delay.cu   (4.2 sec kernel)
│     ├─ kernel_hadamard.cu          (5 ms multiply kernel)
│     └─ cufft_wrapper.h/cpp   (cuFFT helper)
└─ main.cpp                    (entry point)

Total: ~1200 lines of code (not including kernels)
```

---

## PERFORMANCE VALIDATION CHECKLIST (What "done" looks like)

```
✅ RTX 2080 Ti: 4.5-5 seconds (expect 4.65 sec)
✅ GPU memory: < 4 GB used (expect 3.2 GB)
✅ CPU accuracy: L2 error < 1e-5 (float32 precision)
✅ No memory leaks: same memory after 100 runs
✅ Deterministic: identical results (run 10 times)
✅ Works on Ubuntu: port to RTX 3060
✅ No GPU hangs: completes in reasonable time
✅ Breakdown: 90% in compute, 7% in transfers (good!)
```

---

## BIGGEST CHALLENGES (Be prepared!)

### 1. CMake Configuration 
- Problem: CUDA path incorrect, cuFFT not found
- Solution: Copy template from final_architecture_with_fft.md
- Time to fix: 15 minutes once you know it

### 2. GPU Memory Management
- Problem: Out of memory, garbage results, crashes
- Solution: Use in-place processing (only 2.66 GB)
- Time to fix: 30 minutes if you understand architecture

### 3. Synchronization Bugs
- Problem: Results are random, kernel outputs garbage
- Solution: Add cudaDeviceSynchronize() after kernels
- Time to fix: 5 minutes

### 4. Reference FFT Loop
- Problem: Program runs 15+ seconds instead of 4.65
- Solution: Precompute reference FFT once, not per-beam
- Time to fix: 10 minutes

**Total potential blockers: ~1 hour of debugging**  
**(if you know what to look for!)**

---

## YOUR DOCUMENTATION (Use as reference)

```
Quick lookup:
├─ INDEX.md                      (map of everything)
├─ FINAL_SUMMARY.md              (complete overview)
├─ README_START_HERE.md          (quick start)
├─ final_architecture_with_fft.md (detailed design + CMakeLists template)
├─ QUICK_COMMANDS.sh              (copy-paste commands)
├─ quick_reference_guide.md       (practical patterns)
├─ common_pitfalls_solutions.md   (debug problems)
└─ multi_gpu_strategy.md          (future GPU support)
```

**Where to start:** FINAL_SUMMARY.md (10 min) → then README_START_HERE.md (15 min)

---

## IMMEDIATE NEXT STEPS (Right now!)

```
☐ Verify CUDA 13.0:  nvcc --version
☐ Verify GPU:        nvidia-smi
☐ Read FINAL_SUMMARY.md  (10 minutes)
☐ Read README_START_HERE.md  (15 minutes)
☐ Create GitHub repo
☐ Clone locally
☐ Create folder structure
☐ Copy CMakeLists.txt template from docs
☐ Try first cmake build
☐ If build works: ✅ you're ready to code!
```

**Time commitment:** 1 hour setup, then 3 weeks coding

---

## SUCCESS DEFINITION

You'll know you're done when:
1. Program compiles on Windows + RTX 2080 Ti
2. E2E time is 4.5-5.5 seconds (within 10% of target)
3. Code compiles on Ubuntu (RTX 3060)
4. Performance is documented in CSV report
5. GitHub repo is clean and documented
6. Future developer can clone and build immediately

---

## THE SECRET INGREDIENT

> The hardest part **won't be the GPU code.**  
> It will be getting CMake, CUDA, and VSCode to play nicely together.

**Pro tip:** Spend 2-3 hours getting CMakeLists.txt perfect. Then everything else flows naturally.

---

## ONE FINAL MOTIVATIONAL FACT

**What you're building:**
- Processes 256 radar beams simultaneously
- 1.3 million samples per beam
- Fractional delay + FFT convolution
- In under 5 seconds
- On consumer hardware (RTX 2080 Ti)
- With clean, cross-platform C++ code
- That scales to MI300X (1.5-2 sec!)

**That's professional-grade signal processing.** 🚀

---

## GO BUILD! 🎉

**You have:**
✅ Complete architecture  
✅ All decisions made  
✅ Detailed documentation  
✅ Visual diagrams  
✅ Command reference  
✅ Troubleshooting guide  

**You're ready.**

```
$ git clone <your repo>
$ cd RadarConvolver
$ mkdir build && cd build
$ cmake ..
$ cmake --build . --config Release
$ .\Release\radar_convolver.exe

[GPU processing for 4.65 seconds...]
[Results saved]

✅ SUCCESS
```

---

**Now go! And have fun coding!** 🚀

Questions? See INDEX.md for complete documentation map.