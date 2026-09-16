@echo off
setlocal

echo ========================================
echo        zevMobile Windows Builder
echo ========================================
echo.

where gcc >nul 2>nul
if errorlevel 1 (
    echo ERROR: GCC was not found in PATH.
    echo.
    echo Install a Windows GCC toolchain such as MSYS2 UCRT64,
    echo then make sure its bin directory is in your PATH.
    echo.
    echo Example:
    echo   C:\msys64\ucrt64\bin
    echo.
    pause
    exit /b 1
)

if not exist build mkdir build

echo Building zevMobile...
echo.

gcc -std=c11 -Wall -Wextra -O2 ^
    emulator\main.c ^
    emulator\phone.c ^
    kernel\kernel.c ^
    kernel\fs.c ^
    kernel\process.c ^
    kernel\syscall.c ^
    -o build\zevmobile.exe ^
    -mwindows -lgdi32 -luser32

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Output: build\zevmobile.exe
echo.

choice /C YN /N /M "Run zevMobile now? [Y/N] "
if errorlevel 2 goto :done
if errorlevel 1 start "zevMobile" "build\zevmobile.exe"

:done
endlocal
