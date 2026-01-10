# Кроссплатформенный CMake для Windows и Linux
## Автоматическое определение ОС и настройка сборки

---

## ПОЛНЫЙ CMakeLists.txt (COPY-PASTE)

```cmake
cmake_minimum_required(VERSION 3.20)
project(LCH-Farrow VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# ЧАСТЬ 1: ОПРЕДЕЛЕНИЕ ОПЕРАЦИОННОЙ СИСТЕМЫ
# ============================================================================

# Переменные для ОС
if(WIN32)
    set(IS_WINDOWS TRUE)
    set(IS_LINUX FALSE)
    set(PLATFORM_NAME "Windows")
    message(STATUS "🪟 Операционная система: WINDOWS")
    
elseif(UNIX AND NOT APPLE)
    set(IS_LINUX TRUE)
    set(IS_WINDOWS FALSE)
    set(PLATFORM_NAME "Linux/WSL")
    message(STATUS "🐧 Операционная система: LINUX/WSL")
    
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
# ЧАСТЬ 4: ВЫБОР BUILD SYSTEM (Ninja или Make)
# ============================================================================

# Определить доступные build системы
if(IS_WINDOWS)
    # На Windows пытаемся найти Ninja через WSL или MSVC
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "✅ Ninja найдена: ${NINJA_EXECUTABLE}")
    else()
        message(STATUS "⚠️  Ninja не найдена - используется MSVC")
    endif()
    
else()
    # На Linux ищем Ninja
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "✅ Ninja найдена: ${NINJA_EXECUTABLE}")
    else()
        message(STATUS "⚠️  Ninja не найдена - используется Make")
    endif()
endif()

# ============================================================================
# ЧАСТЬ 5: КОМПИЛЯТОР И ФЛАГИ ОПТИМИЗАЦИИ
# ============================================================================

if(MSVC)
    # ===== WINDOWS (MSVC компилятор) =====
    message(STATUS "🔧 Конфигурация: MSVC (Visual Studio)")
    
    # Флаги для Debug
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Od /RTC1")
    
    # Флаги для Release
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Oi /arch:AVX2")
    
    # Общие флаги
    add_compile_options(
        /W4           # Все предупреждения
        /EHsc         # Exception handling
        /std:c++17    # C++17 standard
    )
    
    message(STATUS "Compiler: MSVC (Visual Studio)")
    message(STATUS "Optimization: AVX2 enabled")
    
else()
    # ===== LINUX / WSL (GCC / Clang) =====
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
# ЧАСТЬ 6: СТРУКТУРА ПРОЕКТА
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
# ЧАСТЬ 7: СОЗДАНИЕ ИСПОЛНЯЕМОГО ФАЙЛА
# ============================================================================

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Подключить include директорию
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# ЧАСТЬ 8: ЛИНКОВКА И ЗАВИСИМОСТИ
# ============================================================================

# Пример: linkovka math библиотеки (есть везде)
if(NOT MSVC)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

# ============================================================================
# ЧАСТЬ 9: ОПЦИИ КОМПИЛЯЦИИ ДЛЯ ЦЕЛИ
# ============================================================================

if(IS_WINDOWS)
    # Windows специфичные опции
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)
    endif()
else()
    # Linux специфичные опции
    target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
endif()

# ============================================================================
# ЧАСТЬ 10: ВЫВОД ИНФОРМАЦИИ О СБОРКЕ
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
# ЧАСТЬ 11: ИНСТРУКЦИИ ДЛЯ ПОЛЬЗОВАТЕЛЯ
# ============================================================================

message(STATUS "✨ Команды для сборки:")
if(NINJA_EXECUTABLE)
    message(STATUS "   cmake -B build -G Ninja")
    message(STATUS "   ninja -C build")
else()
    if(IS_WINDOWS)
        message(STATUS "   cmake -B build -G \"Visual Studio 16 2019\"")
        message(STATUS "   cmake --build build --config Release")
    else()
        message(STATUS "   cmake -B build -G Unix\\ Makefiles")
        message(STATUS "   make -C build -j\$(nproc)")
    endif()
endif()
message(STATUS "")

# ============================================================================
# ЧАСТЬ 12: ОПЦИИ ПРОЕКТА (опционально)
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
    add_test(NAME BasicTests COMMAND test_app)
    message(STATUS "Testing enabled")
endif()

# ============================================================================
# КОНЕЦ КОНФИГУРАЦИИ
# ============================================================================

message(STATUS "✅ CMake конфигурация успешна!")
```

---

## КАК ЭТО ИСПОЛЬЗОВАТЬ

### На Windows (PowerShell или WSL)

```bash
# Способ 1: На Windows PowerShell с MSVC
cmake -B build -G "Visual Studio 16 2019" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Способ 2: На Windows PowerShell с WSL + Ninja
wsl
cd /mnt/e/C++/LCH-Farrow
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### На Linux / WSL Ubuntu

```bash
# С Ninja (если установлена)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# С Make (если нет Ninja)
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
make -C build -j$(nproc)
```

---

## ЧТО ВЫВЕДЕТ ЭТОТ CMakeLists.txt

```
🐧 Операционная система: LINUX/WSL
C++ Standard: C++17
Compiler: /usr/bin/c++
Build Type: Release

🔧 Конфигурация: GCC
✅ Ninja найдена: /usr/bin/ninja
Compiler: GCC
Optimization: -O3 -march=native

📁 Структура проекта:
   Source dir: /home/alex/C++/LCH-Farrow
   Build dir: /home/alex/C++/LCH-Farrow/build

📊 Информация о сборке:
   Platform: Linux/WSL
   Compiler: /usr/bin/c++
   Compiler ID: GNU
   Compiler Version: 13.3.0
   C++ Standard: 17
   Build Type: Release

✨ Команды для сборки:
   cmake -B build -G Ninja
   ninja -C build

✅ CMake конфигурация успешна!
```

---

## ОСОБЕННОСТИ ЭТОГО CMAKEFILE

### 1. Автоматическое определение ОС

```cmake
if(WIN32)
    # Windows
elseif(UNIX AND NOT APPLE)
    # Linux
elseif(APPLE)
    # macOS
endif()
```

### 2. Выбор компилятора

```cmake
if(MSVC)
    # Visual Studio
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # GCC
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Clang
    endif()
endif()
```

### 3. Оптимизация под каждую платформу

```cmake
# Windows
target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)

# Linux
target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
```

### 4. Правильные флаги для Debug и Release

```cmake
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")
```

### 5. Поиск Ninja и вывод инструкций

```cmake
find_program(NINJA_EXECUTABLE ninja)
if(NINJA_EXECUTABLE)
    message(STATUS "✅ Ninja найдена")
else()
    message(STATUS "⚠️  Ninja не найдена")
endif()
```

---

## РАСШИРЕНИЯ

### Добавить CUDA поддержку

```cmake
# После "project(LCH-Farrow..."

option(ENABLE_CUDA "Enable CUDA support" OFF)

if(ENABLE_CUDA)
    enable_language(CUDA)
    find_package(CUDA 11.8 REQUIRED)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_ARCHITECTURES 86)
    message(STATUS "✅ CUDA enabled")
endif()
```

**Использование:**
```bash
cmake -B build -DENABLE_CUDA=ON
```

### Добавить OpenMP

```cmake
# После "add_executable..."

find_package(OpenMP REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
```

### Добавить тестирование

```cmake
option(ENABLE_TESTING "Enable testing" ON)

if(ENABLE_TESTING)
    enable_testing()
    add_executable(test_app tests/test_main.cpp src/mylib.cpp)
    target_include_directories(test_app PRIVATE ${CMAKE_SOURCE_DIR}/include)
    add_test(NAME BasicTests COMMAND test_app)
endif()
```

**Запуск тестов:**
```bash
ctest --verbose
```

---

## ИСПОЛЬЗОВАНИЕ ДЛЯ ВАШЕГО ПРОЕКТА

### Шаг 1: Скопируйте CMakeLists.txt

Замените содержимое вашего `~/C++/LCH-Farrow/CMakeLists.txt` на код выше

### Шаг 2: Создайте структуру проекта (если еще нет)

```bash
cd ~/C++/LCH-Farrow

mkdir -p src include

# Создайте файлы
echo '#include <iostream>' > src/main.cpp
echo '#include "mylib.h"' >> src/main.cpp
echo 'int main() { std::cout << add(5, 3); return 0; }' >> src/main.cpp

echo '#ifndef MYLIB_H' > include/mylib.h
echo '#define MYLIB_H' >> include/mylib.h
echo 'int add(int a, int b);' >> include/mylib.h
echo '#endif' >> include/mylib.h

echo '#include "mylib.h"' > src/mylib.cpp
echo 'int add(int a, int b) { return a + b; }' >> src/mylib.cpp
```

### Шаг 3: Собрите

**На Linux/WSL:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/LCH-Farrow
```

**На Windows (WSL):**
```bash
wsl
cd ~/C++/LCH-Farrow
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/LCH-Farrow
```

**На Windows (MSVC):**
```powershell
cmake -B build -G "Visual Studio 16 2019"
cmake --build build --config Release
.\build\Release\LCH-Farrow.exe
```

---

## ОТЛАДКА CMAKE КОНФИГУРАЦИИ

Если что-то не работает:

```bash
# Полный debug вывод
cmake -B build --debug-output

# Трассировка всех команд
cmake -B build --trace

# Вывести все переменные
cmake -B build -DVERBOSE=ON
```

---

## ИТОГ

Этот `CMakeLists.txt`:
- ✅ **Автоматически определяет ОС** (Windows, Linux, macOS)
- ✅ **Выбирает правильный компилятор** (MSVC, GCC, Clang)
- ✅ **Применяет оптимизации** под платформу
- ✅ **Ищет Ninja и предлагает альтернативы**
- ✅ **Выводит подробную информацию** о сборке
- ✅ **Работает везде** (Windows, Linux, WSL, macOS)

Скопируйте этот файл и используйте! 🚀
