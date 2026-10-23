# SuiteSparse (KLU + AMD + COLAMD + BTF + SuiteSparse_config) 精简静态库
# 仅编译标准 double/int 版本(非 _l/_z/_zl 变体),供 SPI 引擎大块稀疏 LU 使用。

file(GLOB KLU_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/KLU/Source/*.c")
file(GLOB AMD_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/AMD/Source/*.c")
file(GLOB BTF_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/BTF/Source/*.c")
file(GLOB COLAMD_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/COLAMD/Source/*.c")

# 去掉变体源(在 v7.14 中同一目录包含 _l/_z/_zl 等 long/complex 版本,
# 它们与主线源共用符号前缀, 不编译可裁掉一半冗余; 但注意 klu.c 等非变体
# 源内部不引用变体, 去掉是安全且省时的。若以后需要 long-index, 再启用 _l 族)
list(FILTER KLU_SOURCES EXCLUDE REGEX "_l\\.c$|_z\\.c$|_zl\\.c$")
list(FILTER AMD_SOURCES EXCLUDE REGEX "_l\\.c$")
list(FILTER BTF_SOURCES EXCLUDE REGEX "_l\\.c$")
list(FILTER COLAMD_SOURCES EXCLUDE REGEX "_l\\.c$")

set(SUITESPARSESRC
    ${KLU_SOURCES}
    ${AMD_SOURCES}
    ${BTF_SOURCES}
    ${COLAMD_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse_config/SuiteSparse_config.c"
)

add_library(SuiteSparseKlu STATIC ${SUITESPARSESRC})

target_include_directories(SuiteSparseKlu PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/SuiteSparse_config"
    "${CMAKE_CURRENT_SOURCE_DIR}/KLU/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/AMD/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/BTF/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/COLAMD/Include"
)

# KLU 部分源文件使用 "MATH" 库(lm 在 Linux 下)
if(UNIX)
    target_link_libraries(SuiteSparseKlu PUBLIC m)
endif()

# 纯 C 代码, 禁用优化告警(项目级 -w 已生效, 此处兜底)
set_target_properties(SuiteSparseKlu PROPERTIES C_STANDARD 99)