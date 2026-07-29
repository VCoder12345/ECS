include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.15.3)

FetchContent_MakeAvailable(Catch2)

add_executable(ecs_tests test.cpp)
target_link_libraries(ecs_tests PRIVATE Catch2::Catch2WithMain ecs)
