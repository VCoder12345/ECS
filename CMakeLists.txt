cmake_minimum_required(VERSION 4.0)
project(
  ECSLibrary
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build the tests" ON)

add_library(ecs STATIC src/ECS.cpp)

target_include_directories(ecs PUBLIC include)

if(BUILD_TESTS)
  # include(CTest)
  # enable_testing()
  add_subdirectory(tests)
endif()
