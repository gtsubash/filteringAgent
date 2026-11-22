cmake_minimum_required(VERSION 3.18)
project(FilteringEngine VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_GPU "Enable GPU accelerated backends" OFF)

# Core Library
add_library(filtering_core STATIC
    src/fir_filter.cpp
    src/filter_design.cpp
    src/spectrum.cpp
    src/html_report.cpp
)

target_include_directories(filtering_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(filtering_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(filtering_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# CLI Demo Application
add_executable(fir_demo apps/fir_demo.cpp)
target_link_libraries(fir_demo PRIVATE filtering_core)
if(MSVC)
    target_compile_options(fir_demo PRIVATE /W4 /permissive-)
endif()

# Standalone Visualization Tool (fir_viewer)
add_executable(fir_viewer apps/signal_viewer.cpp)
target_link_libraries(fir_viewer PRIVATE filtering_core)
if(MSVC)
    target_compile_options(fir_viewer PRIVATE /W4 /permissive-)
endif()

# Unit Tests
enable_testing()
add_executable(fir_tests tests/test_fir_filter.cpp)
target_link_libraries(fir_tests PRIVATE filtering_core)
if(MSVC)
    target_compile_options(fir_tests PRIVATE /W4 /permissive-)
endif()

add_test(NAME fir_tests COMMAND fir_tests)
