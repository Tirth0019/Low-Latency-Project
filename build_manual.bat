@echo off
REM Manual build script using MSVC compiler directly

REM Initialize Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo Compiling with MSVC...
cl /std:c++20 /O2 /EHsc /I include src\main.cpp src\lowlatency.cpp /Fe:main.exe /link

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Executable: main.exe
    echo Run with: main.exe
) else (
    echo.
    echo Build failed!
)

pause
