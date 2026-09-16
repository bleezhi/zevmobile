@echo off
setlocal

echo ========================================
echo        zevMobile Windows Builder
echo ========================================
echo.
where gcc >nul 2>nul
if errorlevel 1 (echo ERROR: GCC was not found in PATH.&pause&exit /b 1)
if not exist build mkdir build

echo Building zevMobile kernel + JVM...
echo.
gcc -std=c11 -Wall -Wextra -O2 ^
    emulator\main.c emulator\phone.c ^
    kernel\kernel.c kernel\fs.c kernel\process.c kernel\syscall.c ^
    jvm\jvm.c jvm\launcher_class.c ^
    -o build\zevmobile.exe ^
    -mwindows -lgdi32 -luser32 -lwinmm
if errorlevel 1 (echo.&echo BUILD FAILED&pause&exit /b 1)
echo.&echo BUILD SUCCESSFUL!&echo Output: build\zevmobile.exe&echo.
choice /C YN /N /M "Run zevMobile now? [Y/N] "
if errorlevel 2 goto :done
if errorlevel 1 start "zevMobile" "build\zevmobile.exe"
:done
endlocal
