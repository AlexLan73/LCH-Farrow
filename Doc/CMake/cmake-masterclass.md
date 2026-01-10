# CMake Master Class для C++ разработки в VS Code
## Windows & Ubuntu Edition

---

## ЧАСТЬ 1: ОСНОВЫ CMAKE

### 1.1 Что такое CMake и почему это важно

CMake — это кроссплатформенный инструмент автоматизации сборки, который:
- **Генерирует** build-системы (Visual Studio, Make, Ninja)
- **Управляет** зависимостями и библиотеками
- **Обеспечивает** переносимость между Windows/Linux/macOS
- **Упрощает** компиляцию проектов с GPU (CUDA, OpenCL, HIP)

Для GPU computing это критично: CMake легко находит CUDA Toolkit, настраивает компиляцию с nvcc, управляет библиотеками типа cuFFT, cuBLAS.

### 1.2 Установка CMake

**Windows:**
```bash
# Через скачивание с официального сайта
https://cmake.org/download/
# ИЛИ через package manager
choco install cmake
winget install kitware.cmake
```

**Ubuntu/Linux:**
```bash
sudo apt update
sudo apt install cmake build-essential git
cmake --version  # Проверка версии
```

Минимальная версия: **CMake 3.10+**
Рекомендуемая для CUDA: **CMake 3.17+**
Для современных GPU проектов: **CMake 3.20+**

---

## ЧАСТЬ 2: СТРУКТУРА ПРОЕКТА И CMakeLists.txt

### 2.1 Структура папок (лучшие практики)

```
my_project/
├── CMakeLists.txt          # Главный конфиг
├── .gitignore
├── README.md
├── src/
│   ├── CMakeLists.txt      # Конфиг для исходников
│   ├── main.cpp
│   └── mylib.cpp
├── include/
│   └── mylib.h
├── tests/
│   ├── CMakeLists.txt
│   └── test_main.cpp
├── cmake/                  # Модули CMake
│   └── FindCUDA.cmake
├── build/                  # Генерируется CMake (в .gitignore!)
│   ├── Debug/
│   └── Release/
└── docs/
```

### 2.2 Минимальный CMakeLists.txt (hello world)

```cmake
# Версия и название проекта
cmake_minimum_required(VERSION 3.20)
project(HelloCMake)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Создать исполняемый файл
add_executable(hello_app src/main.cpp)

# Подключить директорию include
target_include_directories(hello_app PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

**src/main.cpp:**
```cpp
#include <iostream>

int main() {
    std::cout << "Hello from CMake!\n";
    return 0;
}
```

**Сборка:**
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./hello_app  # Linux/macOS
# ИЛИ
hello_app.exe  # Windows
```

---

## ЧАСТЬ 3: ПРОДВИНУТАЯ КОНФИГУРАЦИЯ

### 3.1 Структура проекта с библиотеками

**CMakeLists.txt (главный):**
```cmake
cmake_minimum_required(VERSION 3.20)
project(GPUCorrelator 
    VERSION 1.0.0
    LANGUAGES CXX CUDA)

# Опции конфигурации
option(BUILD_TESTS "Build test suite" ON)
option(ENABLE_CUDA "Enable CUDA support" ON)
option(BUILD_SHARED_LIBS "Build as shared library" OFF)

# Стандарты
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги оптимизации
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2 /arch:AVX2)
    else()
        add_compile_options(-O3 -march=native)
    endif()
endif()

# Добавить подпроекты
add_subdirectory(src)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

**src/CMakeLists.txt:**
```cmake
# Создать статическую библиотеку
add_library(correlator_lib STATIC
    fft_engine.cpp
    signal_processing.cpp
)

# Флаги компиляции для этой библиотеки
target_compile_options(correlator_lib PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# Зависимости
target_include_directories(correlator_lib 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Если используется CUDA
if(ENABLE_CUDA)
    find_package(CUDA 11.8 REQUIRED)
    target_link_libraries(correlator_lib PRIVATE CUDA::cudart CUDA::cufft)
    set_target_properties(correlator_lib PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
endif()

# Исполняемый файл
add_executable(correlator_app main.cpp)
target_link_libraries(correlator_app PRIVATE correlator_lib)
```

### 3.2 CUDA интеграция (критично для ваших проектов)

```cmake
# Включить поддержку CUDA
enable_language(CUDA)

# Найти CUDA Toolkit
find_package(CUDA 11.8 REQUIRED)

# Создать CUDA библиотеку
add_library(cuda_fft STATIC
    kernels/fft_kernel.cu
    fft_wrapper.cpp
)

# Установить CUDA архитектуру (cc)
set_target_properties(cuda_fft PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "75;80;86;89"  # Ampere, Ada
)

# Компилятор и флаги для CUDA
target_compile_options(cuda_fft PRIVATE 
    $<$<COMPILE_LANGUAGE:CUDA>:
        --expt-relaxed-constexpr
        -lineinfo
    >
)

# Линковка CUDA библиотек
target_link_libraries(cuda_fft PUBLIC
    CUDA::cudart
    CUDA::cufft
    CUDA::cublas
)
```

### 3.3 Находение внешних зависимостей

```cmake
# Встроенные модули Find*
find_package(CUDA REQUIRED)
find_package(OpenCL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# Или через pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFT fftw3f REQUIRED IMPORTED_TARGET)

# Использование найденных библиотек
target_link_libraries(my_app PRIVATE
    CUDA::cudart
    OpenCL::OpenCL
    Boost::system
    PkgConfig::FFT
)
```

---

## ЧАСТЬ 4: VS CODE КОНФИГУРАЦИЯ

### 4.1 Установка расширений

**Необходимые расширения:**
1. **C/C++ Extension Pack** (Microsoft)
2. **CMake Tools** (Microsoft)
3. **CMake** (twxs)
4. **Clang-Format** (xaver)
5. **Better C++ Syntax** (Jeff Hykin)

Если используется CUDA:
6. **CUDA Toolkit Diagnostics** (jarvis)
7. **Shader languages support for VS Code** (slevesque)

### 4.2 .vscode/settings.json (Windows & Ubuntu)

```json
{
    "cmake.configureOnOpen": true,
    "cmake.configureOnEdit": false,
    "cmake.sourceDirectory": "${workspaceFolder}",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    
    "[cpp]": {
        "editor.defaultFormatter": "xaver.clang-format",
        "editor.formatOnSave": true,
        "editor.insertSpaces": true,
        "editor.tabSize": 4
    },
    
    "C_Cpp.default.cppStandard": "c++17",
    "C_Cpp.default.cMode": "c17",
    "C_Cpp.clang_format_style": "file",
    "C_Cpp.intelliSenseEngine": "tag Parser",
    
    // Platform-specific settings
    "cmake.generator": "Ninja",  // Linux/macOS: "Unix Makefiles" или "Ninja"
    // Windows: "Visual Studio 16 2019", "Ninja", или "NMake Makefiles"
}
```

### 4.3 .vscode/tasks.json (сборка и запуск)

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake: Configure",
            "type": "shell",
            "command": "cmake",
            "args": [
                "-B", "build",
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DENABLE_CUDA=ON"
            ],
            "group": {
                "kind": "build",
                "isDefault": false
            }
        },
        {
            "label": "CMake: Build Debug",
            "type": "shell",
            "command": "cmake",
            "args": ["--build", "build", "--config", "Debug", "-j4"],
            "dependsOn": "CMake: Configure",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        },
        {
            "label": "CMake: Build Release",
            "type": "shell",
            "command": "cmake",
            "args": ["--build", "build", "--config", "Release", "-j4"],
            "group": "build"
        },
        {
            "label": "Run Program",
            "type": "shell",
            "command": "${workspaceFolder}/build/correlator_app",
            "group": "test",
            "dependsOn": "CMake: Build Debug"
        }
    ]
}
```

### 4.4 .vscode/launch.json (отладка)

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug (Linux/macOS)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/correlator_app",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "CMake: Build Debug",
            "miDebuggerPath": "/usr/bin/gdb"
        },
        {
            "name": "Debug (Windows)",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}\\build\\Debug\\correlator_app.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "preLaunchTask": "CMake: Build Debug"
        }
    ]
}
```

---

## ЧАСТЬ 5: ПЛАТФОРМО-СПЕЦИФИЧНЫЕ КОНФИГУРАЦИИ

### 5.1 Windows (MSVC + Ninja/Visual Studio)

**CMakeLists.txt специфика:**
```cmake
if(WIN32)
    # MSVC специфичные опции
    if(MSVC)
        add_compile_options(/W4 /WX)  # Предупреждения как ошибки
        set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Od")
        set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Oi")
    endif()
    
    # Поиск CUDA на Windows
    if(ENABLE_CUDA)
        set(CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.8")
        find_package(CUDA 11.8 REQUIRED)
    endif()
endif()
```

**Запуск на Windows:**
```bash
# С Visual Studio
cmake -B build -G "Visual Studio 16 2019"
cmake --build build --config Release

# С Ninja (быстрее)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### 5.2 Ubuntu/Linux (GCC + Make/Ninja)

**CMakeLists.txt специфика:**
```cmake
if(UNIX AND NOT APPLE)
    # GCC/Clang опции
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # SIMD оптимизация
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=native -mtune=native)
    endif()
    
    # CUDA на Linux
    if(ENABLE_CUDA)
        find_package(CUDA 11.8 REQUIRED)
        # CUDA обычно в /usr/local/cuda
    endif()
endif()
```

**Запуск на Linux:**
```bash
# С Make
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j$(nproc)

# С Ninja (рекомендуется)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

---

## ЧАСТЬ 6: ПРАКТИЧЕСКИЕ ПРИМЕРЫ

### 6.1 Пример: FFT проект с CUDA

**Структура:**
```
fft_project/
├── CMakeLists.txt
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── fft_cuda.cu
│   └── signal_utils.cpp
├── include/
│   ├── fft_cuda.h
│   └── signal_utils.h
├── tests/
│   ├── CMakeLists.txt
│   └── test_fft.cpp
└── build/
```

**CMakeLists.txt (главный):**
```cmake
cmake_minimum_required(VERSION 3.20)
project(FFT_CUDA VERSION 1.0.0 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

# CUDA архитектуры (для ваших случаев)
set(CMAKE_CUDA_ARCHITECTURES 75 80 86 89)  # V100, A100, RTX 3080, H100

# Флаги компиляции
if(MSVC)
    add_compile_options(/O2 /arch:AVX2)
else()
    add_compile_options(-O3 -march=native -ffast-math)
endif()

# CUDA Toolkit
find_package(CUDA 11.8 REQUIRED)

add_subdirectory(src)
```

**src/CMakeLists.txt:**
```cmake
add_library(fft_cuda_lib STATIC
    fft_cuda.cu
    signal_utils.cpp
)

target_compile_options(fft_cuda_lib PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:
        --generate-code=arch=compute_86,code=sm_86
        -Xcompiler -fPIC
        --expt-relaxed-constexpr
    >
)

target_include_directories(fft_cuda_lib 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(fft_cuda_lib PUBLIC
    CUDA::cudart
    CUDA::cufft
)

# Исполняемый файл
add_executable(fft_correlator main.cpp)
target_link_libraries(fft_correlator PRIVATE fft_cuda_lib)
```

### 6.2 Интеграция VkFFT (если используете Vulkan)

```cmake
# Скачать VkFFT
include(FetchContent)
FetchContent_Declare(VkFFT
    GIT_REPOSITORY https://github.com/DTolm/VkFFT.git
    GIT_TAG master
)
FetchContent_MakeAvailable(VkFFT)

# Использование
target_include_directories(my_app PRIVATE ${vkfft_SOURCE_DIR})
```

---

## ЧАСТЬ 7: РАСШИРЕННЫЕ ТЕХНИКИ

### 7.1 Условная компиляция

```cmake
# Опции для пользователя
option(USE_OPENMP "Enable OpenMP" ON)
option(USE_VULKAN "Enable Vulkan" OFF)
option(DEBUG_MODE "Enable debug output" OFF)

# Условная линковка
if(USE_OPENMP)
    find_package(OpenMP REQUIRED)
    target_link_libraries(my_app PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(my_app PRIVATE USE_OPENMP)
endif()

if(USE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(my_app PRIVATE Vulkan::Vulkan)
endif()

if(DEBUG_MODE)
    target_compile_definitions(my_app PRIVATE DEBUG_OUTPUT=1)
endif()
```

**Использование при запуске:**
```bash
cmake -B build \
    -DUSE_OPENMP=ON \
    -DUSE_VULKAN=OFF \
    -DDEBUG_MODE=ON
```

### 7.2 Автоматическое тестирование

**tests/CMakeLists.txt:**
```cmake
enable_testing()

add_executable(test_fft test_fft.cpp)
target_link_libraries(test_fft PRIVATE fft_cuda_lib)

add_test(NAME FFT_BasicTest COMMAND test_fft)
add_test(NAME FFT_LargeInput COMMAND test_fft --size=1048576)
```

**Запуск тестов:**
```bash
ctest --build-config Release --verbose
```

### 7.3 Установка и экспорт

```cmake
# CMakeLists.txt в корне

# Установка библиотеки и заголовков
install(TARGETS correlator_lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include/correlator
    FILES_MATCHING PATTERN "*.h"
)

# CMake конфиг для других проектов
install(FILES correlatorConfig.cmake
    DESTINATION lib/cmake/correlator
)
```

**Сборка и установка:**
```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

---

## ЧАСТЬ 8: ОТЛАДКА И ОПТИМИЗАЦИЯ

### 8.1 Debug информация

```cmake
# Для GDB/LLDB
if(NOT MSVC)
    add_compile_options(-g -ggdb3)
endif()

# Для MSVC
if(MSVC)
    add_compile_options(/Zi)
    link_options(/DEBUG)
endif()
```

### 8.2 Проверка конфигурации

```cmake
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CUDA available: ${CUDA_FOUND}")
if(CUDA_FOUND)
    message(STATUS "CUDA version: ${CUDA_VERSION}")
    message(STATUS "CUDA Toolkit: ${CUDA_TOOLKIT_ROOT_DIR}")
endif()
```

**Запуск с выводом:**
```bash
cmake -B build --debug-output
```

---

## ЧАСТЬ 9: ШПАРГАЛКА КОМАНД

```bash
# Конфигурация
cmake -B build                              # Debug на Linux/macOS
cmake -B build -G Ninja                     # С Ninja
cmake -B build -G "Visual Studio 16 2019"   # Windows с MSVC
cmake -B build -DCMAKE_BUILD_TYPE=Release   # Release сборка

# Сборка
cmake --build build                         # Default
cmake --build build --config Release        # Release явно
cmake --build build -j$(nproc)              # С параллелизмом
cmake --build build --target clean          # Очистка

# Переконфигурация
cmake --reconfigure -B build
rm -rf build && cmake -B build              # Полный пересброс

# Установка
cmake --install build --prefix /usr/local
cmake --install build --config Release

# Тестирование
ctest --build-config Release
ctest --output-on-failure
ctest -N                                     # Список тестов

# Отладка
cmake -B build --debug-output               # Подробный вывод
cmake -B build --trace                      # Трассировка
cmake -B build --trace-expand               # Полная трассировка
```

---

## ЧАСТЬ 10: ТИПИЧНЫЕ ОШИБКИ И РЕШЕНИЯ

### Ошибка: "CUDA not found"
```cmake
# Решение 1: Явно указать путь
set(CUDA_TOOLKIT_ROOT_DIR "/usr/local/cuda")

# Решение 2: Через переменную окружения
# На Linux: export CUDA_HOME=/usr/local/cuda
# На Windows: установить через installer
```

### Ошибка: "Compiler not found"
```cmake
# Явно указать компилятор
cmake -B build -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc
# Или
cmake -B build -DCMAKE_CXX_COMPILER=clang++
```

### Ошибка: "Architecture mismatch"
```cmake
# Указать архитектуру явно
cmake -B build -DCMAKE_SYSTEM_PROCESSOR=x86_64
# Или для cross-compilation
cmake -B build -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=armv7l
```

---

## ЗАКЛЮЧЕНИЕ

**Ключевые моменты для эффективной работы:**

1. **Модульность** — разбивайте большие проекты на подпроекты через add_subdirectory
2. **Переносимость** — используйте условия для Windows/Linux вместо hardcoded путей
3. **Производительность** — для GPU проектов правильно настраивайте CUDA_ARCHITECTURES
4. **VS Code интеграция** — используйте CMake Tools для удобства
5. **Документирование** — добавляйте message() для отладки конфигурации

**Следующие шаги:**
- Создайте простой проект hello_cmake
- Мигрируйте существующий Makefile в CMake
- Интегрируйте CUDA в ваш FFT проект
- Настройте CI/CD через GitHub Actions с CMake
