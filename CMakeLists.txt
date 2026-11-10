cmake_minimum_required(VERSION 3.22)

project(clip5 VERSION 0.9.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(CLIP5_BUILD_PLUGIN "Compilar el plugin (JUCE)" ON)
option(CLIP5_BUILD_TESTS  "Compilar las pruebas (Catch2)" ON)
option(CLIP5_BUILD_BENCH  "Compilar el benchmark de CPU" ON)
set(CLIP5_AAX_SDK_PATH "" CACHE PATH "Ruta al AAX SDK (opcional; no es público)")

# macOS Universal (arm64 + x86_64)
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET ON)

# ------------------------------------------------------------------ motor DSP (sin JUCE)
add_library(clip5_dsp STATIC
    dsp/src/Curves.cpp
    dsp/src/CurveTable.cpp
    dsp/src/FilterDesign.cpp
    dsp/src/TableBank.cpp
    dsp/src/Engine.cpp)
target_include_directories(clip5_dsp PUBLIC dsp/include)
find_package(Threads REQUIRED)
target_link_libraries(clip5_dsp PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(clip5_dsp PRIVATE /W4 /O2)
else()
    target_compile_options(clip5_dsp PRIVATE -Wall -Wextra -O3)
endif()

# ------------------------------------------------------------------ pruebas
if(CLIP5_BUILD_TESTS)
    FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.16.1 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
    add_executable(clip5_tests
        tests/test_curvas.cpp
        tests/test_filtros.cpp
        tests/test_motor.cpp
        tests/test_medicion.cpp)
    target_link_libraries(clip5_tests PRIVATE clip5_dsp Catch2::Catch2WithMain)
    enable_testing()
    add_test(NAME clip5_tests COMMAND clip5_tests)
endif()

# ------------------------------------------------------------------ benchmark
if(CLIP5_BUILD_BENCH)
    add_executable(clip5_bench bench/clip5_bench.cpp)
    target_link_libraries(clip5_bench PRIVATE clip5_dsp)
endif()

# ------------------------------------------------------------------ plugin
if(CLIP5_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()
