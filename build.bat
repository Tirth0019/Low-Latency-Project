@echo off
REM Build script for Low Latency Project using MSVC

REM Initialize Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Create build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Configure with CMake (if CMake is available)
if exist "C:\Program Files\CMake\bin\cmake.exe" (
    "C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022" -A x64
    cmake --build . --config Release
    echo.
    echo Build complete! Executable is in: build\Release\LowLatencyProject.exe
) else (
    echo CMake not found. Please install CMake or compile manually.
    echo.
    echo Manual compilation:
    echo cl /std:c++20 /O2 /EHsc /I ..\include ..\src\main.cpp ..\src\lowlatency.cpp /Fe:main.exe
)

cd ..
pause
