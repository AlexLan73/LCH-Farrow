#!/bin/bash

# ============================================================================
# BUILD SCRIPT для Farrow Filter с ЛЧМ сигналами и матрицей 48×5
# ============================================================================

echo "╔════════════════════════════════════════════════════════════════════════╗"
echo "║  Farrow Filter for Chirp Signals - Build Script                       ║"
echo "║  Platform: Linux / macOS with NVIDIA CUDA OpenCL                      ║"
echo "╚════════════════════════════════════════════════════════════════════════╝"
echo ""

# ПРОВЕРИТЬ ЗАВИСИМОСТИ
echo "Checking dependencies..."

if ! command -v g++ &> /dev/null; then
    echo "❌ g++ not found"
    exit 1
fi
echo "✓ g++ found"

# НАЙТИ ПУТИ CUDA
CUDA_PATH="/usr/local/cuda"
CUDA_LIB_PATH="/usr/local/cuda/lib64"

if [ ! -d "$CUDA_PATH" ]; then
    CUDA_PATH="/usr"
    CUDA_LIB_PATH="/usr/lib/x86_64-linux-gnu"
fi

echo "CUDA path: $CUDA_PATH"
echo ""

# КОМПИЛЯЦИЯ
echo "Compiling Farrow Filter for Chirp Signals..."

g++ -O3 -std=c++11 \
    -I"$CUDA_PATH/include" \
    -L"$CUDA_LIB_PATH" \
    -lOpenCL \
    -lm \
    farrow_chirp_host.cpp \
    -o farrow_chirp_test

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✓ Build successful!"
echo ""
echo "To run: ./farrow_chirp_test"
echo ""
