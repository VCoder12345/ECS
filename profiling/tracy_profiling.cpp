#include <ecs/World.h>
#include <tracy/Tracy.hpp>

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

int main() {
  World world;

  constexpr int entityCount = 1'000'000;

  for (int i = 0; i < entityCount; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);

    world.addComponentToEntity<Velocity>(e, 1.0f, 2.0f, 3.0f);
  }

  while (true) {
    {
      ZoneScopedN("ECS update");

      world.each<Position, Velocity>([](Entity, Position &p, Velocity &v) {
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
      });
    }

    FrameMark;
  }
}
