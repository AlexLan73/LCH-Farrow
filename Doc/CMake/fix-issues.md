# Решение проблем: CUDA, CMake и структура проекта
## Для alex@WIN-V41QB4VHFO9

---

## ПРОБЛЕМА 1: CUDA пакет не найден

### Ошибка:
```
E: Unable to locate package cuda-toolkit-12-4
```

### Причина:
CUDA пакеты не в стандартном Ubuntu репозитории. Нужно добавить NVIDIA репозиторий.

### Решение:

**Способ 1: Через NVIDIA репозиторий (РЕКОМЕНДУЕТСЯ)**

```bash

# Ubuntu уже имеет CUDA в стандартном репозитории!
sudo apt install -y nvidia-cuda-toolkit

# Проверка
nvcc --version

# !!!  не заработало 
# Шаг 1: Добавить NVIDIA ключ
# wget -qO - https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/////x86_64/3bf863cc.pub | sudo apt-key add -

wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt update
sudo apt install -y cuda-toolkit-12-4
nvcc --version

# Шаг 2: Добавить репозиторий
echo "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64 /" | sudo tee /etc/apt/sources.list.d/cuda.list


# Шаг 3: Обновить пакеты
sudo apt update

# Шаг 4: Установить CUDA Toolkit 12.4
sudo apt install -y cuda-toolkit-12-4

# Проверка установки
nvcc --version
```

**Способ 2: Через cuda-keyring (новый способ, если способ 1 не сработал)**

```bash
# Шаг 1: Скачать cuda-keyring
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb

# Шаг 2: Установить keyring
sudo dpkg -i cuda-keyring_1.1-1_all.deb

# Шаг 3: Обновить пакеты
sudo apt update

# Шаг 4: Установить CUDA Toolkit
sudo apt install -y cuda-toolkit-12-4

# Проверка
nvcc --version
```

**Способ 3: Если не сработают способы 1-2 (минимальная CUDA без полного Toolkit)**

```bash
# Установить только необходимое
sudo apt install -y nvidia-cuda-dev nvidia-cuda-toolkit

# Или найти доступные CUDA пакеты
apt search cuda | grep toolkit
```

### Проверка после установки:

```bash
# Должны увидеть версию CUDA
nvcc --version

# Проверить наличие cuFFT, cuBLAS
ls /usr/local/cuda/lib64/ | grep cu

# Добавить в PATH (если нужно)
echo 'export CUDA_HOME=/usr/local/cuda' >> ~/.bashrc
echo 'export PATH=$CUDA_HOME/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

---

## ПРОБЛЕМА 2: CMakeLists.txt не найден

### Ошибка:
```
CMake Error: The source directory "/home/alex" does not appear to contain CMakeLists.txt.
```

### Причина:
Вы в директории `~` (home), а там нет CMakeLists.txt. Нужно:
1. Создать проект в правильной директории, ИЛИ
2. Перейти в директорию, где есть CMakeLists.txt

### Решение:

**Вариант А: Перейти в директорию вашего проекта LCH-Farrow**

```bash
# Вы уже видели эту директорию в приглашении
cd ~/C++/LCH-Farrow

# Проверить наличие CMakeLists.txt
ls -la CMakeLists.txt

# Теперь можно запустить cmake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

**Вариант Б: Создать новый проект с нуля**

```bash
# Шаг 1: Создать директорию проекта
mkdir -p ~/C++/HelloCMake
cd ~/C++/HelloCMake

# Шаг 2: Создать CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(HelloCMake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hello src/main.cpp)
target_include_directories(hello PRIVATE ${CMAKE_SOURCE_DIR}/include)
EOF

# Шаг 3: Создать структуру папок
mkdir -p src include

# Шаг 4: Создать main.cpp
cat > src/main.cpp << 'EOF'
#include <iostream>

int main() {
    std::cout << "Hello from CMake!\n";
    return 0;
}
EOF

# Шаг 5: Собрать проект
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# Шаг 6: Запустить
./build/hello
```

**Вариант В: Скопировать существующий проект**

```bash
# Если у вас есть проект в другом месте
cd ~/C++/LCH-Farrow

# Убедиться что там есть CMakeLists.txt
ls CMakeLists.txt

# Очистить старую сборку
rm -rf build

# Собрать заново
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

---

## ПРОБЛЕМА 3: Директория build не существует

### Ошибка:
```
ninja: fatal: chdir to 'build' - No such file or directory.
```

### Причина:
Сначала нужно запустить `cmake -B build`, а потом `ninja -C build`.

### Решение:

**Правильный порядок команд:**

```bash
# 1. Убедитесь что вы в директории проекта
cd ~/C++/LCH-Farrow
pwd  # Проверить текущую директорию

# 2. Запустить CMake (создаст папку build)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. ТЕПЕРЬ можно запустить Ninja
ninja -C build

# 4. Запустить исполняемый файл
./build/hello
```

**Если что-то пошло не так, полная очистка:**

```bash
# Удалить всю папку build
rm -rf build

# Удалить кэш CMake (если есть)
rm -rf CMakeCache.txt CMakeFiles/

# Начать заново
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

---

## ПОЛНЫЙ WORKFLOW ДЛЯ ВАШЕГО ПРОЕКТА

### Сейчас вы находитесь в: `/home/alex`

**Шаг 1: Создать структуру проекта**

```bash
# Перейти в директорию C++
cd ~/C++

# Создать проект LCH-Farrow (если его еще нет)
mkdir -p LCH-Farrow
cd LCH-Farrow

# Создать структуру папок
mkdir -p src include build

# Проверить структуру
ls -la
# Должно быть:
# drwxr-xr-x src/
# drwxr-xr-x include/
# drwxr-xr-x build/
```

### Шаг 2: Создать CMakeLists.txt

```bash
# Находитесь в ~/C++/LCH-Farrow

cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(LCH-Farrow VERSION 1.0.0 LANGUAGES CXX)

# Стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Флаги оптимизации для GCC 13.3.0
if(NOT MSVC)
    add_compile_options(-O3 -march=native)
endif()

# Создать исполняемый файл
add_executable(test_app src/main.cpp)

# Подключить include директорию
target_include_directories(test_app PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Вывод информации о сборке
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
EOF

cat CMakeLists.txt  # Проверить содержимое
```

### Шаг 3: Создать исходный файл

```bash
cat > src/main.cpp << 'EOF'
#include <iostream>
#include <vector>

int main() {
    std::cout << "Hello from CMake with GCC 13.3.0!\n";
    std::cout << "Testing basic C++17 features\n";
    
    std::vector<int> v = {1, 2, 3, 4, 5};
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << "\n";
    
    return 0;
}
EOF

cat src/main.cpp  # Проверить содержимое
```

### Шаг 4: Собрать проект с Ninja

```bash
# Текущая директория должна быть ~/C++/LCH-Farrow

# Создать конфигурацию с Ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Проверить что build создана
ls -la build/
# Должно быть несколько файлов CMake

# Собрать проект
ninja -C build

# Проверить что создан исполняемый файл
ls -la build/test_app
```

### Шаг 5: Запустить приложение

```bash
# Запустить
./build/test_app

# Ожидаемый вывод:
# Hello from CMake with GCC 13.3.0!
# Testing basic C++17 features
# 1 2 3 4 5
```

---

## БЫСТРАЯ ШПАРГАЛКА КОМАНД

### Если вы в ~/C++/LCH-Farrow:

```bash
# 1. Первый раз (создает конфигурацию)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. Сборка
ninja -C build

# 3. Запуск
./build/test_app

# 4. Очистка (удалить сборку)
rm -rf build

# 5. Переконфигурация (если изменили CMakeLists.txt)
cmake --reconfigure -B build

# 6. Полный пересброс
rm -rf build && cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && ninja -C build
```

### Если вы в другой директории:

```bash
# Перейти в проект
cd ~/C++/LCH-Farrow

# Потом выполнять команды выше
```

---

## ПРОВЕРКА ВСЕХ ИНСТРУМЕНТОВ ПОСЛЕ УСТАНОВКИ

```bash
# Все в одной команде
cmake --version && gcc --version && g++ --version && make --version && git --version && ninja --version && nvcc --version && python3 --version

# Ожидаемый результат:
# cmake version 3.28.3 ✅
# gcc (Ubuntu 13.3.0...) 13.3.0 ✅
# g++ (Ubuntu 13.3.0...) 13.3.0 ✅
# GNU Make 4.3 ✅
# git version 2.43.0 ✅
# 1.12.0 ✅ (вместо "not found")
# nvcc: NVIDIA (R) Cuda compiler driver
# cuda compilation tools, release 12.4, V12.4.xx ✅
# Python 3.12.1 ✅ (или другая версия)
```

---

## ЕСЛИ ВСЕ ЕЩЕ НЕ РАБОТАЕТ

### Debug CMake конфигурации:

```bash
# Находитесь в ~/C++/LCH-Farrow

# Verbose режим
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release --debug-output

# Если ошибка - смотрите вывод и ищите строку с ERROR

# Полная трассировка
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release --trace
```

### Debug Ninja сборки:

```bash
# Подробный вывод при сборке
ninja -C build -v

# Только подготовка без компиляции
ninja -C build -n

# Один файл за раз (медленнее, но понятнее)
ninja -C build -j 1 -v
```

### Debug исполнения:

```bash
# Запустить с выводом отладочной информации
./build/test_app --verbose

# Или через gdb (если установлен)
gdb ./build/test_app
# (потом вводите "run" и нажимаете Enter)
```

---

## ИТОГОВЫЙ ПЛАН ДЕЙСТВИЙ

### 1️⃣ Установить CUDA (если еще не установлена)

```bash
# Одна из команд:
sudo apt install cuda-toolkit-12-4
# или
sudo apt install nvidia-cuda-toolkit
# или смотрите решение выше

# Проверка
nvcc --version
```

### 2️⃣ Перейти в проект

```bash
cd ~/C++/LCH-Farrow
pwd  # Должно вывести: /home/alex/C++/LCH-Farrow
```

### 3️⃣ Убедиться что есть CMakeLists.txt

```bash
ls CMakeLists.txt  # Должен быть файл
cat CMakeLists.txt  # Должна быть конфигурация
```

### 4️⃣ Собрать проект

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
```

### 5️⃣ Запустить

```bash
./build/test_app
```

### 6️⃣ Проверить все версии

```bash
cmake --version && gcc --version && g++ --version && ninja --version && nvcc --version
```

---

## ВАШ ТОЧНЫЙ ПУТЬ

```
Текущее местоположение: /home/alex
Нужная директория: /home/alex/C++/LCH-Farrow

Команды:
1. cd ~/C++/LCH-Farrow
2. pwd  # проверить что вы там
3. ls CMakeLists.txt  # проверить файл
4. cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
5. ninja -C build
6. ./build/test_app
```

---

## ВЫВОД

**Проблема 1 (CUDA не найдена):** Нужно добавить NVIDIA репозиторий
**Проблема 2 (CMakeLists.txt не найден):** Нужно перейти в ~/C++/LCH-Farrow
**Проблема 3 (build не существует):** Сначала cmake, потом ninja

Все три проблемы решаются простыми командами выше! 🎯
