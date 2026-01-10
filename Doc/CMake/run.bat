@echo off
cls
if "%1"=="" (
    echo.
    echo ERROR: Missing parameter!
    echo Usage: start.bat de  or  start.bat re
    echo.
    pause
    exit /b 1
)

if /i "%1"=="de" (
    echo.
    echo ========================
    echo DEBUG MODE
    echo ========================
    echo.
    if exist build rmdir /s /q build
    cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DENABLE_CUDA=OFF
    if %errorlevel% neq 0 (
        echo CMAKE ERROR!
        pause
        exit /b 1
    )
    cmake --build build --config Debug
    if %errorlevel% neq 0 (
        echo BUILD ERROR!
        pause
        exit /b 1
    )
    echo.
    echo SUCCESS - Debug build complete
    pause
    exit /b 0
)

if /i "%1"=="re" (
    echo.
    echo ========================
    echo RELEASE MODE
    echo ========================
    echo.
    if exist build rmdir /s /q build
    cmake -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=OFF
    if %errorlevel% neq 0 (
        echo CMAKE ERROR!
        pause
        exit /b 1
    )
    cmake --build build --config Release
    if %errorlevel% neq 0 (
        echo BUILD ERROR!
        pause
        exit /b 1
    )
    echo.
    echo SUCCESS - Release build complete
    pause
    exit /b 0
)

echo ERROR: Use de or re
pause
exit /b 1