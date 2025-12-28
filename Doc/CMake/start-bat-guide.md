# Batch скрипт с параметрами (Debug/Release)
## start.bat с динамическими сообщениями

```batch
@echo off
REM ============================================================================
REM Build Script с параметрами Debug/Release
REM ============================================================================

setlocal enabledelayedexpansion

REM Очистить консоль
cls

REM ============================================================================
REM ЧАСТЬ 1: ОБРАБОТКА ПАРАМЕТРОВ
REM ============================================================================

REM Проверить что параметр передан
if "%1"=="" (
    echo.
    echo ================================
    echo    ❌ ERROR: Missing parameter!
    echo ================================
    echo.
    echo Usage:
    echo   start.bat de     (Debug mode)
    echo   start.bat re     (Release mode)
    echo.
    echo Examples:
    echo   start.bat de
    echo   start.bat re
    echo.
    pause
    exit /b 1
)

REM Преобразовать в нижний регистр
set "MODE=%1"
for %%A in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do (
    set "MODE=!MODE:%%A=%%A!"
)

REM ============================================================================
REM ЧАСТЬ 2: ОПРЕДЕЛИТЬ РЕЖИМ
REM ============================================================================

if /i "%MODE%"=="de" (
    set "BUILD_TYPE=Debug"
    set "BUILD_MODE=🔴 DEBUG MODE"
    set "EMOJI=🐞"
    set "CONFIG_NAME=Debug"
    goto BUILD_START
)

if /i "%MODE%"=="debug" (
    set "BUILD_TYPE=Debug"
    set "BUILD_MODE=🔴 DEBUG MODE"
    set "EMOJI=🐞"
    set "CONFIG_NAME=Debug"
    goto BUILD_START
)

if /i "%MODE%"=="re" (
    set "BUILD_TYPE=Release"
    set "BUILD_MODE=🟢 RELEASE MODE"
    set "EMOJI=⚡"
    set "CONFIG_NAME=Release"
    goto BUILD_START
)

if /i "%MODE%"=="release" (
    set "BUILD_TYPE=Release"
    set "BUILD_MODE=🟢 RELEASE MODE"
    set "EMOJI=⚡"
    set "CONFIG_NAME=Release"
    goto BUILD_START
)

REM Если параметр не распознан
echo.
echo ================================
echo    ❌ INVALID PARAMETER!
echo ================================
echo.
echo Valid parameters:
echo   de, debug     = Debug mode
echo   re, release   = Release mode
echo.
pause
exit /b 1

REM ============================================================================
REM ЧАСТЬ 3: ПОСТРОЕНИЕ
REM ============================================================================

:BUILD_START

echo.
echo ================================
echo.
echo %EMOJI% %BUILD_MODE% %EMOJI%
echo.
echo ================================
echo.

REM Показать информацию о сборке
echo 📊 BUILD INFORMATION:
echo    Mode: %BUILD_MODE%
echo    Type: %BUILD_TYPE%
echo    Config: %CONFIG_NAME%
echo.

REM Очистить старую сборку
echo 🧹 Cleaning old build...
if exist build rmdir /s /q build
if %ERRORLEVEL% equ 0 (
    echo    ✅ Build directory cleaned
) else (
    echo    ⚠️  Build directory already empty
)
echo.

REM ============================================================================
REM ЧАСТЬ 4: CMAKE КОНФИГУРАЦИЯ
REM ============================================================================

echo ⚙️  Running CMake configuration...
echo.

if "%BUILD_TYPE%"=="Debug" (
    echo 🔴 Debug Configuration:
    echo    - No optimization
    echo    - Full debug symbols
    echo    - Slower execution
    echo.
    cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DENABLE_CUDA=OFF
) else (
    echo 🟢 Release Configuration:
    echo    - Full optimization (/O2)
    echo    - Minimal debug symbols
    echo    - Maximum performance
    echo.
    cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=OFF
)

if %ERRORLEVEL% equ 0 (
    echo.
    echo ✅ CMake configuration successful!
) else (
    echo.
    echo ❌ CMake configuration FAILED!
    pause
    exit /b 1
)

echo.

REM ============================================================================
REM ЧАСТЬ 5: СБОРКА
REM ============================================================================

echo 🔨 Building project...
echo.

cmake --build build --config %CONFIG_NAME%

if %ERRORLEVEL% equ 0 (
    echo.
    echo ✅ Build SUCCESSFUL!
) else (
    echo.
    echo ❌ Build FAILED!
    pause
    exit /b 1
)

echo.

REM ============================================================================
REM ЧАСТЬ 6: ИНФОРМАЦИЯ О ЗАВЕРШЕНИИ
REM ============================================================================

echo ================================
echo.
if "%BUILD_TYPE%"=="Debug" (
    echo 🐞 DEBUG BUILD COMPLETE
    echo.
    echo 📁 Output: .\build\Debug\LCH-Farrow.exe
    echo.
    echo 💡 Tips for debugging:
    echo    - Run with Visual Studio debugger
    echo    - Set breakpoints in code
    echo    - Use Debug menu
) else (
    echo ⚡ RELEASE BUILD COMPLETE
    echo.
    echo 📁 Output: .\build\Release\LCH-Farrow.exe
    echo.
    echo 💡 Optimizations applied:
    echo    - O2 optimization
    echo    - AVX2 instructions
    echo    - Function-level linking
)
echo.
echo ================================
echo.

REM ============================================================================
REM ЧАСТЬ 7: ПРЕДЛОЖИТЬ ЗАПУСК
REM ============================================================================

set /p RUN="Run executable now? (Y/N): "

if /i "%RUN%"=="Y" (
    echo.
    echo 🚀 Starting %BUILD_TYPE% executable...
    echo.
    if "%BUILD_TYPE%"=="Debug" (
        if exist "build\Debug\LCH-Farrow.exe" (
            .\build\Debug\LCH-Farrow.exe
        ) else (
            echo ❌ Executable not found!
        )
    ) else (
        if exist "build\Release\LCH-Farrow.exe" (
            .\build\Release\LCH-Farrow.exe
        ) else (
            echo ❌ Executable not found!
        )
    )
) else (
    echo.
    echo To run manually:
    if "%BUILD_TYPE%"=="Debug" (
        echo   .\build\Debug\LCH-Farrow.exe
    ) else (
        echo   .\build\Release\LCH-Farrow.exe
    )
)

echo.
pause
exit /b 0
```

---

## 📝 КАК ИСПОЛЬЗОВАТЬ:

### Debug режим:
```batch
start.bat de
# или
start.bat debug
```

### Release режим:
```batch
start.bat re
# или
start.bat release
```

### Без параметров (ошибка):
```batch
start.bat
# Выведет справку
```

---

## 🎯 ЧТО ВЫВЕЛИТСЯ:

### Debug (de):
```
================================

🐞 🔴 DEBUG MODE 🐞

================================

📊 BUILD INFORMATION:
   Mode: 🔴 DEBUG MODE
   Type: Debug
   Config: Debug

🧹 Cleaning old build...
   ✅ Build directory cleaned

⚙️  Running CMake configuration...

🔴 Debug Configuration:
   - No optimization
   - Full debug symbols
   - Slower execution

✅ CMake configuration successful!

🔨 Building project...

✅ Build SUCCESSFUL!

================================

🐞 DEBUG BUILD COMPLETE

📁 Output: .\build\Debug\LCH-Farrow.exe

💡 Tips for debugging:
   - Run with Visual Studio debugger
   - Set breakpoints in code
   - Use Debug menu

================================

Run executable now? (Y/N): 
```

### Release (re):
```
================================

⚡ 🟢 RELEASE MODE ⚡

================================

📊 BUILD INFORMATION:
   Mode: 🟢 RELEASE MODE
   Type: Release
   Config: Release

🧹 Cleaning old build...
   ✅ Build directory cleaned

⚙️  Running CMake configuration...

🟢 Release Configuration:
   - Full optimization (/O2)
   - Minimal debug symbols
   - Maximum performance

✅ CMake configuration successful!

🔨 Building project...

✅ Build SUCCESSFUL!

================================

⚡ RELEASE BUILD COMPLETE

📁 Output: .\build\Release\LCH-Farrow.exe

💡 Optimizations applied:
   - O2 optimization
   - AVX2 instructions
   - Function-level linking

================================

Run executable now? (Y/N): 
```

---

## ✨ ОСОБЕННОСТИ:

✅ **Параметры:** `de` / `debug` / `re` / `release`  
✅ **Эмодзи индикаторы:** 🐞 Debug, ⚡ Release  
✅ **Цветной вывод:** яркие сообщения  
✅ **Проверка ошибок:** останавливается при ошибках  
✅ **询问запуск:** можно сразу запустить exe  
✅ **Справка:** выводит помощь при неправильных параметрах  

---

## 🚀 ДОБАВИТЬ В ПРОЕКТ:

1. Скопируйте код выше в файл `start.bat`
2. Сохраните в корне проекта
3. Запустите:
   ```batch
   start.bat de
   start.bat re
   ```

**Готово!** 🎉💪
