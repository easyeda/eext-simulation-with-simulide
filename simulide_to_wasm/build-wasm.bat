@echo off
REM Build script for WASM compilation

set "SCRIPT_DIR=%~dp0"
echo Setting up Emscripten environment...
call "D:\eda\emsdk\emsdk\emsdk_env.bat"
set "PATH=C:\Program Files\CMake\bin;D:\eda\emsdk\emsdk\upstream\bin;%PATH%"

cd /d "%SCRIPT_DIR%"
if not exist build-wasm mkdir build-wasm
cd build-wasm

echo Configuring with emcmake...
call emcmake cmake .. -G "MinGW Makefiles" -DENABLE_WASM=ON

echo Building WASM...
call emmake make -j4

echo Build complete!
cd ..
