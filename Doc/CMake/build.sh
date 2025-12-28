#!/bin/bash

# ============================================================================
# Build Configuration Script для Linux/Ubuntu
# ============================================================================

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функция для вывода
print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}================================${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ️  $1${NC}"
}

# ============================================================================
# МЕНЮ
# ============================================================================

show_menu() {
    print_header "BUILD CONFIGURATION MENU"
    echo ""
    echo "1) CPU only (Release)"
    echo "2) CPU only (Debug)"
    echo "3) CUDA (Release) - Ninja"
    echo "4) CUDA (Debug) - Ninja"
    echo "5) OpenCL (Release) - Ninja"
    echo "6) Clean build directory"
    echo "7) Full rebuild (clean + build)"
    echo "8) Run executable"
    echo "9) Exit"
    echo ""
}

# ============================================================================
# BUILD FUNCTIONS
# ============================================================================

build_cpu_release() {
    print_header "Building CPU Release"
    rm -rf build
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=OFF -DENABLE_OPENCL=OFF
    if [ $? -eq 0 ]; then
        ninja -C build
        if [ $? -eq 0 ]; then
            print_success "Build completed!"
        else
            print_error "Build failed!"
        fi
    else
        print_error "CMake configuration failed!"
    fi
}

build_cpu_debug() {
    print_header "Building CPU Debug"
    rm -rf build
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_CUDA=OFF -DENABLE_OPENCL=OFF
    if [ $? -eq 0 ]; then
        ninja -C build
        if [ $? -eq 0 ]; then
            print_success "Build completed!"
        else
            print_error "Build failed!"
        fi
    else
        print_error "CMake configuration failed!"
    fi
}

build_cuda_release() {
    print_header "Building CUDA Release"
    rm -rf build
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=ON -DCUDA_ARCH="70;75;80"
    if [ $? -eq 0 ]; then
        ninja -C build
        if [ $? -eq 0 ]; then
            print_success "Build with CUDA completed!"
        else
            print_error "Build failed!"
        fi
    else
        print_error "CMake configuration failed! Is CUDA installed?"
    fi
}

build_cuda_debug() {
    print_header "Building CUDA Debug"
    rm -rf build
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_CUDA=ON -DCUDA_ARCH="70;75;80"
    if [ $? -eq 0 ]; then
        ninja -C build
        if [ $? -eq 0 ]; then
            print_success "Build with CUDA (Debug) completed!"
        else
            print_error "Build failed!"
        fi
    else
        print_error "CMake configuration failed! Is CUDA installed?"
    fi
}

build_opencl_release() {
    print_header "Building OpenCL Release"
    rm -rf build
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENCL=ON
    if [ $? -eq 0 ]; then
        ninja -C build
        if [ $? -eq 0 ]; then
            print_success "Build with OpenCL completed!"
        else
            print_error "Build failed!"
        fi
    else
        print_error "CMake configuration failed! Is OpenCL installed?"
    fi
}

clean_build() {
    print_header "Cleaning Build Directory"
    rm -rf build
    print_success "Build directory cleaned!"
}

full_rebuild() {
    print_header "Full Rebuild"
    clean_build
    build_cpu_release
}

run_executable() {
    print_header "Running Executable"
    if [ -f "./build/LCH-Farrow" ]; then
        ./build/LCH-Farrow
    else
        print_error "Executable not found! Build first."
    fi
}

# ============================================================================
# MAIN LOOP
# ============================================================================

while true; do
    show_menu
    read -p "Select option (1-9): " choice
    
    case $choice in
        1) build_cpu_release ;;
        2) build_cpu_debug ;;
        3) build_cuda_release ;;
        4) build_cuda_debug ;;
        5) build_opencl_release ;;
        6) clean_build ;;
        7) full_rebuild ;;
        8) run_executable ;;
        9) 
            print_info "Goodbye!"
            exit 0
            ;;
        *)
            print_error "Invalid option!"
            ;;
    esac
    
    echo ""
    read -p "Press Enter to continue..."
done

