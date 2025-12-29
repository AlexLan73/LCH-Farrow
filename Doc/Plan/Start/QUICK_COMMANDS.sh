#!/bin/bash
# RadarConvolver: Шпаргалка команд

# ============================================================
# ПРОВЕРКА ОКРУЖЕНИЯ
# ============================================================

# Проверить CUDA
nvcc --version

# Проверить GPU
nvidia-smi

# Проверить версию GCC (Linux)
gcc --version

# ============================================================
# WINDOWS (VS2022) - CMAKE
# ============================================================

# Создание build директории
mkdir build && cd build

# CMake конфиг
cmake .. -G "Visual Studio 17 2022" -DCUDA_TOOLKIT_ROOT_DIR="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.0"

# Сборка
cmake --build . --config Release -j 8

# Запуск
.\Release\radar_convolver.exe --input ..\data\lfm_signal.bin

# ============================================================
# LINUX/UBUNTU - CMAKE
# ============================================================

# Создание build
mkdir build && cd build

# CMake конфиг
cmake .. -DCMAKE_BUILD_TYPE=Release -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda

# Сборка
make -j8

# Запуск
./radar_convolver --input ../data/lfm_signal.bin

# ============================================================
# GIT WORKFLOW
# ============================================================

# Инициализация
git init
git add .gitignore CMakeLists.txt .vscode/ src/
git commit -m "Initial architecture setup"
git remote add origin https://github.com/YOUR_USERNAME/RadarConvolver.git
git branch -M main
git push -u origin main

# Еженедельные коммиты
git add .
git commit -m "Week 1: C++ foundation (buffer, bank, factory)"
git push origin main

git commit -m "Week 2: GPU kernel + FFT integration"
git push origin main

git commit -m "Week 3: Multi-GPU + Ubuntu support"
git push origin main

# ============================================================
# ПРОФИЛИРОВАНИЕ И ОТЛАДКА
# ============================================================

# Смотреть GPU память в реальном времени
nvidia-smi --query-gpu=memory.used,memory.total --format=csv,noheader -l 1

# Или
watch -n 0.1 nvidia-smi

# Проверить утечки памяти
cuda-memcheck ./radar_convolver

# Профилирование NVIDIA
nvprof ./radar_convolver

# ============================================================
# CMAKE REBUILD (если ошибка в CMakeLists.txt)
# ============================================================

cd build
rm -rf *
cmake .. -DCUDA_TOOLKIT_ROOT_DIR="YOUR_PATH"
cmake --build . --config Release -j

# ============================================================
# ОЧИСТКА
# ============================================================

# Удалить build директорию
rm -rf build/

# Полная очистка Git
git clean -fdx

# ============================================================
# ПОМОЩЬ
# ============================================================

# Смотрите документацию:
# - START_HERE_5MIN.md (5 мин введение)
# - final_architecture_with_fft.md (полная архитектура)
# - common_pitfalls_solutions.md (если ошибка)