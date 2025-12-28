# Build Configuration System для CMake
## Файл конфигурации вместо длинных команд в консоли

---

## 📋 ЧАСТЬ 1: Создать `CMakePresets.json`

Это **стандартный способ** хранить конфигурации в CMake!

**Файл: `CMakePresets.json` (в корне проекта)**

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "default",
      "description": "Default configuration",
      "hidden": true,
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "windows-msvc",
      "description": "Windows with Visual Studio 2022",
      "inherits": "default",
      "generator": "Visual Studio 17 2022",
      "cacheVariables": {
        "ENABLE_CUDA": "OFF",
        "ENABLE_OPENCL": "OFF"
      }
    },
    {
      "name": "windows-cuda",
      "description": "Windows with CUDA support",
      "inherits": "windows-msvc",
      "cacheVariables": {
        "ENABLE_CUDA": "ON",
        "CUDA_ARCH": "75;80"
      }
    },
    {
      "name": "linux-ninja",
      "description": "Linux with Ninja",
      "inherits": "default",
      "generator": "Ninja",
      "cacheVariables": {
        "ENABLE_CUDA": "OFF",
        "ENABLE_OPENCL": "OFF"
      }
    },
    {
      "name": "linux-cuda",
      "description": "Linux with CUDA and Ninja",
      "inherits": "linux-ninja",
      "cacheVariables": {
        "ENABLE_CUDA": "ON",
        "CUDA_ARCH": "70;75;80"
      }
    },
    {
      "name": "linux-opencl",
      "description": "Linux with OpenCL and Ninja",
      "inherits": "linux-ninja",
      "cacheVariables": {
        "ENABLE_OPENCL": "ON"
      }
    },
    {
      "name": "debug",
      "description": "Debug configuration",
      "inherits": "linux-ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "release",
      "configurePreset": "linux-ninja",
      "jobs": 4
    },
    {
      "name": "debug",
      "configurePreset": "debug",
      "jobs": 4
    }
  ]
}
```

---

## 📝 ЧАСТЬ 2: Создать `build.sh` для Linux

**Файл: `build.sh` (в корне проекта)**

```bash
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
```

---

## 🪟 ЧАСТЬ 3: Создать `build.bat` для Windows

**Файл: `build.bat` (в корне проекта)**

```batch
@echo off
REM ============================================================================
REM Build Configuration Script для Windows
REM ============================================================================

setlocal enabledelayedexpansion

REM Цвета (используем базовые цвета Windows)
cls

:MENU
echo.
echo ================================
echo    BUILD CONFIGURATION MENU
echo ================================
echo.
echo 1) CPU only (Release)
echo 2) CPU only (Debug)
echo 3) CUDA (Release)
echo 4) CUDA (Debug)
echo 5) Clean build directory
echo 6) Full rebuild
echo 7) Run executable
echo 8) Exit
echo.

set /p choice="Select option (1-8): "

if "%choice%"=="1" goto CPU_RELEASE
if "%choice%"=="2" goto CPU_DEBUG
if "%choice%"=="3" goto CUDA_RELEASE
if "%choice%"=="4" goto CUDA_DEBUG
if "%choice%"=="5" goto CLEAN
if "%choice%"=="6" goto FULL_REBUILD
if "%choice%"=="7" goto RUN
if "%choice%"=="8" goto EXIT_PROG
goto INVALID

:CPU_RELEASE
echo.
echo ================================
echo    Building CPU Release
echo ================================
if exist build rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022" -DENABLE_CUDA=OFF
cmake --build build --config Release
goto MENU

:CPU_DEBUG
echo.
echo ================================
echo    Building CPU Debug
echo ================================
if exist build rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022" -DENABLE_CUDA=OFF
cmake --build build --config Debug
goto MENU

:CUDA_RELEASE
echo.
echo ================================
echo    Building CUDA Release
echo ================================
if exist build rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022" -DENABLE_CUDA=ON -DCUDA_ARCH="75;80"
cmake --build build --config Release
goto MENU

:CUDA_DEBUG
echo.
echo ================================
echo    Building CUDA Debug
echo ================================
if exist build rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022" -DENABLE_CUDA=ON -DCUDA_ARCH="75;80"
cmake --build build --config Debug
goto MENU

:CLEAN
echo.
echo ================================
echo    Cleaning Build Directory
echo ================================
if exist build rmdir /s /q build
echo Build directory cleaned!
goto MENU

:FULL_REBUILD
echo.
echo ================================
echo    Full Rebuild
echo ================================
if exist build rmdir /s /q build
cmake -B build -G "Visual Studio 17 2022" -DENABLE_CUDA=OFF
cmake --build build --config Release
goto MENU

:RUN
echo.
echo ================================
echo    Running Executable
echo ================================
if exist "build\Release\LCH-Farrow.exe" (
    .\build\Release\LCH-Farrow.exe
) else (
    echo Executable not found! Build first.
)
goto MENU

:INVALID
echo Invalid option!
goto MENU

:EXIT_PROG
echo Goodbye!
exit /b 0
```

---

## 📝 ЧАСТЬ 4: Создать `.env` файл конфигурации

**Файл: `.env.build` (в корне проекта)**

```bash
# ============================================================================
# Build Configuration Environment
# ============================================================================

# === CUDA Configuration ===
ENABLE_CUDA=OFF
ENABLE_OPENCL=OFF
CUDA_ARCH="70;75;80"  # V100, RTX 2080, A100

# === Build Type ===
CMAKE_BUILD_TYPE=Release  # Release or Debug

# === Generator ===
# Linux: Ninja or Unix\ Makefiles
# Windows: Visual Studio 17 2022
CMAKE_GENERATOR=Ninja

# === Compiler ===
# Linux: gcc, clang
# Windows: msvc
COMPILER=gcc

# === Optimization ===
ENABLE_LTO=ON  # Link Time Optimization
ENABLE_FAST_MATH=ON

# === Directories ===
BUILD_DIR=build
INSTALL_DIR=/usr/local
```

---

## 🚀 КАК ИСПОЛЬЗОВАТЬ:

### На Linux:

```bash
# Сделать скрипт исполняемым
chmod +x build.sh

# Запустить меню
./build.sh

# Выбрать нужную конфигурацию (1-9)
```

### На Windows:

```cmd
# Просто запустить
build.bat

# Выбрать нужную конфигурацию (1-8)
```

### CMakePresets.json:

```bash
# Список доступных конфигураций
cmake --list-presets

# Собрать с конкретной конфигурацией
cmake --preset linux-cuda
cmake --build --preset release
```

---

## ✅ ГОТОВЫЕ КОНФИГУРАЦИИ:

| Конфиг | Платформа | GPU | Описание |
|--------|-----------|-----|---------|
| `windows-msvc` | Windows | ❌ | CPU only |
| `windows-cuda` | Windows | CUDA | CUDA поддержка |
| `linux-ninja` | Linux | ❌ | CPU only |
| `linux-cuda` | Linux | CUDA | CUDA поддержка |
| `linux-opencl` | Linux | OpenCL | OpenCL поддержка |
| `debug` | Linux | ❌ | Debug режим |

---

## 🎯 ДОБАВИТЬ В GITIGNORE:

```
# .gitignore - добавить:
build/
.cache/
.env.local
CMakeUserPresets.json
```

---

**Теперь просто запускаете `./build.sh` и выбираете конфигурацию!** 🎉

**Нет больше длинных команд!** 🚀
