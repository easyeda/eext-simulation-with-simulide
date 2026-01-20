
# 递归查找所有源文件（确保完整性）
file(GLOB_RECURSE CIRSIM_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# Add TinyXML source files
set(TINYXML_SRC
    ${CMAKE_SOURCE_DIR}/external/tinyxml/tinyxml/tinyxml.cpp
    ${CMAKE_SOURCE_DIR}/external/tinyxml/tinyxml/tinyxmlerror.cpp
    ${CMAKE_SOURCE_DIR}/external/tinyxml/tinyxml/tinyxmlparser.cpp
    ${CMAKE_SOURCE_DIR}/external/tinyxml/tinyxml/tinystr.cpp
)

# Combine all sources
set(ALL_SRC ${CIRSIM_SRC} ${TINYXML_SRC})

# 保持OBJECT库类型（增量构建优势）
add_library(CirSimModule OBJECT ${ALL_SRC})

# Only use precompiled headers for non-WASM builds
if(NOT EMSCRIPTEN)
    target_precompile_headers(CirSimModule PRIVATE ${CMAKE_SOURCE_DIR}/src/public/include/pch.h)
endif()

target_include_directories(CirSimModule
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/cirSim/include
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/e-elements
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements
        ${CMAKE_SOURCE_DIR}/external/asio/include
        ${CMAKE_SOURCE_DIR}/external/tinyxml/tinyxml
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/meters
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/memory
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/properties
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/e-elements/outputs
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/e-elements/active
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/e-elements/passive
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elmData
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/componentselector
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/outputs
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/outputs/motors
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/outputs/displays
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/outputs/leds
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/sources
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/active
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/switches
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/passive
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/passive/resistors
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/passive/resist_sensors
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/passive/reactive
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/logic
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/micro
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/components/other
        ${CMAKE_SOURCE_DIR}/src/cirSim/include/CircuitElm/elements/subcircuits
)

