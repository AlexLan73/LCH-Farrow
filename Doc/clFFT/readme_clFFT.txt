
## clFFT для Windows и Ubuntu

### Официальный репозиторий (рекомендуется)

clMathLibraries/clFFT на GitHub — это главный источник для clFFT. Здесь вы найдёте исходный код и инструкции по сборке.[1]

**Ссылка:** https://github.com/clMathLibraries/clFFT

### Для Ubuntu

**Вариант 1: Через пакетный менеджер (самый простой)**

```bash
sudo apt-get install libclfft-dev libclfft3
```

Пакеты доступны в официальном репозитории Ubuntu Launchpad.[2]

**Вариант 2: Сборка из исходников**

```bash
git clone https://github.com/clMathLibraries/clFFT.git
cd clFFT
mkdir build && cd build
cmake ../src
make -j$(nproc)
sudo make install
```

### Для Windows

**Вариант 1: Скачать готовые бинарники**

На странице Releases в GitHub можно найти скомпилированные версии для Windows с файлами:[3]
- `clFFT.dll`
- `clFFT.lib`
- `clFFT.h`

**Вариант 2: Сборка из исходников**

1. Клонируйте репозиторий
2. Используйте CMake GUI или командную строку
3. Соберите с Visual Studio (рекомендуется для вашей конфигурации VS 2022)

```bash
git clone https://github.com/clMathLibraries/clFFT.git
cd clFFT
mkdir build && cd build
cmake -G "Visual Studio 17 2022" ../src
cmake --build . --config Debug
```

### Документация

Полная документация доступна на GitHub Pages проекта, там описаны все API функции и примеры использования.[4]

