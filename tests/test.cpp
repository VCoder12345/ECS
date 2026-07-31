#include <catch2/catch_test_macros.hpp>

#include <ecs/World.h>
#include <ecs/Archetype.h>

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

struct Health {
  int hearts;
};

inline bool floatEqual(float a, float b, float eps = 0.001f) {
  return std::abs(a - b) < eps;
}

TEST_CASE("ECS creation", "[ecs]") { 
  World ecs; 

  Entity e1 = ecs.createEntity();
  Entity e2 = ecs.createEntity();
  REQUIRE(e1 + 1 == e2);
}

TEST_CASE("Archetype creation", "[arch]") {
  Archetype at = Archetype::create<Position, Health>();
  
  REQUIRE(at.getMask().test(getComponentID<Position>()));
  REQUIRE(at.getMask().test(getComponentID<Health>()));
  REQUIRE(!at.getMask().test(getComponentID<Velocity>()));
}
