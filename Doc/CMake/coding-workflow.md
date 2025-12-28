# Написание кода и сборка с CMake
## Практический справочник для alex@WIN-V41QB4VHFO9

---

## БЫСТРЫЙ СТАРТ: ТИПИЧНЫЙ WORKFLOW

### Ежедневная работа:

```bash
# 1. Перейти в проект
cd ~/C++/LCH-Farrow

# 2. Проверить статус (есть ли изменения с сервера)
git status
git pull  # Если нужны новые изменения

# 3. Написать или отредактировать код
# Используйте VS Code или другой редактор

# 4. Сохранить файл (VS Code делает автоматически)

# 5. Собрать проект
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# 6. Запустить и протестировать
./build/test_app

# 7. Если работает - сохранить в Git
git add .
git commit -m "Описание изменений"
git push

# 8. Повторить с пункта 2
```

---

## ЧАСТЬ 1: СОЗДАНИЕ И РЕДАКТИРОВАНИЕ КОДА

### 1.1 Структура файлов (как организовать)

```
~/C++/LCH-Farrow/
├── CMakeLists.txt           # Конфигурация сборки
├── .gitignore               # Что не загружать на GitHub
├── README.md                # Описание проекта
├── src/
│   ├── main.cpp             # Главный файл программы
│   ├── mylib.cpp            # Реализация библиотеки
│   └── utils.cpp            # Вспомогательные функции
├── include/
│   ├── mylib.h              # Заголовок библиотеки
│   └── utils.h              # Заголовок утилит
├── tests/                   # (опционально) Тесты
│   └── test_main.cpp
└── build/                   # Генерируется CMake (не в Git!)
    ├── CMakeFiles/
    ├── Makefile/ninja files
    └── test_app             # Скомпилированный файл
```

### 1.2 Пример простого проекта

**src/main.cpp:**
```cpp
#include <iostream>
#include "mylib.h"

int main() {
    std::cout << "Hello from CMake!\n";
    std::cout << "Result: " << add(5, 3) << "\n";
    return 0;
}
```

**include/mylib.h:**
```cpp
#ifndef MYLIB_H
#define MYLIB_H

int add(int a, int b);
int multiply(int a, int b);

#endif // MYLIB_H
```

**src/mylib.cpp:**
```cpp
#include "mylib.h"

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}
```

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(LCH-Farrow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги оптимизации для вашего GCC 13.3.0
if(NOT MSVC)
    add_compile_options(-O3 -march=native)
endif()

# Создать исполняемый файл
add_executable(test_app 
    src/main.cpp
    src/mylib.cpp
)

# Подключить include директорию
target_include_directories(test_app PRIVATE ${CMAKE_SOURCE_DIR}/include)
```

---

## ЧАСТЬ 2: СБОРКА ПРОЕКТА

### 2.1 Первая сборка (конфигурация + сборка)

```bash
# Находитесь в ~/C++/LCH-Farrow

# Шаг 1: Конфигурация (создает папку build и подготавливает)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Шаг 2: Сборка (компилирует)
ninja -C build

# Или одной командой:
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && ninja -C build

# Проверить что создался исполняемый файл
ls -la build/test_app
```

### 2.2 Последующие сборки (быстрее)

```bash
# Если вы изменили только .cpp файлы, а не CMakeLists.txt:
ninja -C build

# Если изменили CMakeLists.txt:
cmake -B build  # Переконфигурировать
ninja -C build  # Собрать

# Полная пересборка (очистка + сборка)
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### 2.3 Debug vs Release сборка

```bash
# Debug (для разработки и отладки)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
ninja -C build

# Release (для оптимизации и финального кода)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
ninja -C build

# Проверить что сборка делается
cmake --build build --verbose  # Показывает команды компиляции
```

---

## ЧАСТЬ 3: ЗАПУСК И ТЕСТИРОВАНИЕ

### 3.1 Запуск программы

```bash
# Запустить исполняемый файл
./build/test_app

# С аргументами (если ваша программа их поддерживает)
./build/test_app arg1 arg2

# Запустить и перенаправить вывод в файл
./build/test_app > output.txt

# Запустить и видеть время выполнения
time ./build/test_app
```

### 3.2 Отладка с помощью gdb

```bash
# Сборка в debug режиме
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
ninja -C build

# Запустить в отладчике
gdb ./build/test_app

# Команды в gdb:
# (gdb) run                    # Запустить программу
# (gdb) break main             # Поставить точку останова в main
# (gdb) step                   # Выполнить одну строку (входит в функции)
# (gdb) next                   # Выполнить одну строку (не входит в функции)
# (gdb) continue               # Продолжить выполнение
# (gdb) print variable         # Вывести значение переменной
# (gdb) list                   # Показать код
# (gdb) quit                   # Выход
```

### 3.3 Профилирование (для оптимизации)

```bash
# Запустить с профилированием
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
ninja -C build

# Профилировать с помощью perf (Linux)
perf record ./build/test_app
perf report

# Или просто время выполнения
time ./build/test_app
time ninja -C build  # Время сборки
```

---

## ЧАСТЬ 4: БЫСТРЫЙ ЦИКЛ РАЗРАБОТКИ

### В VS Code интегрированно:

```bash
# Терминал в VS Code: Ctrl+`

# 1. Один раз (конфигурация)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 2. Постоянно (fast loop)
ninja -C build && ./build/test_app

# 3. С отладкой (Debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
ninja -C build
gdb ./build/test_app
```

### Или используйте Task в VS Code:

**.vscode/tasks.json (уже дается в cmake-masterclass.md):**

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build & Run",
            "type": "shell",
            "command": "bash",
            "args": ["-c", "cmake -B build -G Ninja && ninja -C build && ./build/test_app"],
            "group": { "kind": "build", "isDefault": true },
            "problemMatcher": []
        }
    ]
}
```

**Использование:** Ctrl+Shift+B (или через Command Palette)

---

## ЧАСТЬ 5: РЕШЕНИЕ ПРОБЛЕМ ПРИ СБОРКЕ

### Проблема 1: "No such file or directory"

```bash
# Ошибка: fatal error: mylib.h: No such file or directory
# Причина: Неправильно указана директория include

# Решение в CMakeLists.txt:
target_include_directories(test_app PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Или абсолютный путь:
target_include_directories(test_app PRIVATE /home/alex/C++/LCH-Farrow/include)
```

### Проблема 2: "undefined reference"

```bash
# Ошибка: undefined reference to `add'
# Причина: Забыли добавить .cpp файл в CMakeLists.txt

# Решение:
add_executable(test_app 
    src/main.cpp
    src/mylib.cpp  # ← Должен быть тут!
)
```

### Проблема 3: Стара версия скомпилированного файла

```bash
# Ошибка: Изменили код, но старая версия запускается
# Причина: Не пересобрали

# Решение: Полная пересборка
rm -rf build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/test_app
```

### Проблема 4: Конфликты из Git

```bash
# Ошибка: CONFLICT при pull
# Решение: Смотрите git-pull-guide.md

git status                    # Увидите конфликтующие файлы
# Отредактируйте файлы вручную
git add .
git commit -m "Resolve conflicts"
```

---

## ЧАСТЬ 6: УЛУЧШЕНИЯ ДЛЯ ВАШЕГО ПРОЕКТА

### 6.1 Добавить больше файлов источников

**CMakeLists.txt:**
```cmake
# Вместо перечисления каждого файла:
add_executable(test_app 
    src/main.cpp
    src/mylib.cpp
    src/utils.cpp
    src/other.cpp
)

# Можно использовать glob (автоматически):
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(test_app ${SOURCES})
```

### 6.2 Добавить предупреждения при компиляции

```cmake
# Включить все предупреждения
if(NOT MSVC)
    target_compile_options(test_app PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Обработать все как ошибки
target_compile_options(test_app PRIVATE -Werror)
```

### 6.3 Добавить тесты (CTest)

**tests/test_main.cpp:**
```cpp
#include <iostream>
#include "mylib.h"

int main() {
    // Простые проверки
    if (add(2, 3) != 5) {
        std::cerr << "Test FAILED: add(2, 3) != 5\n";
        return 1;
    }
    
    if (multiply(3, 4) != 12) {
        std::cerr << "Test FAILED: multiply(3, 4) != 12\n";
        return 1;
    }
    
    std::cout << "All tests PASSED!\n";
    return 0;
}
```

**CMakeLists.txt (добавить):**
```cmake
# Включить тестирование
enable_testing()

# Создать исполняемый файл для тестов
add_executable(test_runner tests/test_main.cpp src/mylib.cpp)
target_include_directories(test_runner PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Добавить тест
add_test(NAME BasicTests COMMAND test_runner)
```

**Запуск тестов:**
```bash
cmake -B build -G Ninja
ninja -C build
ctest --build-config Release --verbose
```

---

## ЧАСТЬ 7: РАБОТА С VS CODE

### 7.1 Отладка в VS Code

**.vscode/launch.json:**
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug C++",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/test_app",
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
            "preLaunchTask": "Build & Run",
            "miDebuggerPath": "/usr/bin/gdb"
        }
    ]
}
```

**Использование:**
- F5 — запустить отладку
- F10 — следующая строка
- F11 — войти в функцию
- Shift+F11 — выход из функции
- Ctrl+Shift+D — открыть Debug панель

### 7.2 Интеллектуальное дополнение кода (IntelliSense)

```json
// .vscode/c_cpp_properties.json
{
    "configurations": [
        {
            "name": "Linux/WSL",
            "includePath": [
                "${workspaceFolder}/include",
                "${workspaceFolder}/src",
                "/usr/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseEngine": "default"
        }
    ],
    "version": 4
}
```

---

## ЧАСТЬ 8: ШПАРГАЛКА ДЛЯ БЫСТРОГО СТАРТА

```bash
# === НОВЫЙ ПРОЕКТ ===
mkdir -p ~/C++/MyProject/src ~/C++/MyProject/include
cd ~/C++/MyProject

# === СБОРКА ===
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# === ЗАПУСК ===
./build/my_app

# === ОТЛАДКА ===
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
ninja -C build
gdb ./build/my_app

# === GIT ===
git add .
git commit -m "Add functionality"
git push

# === БЫСТРЫЙ ЦИКЛ ===
# 1. Редактируем код (VS Code)
# 2. ninja -C build && ./build/test_app
# 3. Если работает - git push

# === ПОЛНАЯ ПЕРЕСБОРКА ===
rm -rf build && cmake -B build -G Ninja && ninja -C build && ./build/test_app

# === ТЕСТИРОВАНИЕ ===
ctest --build-config Release --verbose
```

---

## ДЛЯ ВАШЕГО КОНКРЕТНОГО СЛУЧАЯ

**Вы сейчас:**
- ✅ Установили CMake, GCC, Ninja, Git
- ✅ Инициализировали проект в Git
- ✅ Готовы писать код

**Следующие шаги:**

```bash
# 1. Перейти в проект
cd ~/C++/LCH-Farrow

# 2. Создать структуру (если еще нет)
mkdir -p src include

# 3. Написать простой код (пример выше)

# 4. Собрать
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# 5. Запустить
./build/test_app

# 6. Если работает
git add .
git commit -m "Initial project structure"
git push

# 7. Продолжить разработку
# Просто повторяйте шаги: редактируй → собирай → тестируй → push
```

---

## РЕКОМЕНДАЦИИ ДЛЯ ОБУЧЕНИЯ

### Когда вы готовы к следующему уровню:

1. **Добавить библиотеки** (как в cmake-masterclass.md)
   - Статические библиотеки (.a)
   - Динамические библиотеки (.so)

2. **Использовать внешние библиотеки** (Boost, OpenSSL и т.д.)
   - find_package()
   - target_link_libraries()

3. **CUDA интеграция** (для GPU вычислений)
   - enable_language(CUDA)
   - Компиляция .cu файлов

4. **Сложные проекты**
   - add_subdirectory() для разных модулей
   - Кроссплатформенная сборка

5. **CI/CD** (автоматизация)
   - GitHub Actions
   - Автоматическая сборка при push

---

## ИТОГ

**Вы готовы:**
- ✅ Писать C++ код
- ✅ Собирать его с CMake
- ✅ Использовать Git
- ✅ Отлаживать в VS Code
- ✅ Оптимизировать сборку с Ninja

**Главное правило:** Быстрый цикл разработки:
```bash
Редактируй код → Собери → Запусти → Протестируй → Загрузи в Git
```

Успехов в разработке! 🚀💪

---

## ВОПРОСЫ?

Если что-то не работает:
1. Проверьте `git status`
2. Посмотрите `cmake --build build --verbose`
3. Используйте `gdb` для отладки
4. Смотрите соответствующий файл справочника

Happy coding! 🎉
