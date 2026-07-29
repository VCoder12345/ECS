#include <catch2/catch_test_macros.hpp>

#include <ecs/ECS.h>

TEST_CASE("ECS creation", "[ecs]") { 
  ECS ecs; 

  Entity e1 = ecs.createEntity();
  Entity e2 = ecs.createEntity();
  REQUIRE(e1 == e2);
}
