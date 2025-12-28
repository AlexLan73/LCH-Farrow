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
