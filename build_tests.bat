@echo off
set "PATHS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat""

for %%p in (%PATHS%) do (
    if exist %%p (
        call %%p x64
        goto :found
    )
)

echo "vcvarsall.bat not found"
exit /b 1

:found
echo "Building codec_test..."
cl /Zi /EHsc /std:c++17 /Iinclude tests/codec_test.cpp /Fe:codec_test.exe
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

echo "Building loopback_test..."
cl /Zi /EHsc /std:c++17 /Iinclude tests/day4/loopback_test.cpp src/engine/engine.cpp src/engine/matching_engine.cpp src/order/order_book.cpp src/net/socket.cpp ws2_32.lib /Fe:loopback_test.exe
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%
