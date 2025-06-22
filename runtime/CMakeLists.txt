cmake_minimum_required(VERSION 3.20)
project(LGE_Runtime)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(lge_runtime SHARED lge_runtime.c)

set_target_properties(lge_runtime PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

set_target_properties(lge_runtime PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/runtime
)
