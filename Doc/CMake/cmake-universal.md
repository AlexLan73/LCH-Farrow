# CMakeLists.txt для Windows + Ubuntu (УНИВЕРСАЛЬНЫЙ)
## Ваш файл + улучшения для кроссплатформности

```cmake
cmake_minimum_required(VERSION 3.20)
project(LCH-Farrow VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# ЧАСТЬ 1: ОПРЕДЕЛЕНИЕ ОПЕРАЦИОННОЙ СИСТЕМЫ
# ============================================================================

if(WIN32)
    set(IS_WINDOWS TRUE)
    set(IS_LINUX FALSE)
    set(PLATFORM_NAME "Windows")
    message(STATUS "🪟 Операционная система: WINDOWS")
    
elseif(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
    set(IS_WINDOWS FALSE)
    set(PLATFORM_NAME "Linux")
    message(STATUS "🐧 Операционная система: LINUX")
    
elseif(APPLE)
    set(IS_LINUX TRUE)
    set(IS_WINDOWS FALSE)
    set(PLATFORM_NAME "macOS")
    message(STATUS "🍎 Операционная система: macOS")
    
else()
    message(FATAL_ERROR "❌ Неизвестная операционная система!")
endif()

# ============================================================================
# ЧАСТЬ 2: ВЕРСИЯ И СТАНДАРТЫ C++
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ Standard: C++17")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# ============================================================================
# ЧАСТЬ 3: ПАРАМЕТРЫ СБОРКИ
# ============================================================================

# По умолчанию Release если не указано
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# ============================================================================
# ЧАСТЬ 4: КОМПИЛЯТОР И ФЛАГИ ОПТИМИЗАЦИИ
# ============================================================================

if(MSVC)
    # ===== WINDOWS (MSVC компилятор) =====
    message(STATUS "🔧 Конфигурация: MSVC (Visual Studio)")
    
    # Флаги для Debug
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Od /RTC1")
    
    # Флаги для Release с оптимизацией
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Oi /arch:AVX2 /DNDEBUG")
    
    # Общие флаги
    add_compile_options(
        /W4           # Максимум предупреждений
        /EHsc         # Exception handling
        /permissive-  # Строгий C++ стандарт
        /Zc:inline    # Оптимизация
    )
    
    # Отключить непредсказуемые предупреждения
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
    )
    
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
    message(STATUS "Optimization: /O2 /arch:AVX2")
    
else()
    # ===== LINUX / WSL / macOS (GCC / Clang) =====
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(STATUS "🔧 Конфигурация: GCC")
        set(COMPILER_NAME "GCC")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "🔧 Конфигурация: Clang")
        set(COMPILER_NAME "Clang")
    else()
        message(STATUS "🔧 Конфигурация: ${CMAKE_CXX_COMPILER_ID}")
        set(COMPILER_NAME "${CMAKE_CXX_COMPILER_ID}")
    endif()
    
    # Флаги для Debug
    set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb3 -O0 -Wall -Wextra -Wpedantic")
    
    # Флаги для Release
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -Wall")
    
    # Общие флаги
    add_compile_options(
        -Wall           # Все предупреждения
        -Wextra         # Дополнительные предупреждения
        -Wpedantic      # Педантичные проверки
        -ffast-math     # Быстрая математика (для FFT)
        -funroll-loops  # Развернуть циклы
    )
    
    message(STATUS "Compiler: ${COMPILER_NAME}")
    message(STATUS "Optimization: -O3 -march=native")
    
endif()

# ============================================================================
# ЧАСТЬ 5: СТРУКТУРА ПРОЕКТА
# ============================================================================

message(STATUS "")
message(STATUS "📁 Структура проекта:")
message(STATUS "   Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "   Build dir: ${CMAKE_BINARY_DIR}")
message(STATUS "")

# Создать списки файлов
set(SOURCES
    src/main.cpp
    src/mylib.cpp
)

set(HEADERS
    include/mylib.h
)

# ============================================================================
# ЧАСТЬ 6: СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА
# ============================================================================

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Подключить include директорию
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# ЧАСТЬ 7: ЛИНКОВКА И ЗАВИСИМОСТИ
# ============================================================================

# Пример: линковка math библиотеки (есть везде в Linux)
if(NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

# ============================================================================
# ЧАСТЬ 8: ОПЦИИ КОМПИЛЯЦИИ ДЛЯ ЦЕЛИ
# ============================================================================

if(MSVC)
    # Windows специфичные опции
    target_compile_options(${PROJECT_NAME} 
        PRIVATE 
        /arch:AVX2          # AVX2 инструкции
        /Gy                 # Function-level linking
        /fp:precise         # Точная арифметика
    )
else()
    # Linux специфичные опции
    target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
endif()

# ============================================================================
# ЧАСТЬ 9: ВЫВОД ИНФОРМАЦИИ О СБОРКЕ
# ============================================================================

message(STATUS "")
message(STATUS "📊 Информация о сборке:")
message(STATUS "   Platform: ${PLATFORM_NAME}")
message(STATUS "   Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "   Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "   Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "   C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "   Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# ============================================================================
# ЧАСТЬ 10: ИНСТРУКЦИИ ДЛЯ ПОЛЬЗОВАТЕЛЯ
# ============================================================================

if(IS_WINDOWS)
    message(STATUS "✨ Команды для сборки на Windows:")
    message(STATUS "")
    message(STATUS "   cmake -B build -G \"Visual Studio 17 2022\"")
    message(STATUS "   cmake --build build --config Release")
    message(STATUS "   .\\build\\Release\\${PROJECT_NAME}.exe")
    message(STATUS "")
else()
    message(STATUS "✨ Команды для сборки на Linux/macOS:")
    message(STATUS "")
    message(STATUS "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release")
    message(STATUS "   ninja -C build")
    message(STATUS "   ./build/${PROJECT_NAME}")
    message(STATUS "")
    message(STATUS "   или с Make:")
    message(STATUS "   cmake -B build -DCMAKE_BUILD_TYPE=Release")
    message(STATUS "   make -C build -j\$(nproc)")
    message(STATUS "   ./build/${PROJECT_NAME}")
    message(STATUS "")
endif()

# ============================================================================
# ЧАСТЬ 11: ОПЦИИ ПРОЕКТА (опционально)
# ============================================================================

option(ENABLE_TESTING "Enable testing" OFF)
option(VERBOSE "Enable verbose output" OFF)

if(VERBOSE)
    message(STATUS "Verbose mode enabled")
endif()

if(ENABLE_TESTING)
    enable_testing()
    add_executable(test_app tests/test_main.cpp src/mylib.cpp)
    target_include_directories(test_app PRIVATE ${CMAKE_SOURCE_DIR}/include)
    if(NOT MSVC)
        target_link_libraries(test_app PRIVATE m)
    endif()
    add_test(NAME BasicTests COMMAND test_app)
    message(STATUS "Testing enabled")
endif()

# ============================================================================
# КОНЕЦ КОНФИГУРАЦИИ
# ============================================================================

message(STATUS "✅ CMake конфигурация успешна!")
```

---

## ✅ ЧТО ИЗМЕНИЛИ:

### ❌ БЫЛО (старое):
```cmake
find_program(NINJA_EXECUTABLE ninja)
if(NINJA_EXECUTABLE)
    message(STATUS "✅ Ninja найдена: ${NINJA_EXECUTABLE}")
```

### ✅ СТАЛО (новое):
```cmake
# Никакого поиска Ninja!
# Просто указываем генератор явно при запуске cmake:
# Windows:  cmake -B build -G "Visual Studio 17 2022"
# Linux:    cmake -B build -G Ninja
```

---

## 🎯 ТЕПЕРЬ ДЛЯ Ubuntu:

```bash
# 1. Установить зависимости (если первый раз)
sudo apt-get update
sudo apt-get install -y cmake ninja-build build-essential

# 2. Клонировать репозиторий (если не клонировали)
cd ~
git clone git@github.com:AlexLan73/LCH-Farrow.git
cd LCH-Farrow

# 3. Собрать с Ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# 4. Запустить
./build/LCH-Farrow

# ГОТОВО! ✅
```

---

## 🐧 ИЛИ БЕЗ Ninja (Make):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j$(nproc)
./build/LCH-Farrow
```

---

## 📝 СКОПИРУЙТЕ НОВЫЙ CMakeLists.txt:

Замените в вашем репозитории весь CMakeLists.txt на код выше!

**Потом:**
```bash
git add CMakeLists.txt
git commit -m "Improve CMakeLists.txt for Windows and Linux"
git push origin main
```

---

## ✅ РЕЗУЛЬТАТ:

Один CMakeLists.txt работает на:
- ✅ Windows (Visual Studio)
- ✅ Ubuntu/Linux (Ninja или Make)
- ✅ macOS (если у вас есть)

**Красота кроссплатформности!** 🚀
