cmake_minimum_required(VERSION 3.15)

project(lceda-pro-sim-server LANGUAGES CXX)

# ========== 基础配置 ==========
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_WASM "Build with Emscripten" ON)

# ========== wasm 环境 ==========
if(EMSCRIPTEN)
    message(STATUS "Emscripten build enabled")

    # 指定输出文件位置
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)

    # 输出 .js 文件而非 .html 文件以供库使用
    set(CMAKE_EXECUTABLE_SUFFIX ".js")

    # 编译器标志
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

    # Emscripten链接标志 
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
        -sWASM=1 \
        -sMODULARIZE=1 \
        -sEXPORT_ES6=1 \
        -sENVIRONMENT=web \
        -sALLOW_MEMORY_GROWTH=1 \
        -sEXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\",\"FS\"] \
        -sEXIT_RUNTIME=0 \
        -sNO_DISABLE_EXCEPTION_CATCHING \
        -sASSERTIONS=1 \
        -sFORCE_FILESYSTEM=1 \
        --embed-file ${CMAKE_SOURCE_DIR}/config/data@/config/data \
    ")

    message(STATUS "Embedding only config/data folder into WASM")
else()
    message(STATUS "Native build")
endif()

# ========== include ==========
include_directories(
    ${CMAKE_SOURCE_DIR}/src/public/include
)

# ========== 子模块 ==========
add_subdirectory(src/cirSim/src)

# ========== wasm 入口 ==========
add_executable(lceda-pro-sim-server
    src/server.cpp
)

target_link_libraries(lceda-pro-sim-server
    CirSimModule
)
