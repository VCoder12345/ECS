# 1. Include the FetchContent module
include(FetchContent)

# 1. Configure Google Benchmark to skip its own tests and docs. This speeds up
#   our build significantly.
set(BENCHMARK_ENABLE_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL
    OFF
    CACHE BOOL "" FORCE)

set(BENCHMARK_ENABLE_WERROR
    OFF
    CACHE BOOL "" FORCE)

# 1. Declare the external dependency
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.5)

# 1. Download and make the library available
FetchContent_MakeAvailable(googlebenchmark)

# Define the executable
add_executable(ecs_bench benchmark.cpp)

# Link the dependencies
target_link_libraries(
  ecs_bench
  PRIVATE # The Google Benchmark library
          benchmark::benchmark
          # Provides a default main() function for us
          benchmark::benchmark_main
          # Our own algorithms library
          ecs)

target_compile_options(ecs_bench PRIVATE -Wall -Wextra -Wshadow #[[ -Wconversion ]]
                                         -Werror)
