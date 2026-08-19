#include <ecs/Profiling.h>
#include <ecs/World.h>

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

int main() {
  World world;

  constexpr int entityCount = 5'000;

  for (int i = 0; i < entityCount; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);

    world.addComponentToEntity<Velocity>(e, 1.0f, 2.0f, 3.0f);
  }

  constexpr int frameCount = 10'000;

  for (int frame = 0; frame < frameCount; ++frame) {
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
