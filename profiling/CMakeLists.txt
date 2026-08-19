set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy" FORCE)

include(FetchContent)

FetchContent_Declare(
  tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy.git
  GIT_TAG v0.14.0
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(tracy)

target_link_libraries(ecs PRIVATE Tracy::TracyClient)
target_compile_definitions(ecs PRIVATE TRACY_ENABLE)

add_executable(ecs_tracy tracy_profiling.cpp)
target_link_libraries(ecs_tracy PRIVATE ecs Tracy::TracyClient)

target_compile_definitions(ecs_tracy PRIVATE TRACY_ENABLE)
