@echo off
REM Simple compilation script - no CMake needed!

REM Initialize Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Compile directly
cl /std:c++20 /O2 /EHsc /I include src\main.cpp src\lowlatency.cpp /Fe:main.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✓ Build successful! Run with: main.exe
) else (
    echo.
    echo ✗ Build failed!
)
