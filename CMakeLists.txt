cmake_minimum_required(VERSION 4.0)
project(
  ECSLibrary
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build the tests" ON)
option(BUILD_BENCHMARKS "Build the benchmarks" OFF)
option(ENABLE_TRACY "Enable Tracy profiler" OFF)

add_library(ecs STATIC src/World.cpp src/Column.cpp)

target_compile_options(
  ecs
  PRIVATE -Wall -Wextra -Wshadow
          # -Wconversion
          -Werror)

target_include_directories(ecs PUBLIC include)

if(BUILD_TESTS)
  # include(CTest) enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(ENABLE_TRACY)
  add_subdirectory(profiling)
endif()
