@echo off
REM Build script for WASM compilation

echo Setting up Emscripten environment...
call "F:\Program Files (x86)\emsdk\emsdk\emsdk_env.bat"

if not exist build-wasm mkdir build-wasm
cd build-wasm

echo Configuring with emcmake...
call emcmake cmake .. -G "MinGW Makefiles" -DENABLE_WASM=ON

echo Building WASM...
call emmake make -j4

echo Build complete!
cd ..
