cmake_minimum_required(VERSION 3.13)
# set(CMAKE_C_COMPILER gcc-14)
# set(CMAKE_CXX_COMPILER g++-14)

project(sandbox)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES src/sandbox.cpp src/syscall.h src/util.h src/tracer.h src/subprocess.h src/parameters.h src/config.h)
set(WARNING_FLAGS
        -Wall -Wextra -Wpedantic -pedantic-errors
        -Warray-bounds=2 -Wcast-align -Wcast-qual -Wconversion -Wdisabled-optimization -Wduplicated-cond
        -Wduplicated-branches -Weffc++ -Wfloat-equal -Wformat=2 -Wlogical-op -Wmissing-declarations
        -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-noreturn -Wnull-dereference
        -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo
        -Wstack-protector -Wstrict-aliasing=2 -Wstrict-overflow=5 -Wswitch-enum -Wundef -Wunreachable-code
        -Wunused -Wzero-as-null-pointer-constant)

add_executable(sandbox ${SOURCES})
target_compile_options(sandbox PRIVATE ${WARNING_FLAGS} -O2)

add_executable(sandbox_coverage EXCLUDE_FROM_ALL ${SOURCES})
target_compile_options(sandbox_coverage PRIVATE ${WARNING_FLAGS} -DCOVERAGE --coverage -O0)
target_link_options(sandbox_coverage PRIVATE --coverage)

add_custom_target(clean-build
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/sandbox
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/sandbox_coverage
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.gcno
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.gcda
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles/sandbox.dir
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles/sandbox_coverage.dir
    COMMENT "Cleaning build artifacts"
)
