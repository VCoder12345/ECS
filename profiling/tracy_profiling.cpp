// A profiling harness that simulates a realistic frame loop rather than a
// single query in a tight loop. Each frame runs several systems with
// different cost profiles, so a Tracy capture shows where time actually
// goes across query iteration, archetype migration, and command flushing.

#include <ecs/Deferred.h>
#include <ecs/Profiling.h>
#include <ecs/World.h>

#include <cstdint>
#include <random>

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

struct Health {
  int hearts;
};

struct Lifetime {
  int framesRemaining;
};

// Marks an entity as recently damaged. Added and removed constantly, so it
// drives archetype migration traffic.
struct Damaged {
  int amount;
};

namespace {

constexpr int kInitialEntities = 20'000;
constexpr int kFrameCount = 5'000;
constexpr int kSpawnsPerFrame = 200;
constexpr int kProjectileLifetime = 90;

std::mt19937 rng(0xC0FFEE);

void spawnProjectile(World &world) {
  Entity e = world.createEntity();
  world.addComponent<Position>(e, 0.0f, 0.0f, 0.0f);
  world.addComponent<Velocity>(e, 1.0f, 0.5f, 0.25f);
  world.addComponent<Lifetime>(e, kProjectileLifetime);
}

void spawnActor(World &world) {
  Entity e = world.createEntity();
  world.addComponent<Position>(e, 0.0f, 0.0f, 0.0f);
  world.addComponent<Velocity>(e, 0.1f, 0.1f, 0.1f);
  world.addComponent<Health>(e, 100);
}

} // namespace

int main() {
  World world;

  {
    ZoneScopedN("World setup");

    // A mix of archetypes so queries have to filter, not just walk one
    // big contiguous block.
    for (int i = 0; i < kInitialEntities; ++i) {
      if (i % 3 == 0) {
        spawnProjectile(world);
      } else {
        spawnActor(world);
      }
    }
  }

  std::int64_t liveEntities = kInitialEntities;

  for (int frame = 0; frame < kFrameCount; ++frame) {
    // ---- Movement: pure iteration, no structural changes. This is the
    // best case and the number everything else should be compared to.
    {
      ZoneScopedN("Movement");

      world.each<Position, Velocity>([](Entity, Position &p, Velocity &v) {
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
      });
    }

    // ---- Damage: adds a component to a fraction of entities, so each
    // flush triggers archetype migrations.
    {
      ZoneScopedN("Damage system");

      int index = 0;
      eachDeferred<Position, Health>(
          world, [&](WorldCtxt &ctx, Entity e, Position &, Health &h) {
            if (index++ % 32 == frame % 32) {
              h.hearts -= 5;
              ctx.addComponent<Damaged>(e, 5);
            }
          });
    }

    // ---- Damage reaction: consumes the Damaged tag, migrating those
    // entities straight back. Exercises the reverse transition.
    {
      ZoneScopedN("Damage reaction");

      eachDeferred<Damaged, Health>(
          world, [](WorldCtxt &ctx, Entity e, Damaged &d, Health &h) {
            h.hearts -= d.amount / 2;
            ctx.removeComponent<Damaged>(e);
          });
    }

    // ---- Lifetime: destroys entities mid-iteration, the case deferral
    // exists for. Destruction count varies by frame.
    {
      ZoneScopedN("Lifetime system");

      eachDeferred<Lifetime>(world, [&](WorldCtxt &ctx, Entity e, Lifetime &l) {
        if (--l.framesRemaining <= 0) {
          ctx.destroyEntity(e);
          --liveEntities;
        }
      });
    }

    // ---- Death: entities that ran out of health.
    {
      ZoneScopedN("Death system");

      eachDeferred<Health>(world, [&](WorldCtxt &ctx, Entity e, Health &h) {
        if (h.hearts <= 0) {
          ctx.destroyEntity(e);
          --liveEntities;
        }
      });
    }

    // ---- Spawning: keeps the world from draining, and keeps the entity
    // ID free list churning so recycling is exercised every frame.
    {
      ZoneScopedN("Spawning");

      for (int i = 0; i < kSpawnsPerFrame; ++i) {
        if (rng() % 4 == 0) {
          spawnActor(world);
        } else {
          spawnProjectile(world);
        }
        ++liveEntities;
      }
    }

    // Plotted per frame so entity count is visible alongside frame time in
    // the Tracy timeline — useful for telling "slow frame" apart from
    // "frame with more entities".
    TracyPlot("Live entities", liveEntities);

    FrameMark;
  }

  return 0;
}
