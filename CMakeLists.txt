cmake_minimum_required(VERSION 3.20)
project(LGE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required LLVM components
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set up LLVM
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Find the LLVM libraries we need
llvm_map_components_to_libnames(llvm_libs support core irreader)

include(FetchContent)
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11
    GIT_TAG        v2.3.2
)
FetchContent_MakeAvailable(CLI11)

FetchContent_Declare(
    frozen
    GIT_REPOSITORY https://github.com/serge-sans-paille/frozen.git
    GIT_TAG master
)
FetchContent_MakeAvailable(frozen)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create executable
add_executable(lgec
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/codegen.cpp
)

# Link libraries
target_link_libraries(lgec ${llvm_libs} CLI11::CLI11 frozen::frozen)

# LLVM requires special handling for some targets
target_compile_features(lgec PRIVATE cxx_std_20)

add_subdirectory(runtime)
