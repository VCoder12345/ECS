#include <benchmark/benchmark.h>

#include <ecs/Deferred.h>
#include <ecs/World.h>

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

struct Health {
  int hearts;
};

struct Tag {
  std::uint32_t value;
};

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static void fillPositionWorld(World &world, std::size_t count,
                              std::vector<Entity> *out = nullptr) {
  for (std::size_t i = 0; i < count; ++i) {
    Entity e = world.createEntity();
    world.addComponent<Position>(e, 1.0f, 2.0f, 3.0f);
    if (out)
      out->push_back(e);
  }
}

static void fillPositionVelocityWorld(World &world, std::size_t count,
                                      std::vector<Entity> *out = nullptr) {
  for (std::size_t i = 0; i < count; ++i) {
    Entity e = world.createEntity();
    world.addComponent<Position>(e, 1.0f, 2.0f, 3.0f);
    world.addComponent<Velocity>(e, 1.0f, 2.0f, 3.0f);
    if (out)
      out->push_back(e);
  }
}

// Creates all seven combinations of {Position, Velocity, Health}.
// Four of them match a Position query, two match Position + Velocity.
static void fillManyArchetypesWorld(World &world,
                                    std::size_t entitiesPerArchetype) {
  auto spawn = [&](bool pos, bool vel, bool health) {
    for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
      Entity e = world.createEntity();
      if (pos)
        world.addComponent<Position>(e, 1.0f, 2.0f, 3.0f);
      if (vel)
        world.addComponent<Velocity>(e, 1.0f, 2.0f, 3.0f);
      if (health)
        world.addComponent<Health>(e, 100);
    }
  };

  spawn(true, false, false);
  spawn(false, true, false);
  spawn(false, false, true);
  spawn(true, true, false);
  spawn(true, false, true);
  spawn(false, true, true);
  spawn(true, true, true);
}

// ------------------------------------------------------------
// Entity creation / destruction
// ------------------------------------------------------------

static void BM_CreateEntities(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    World world;
    for (std::size_t i = 0; i < count; ++i) {
      benchmark::DoNotOptimize(world.createEntity());
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_CreateEntities)->Args({4096})->Args({65536})->Args({262144});

// Exercises the free-list path: every index is recycled rather than freshly
// allocated, which is the steady state for a long-running game.
static void BM_CreateEntitiesRecycled(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  World world;
  std::vector<Entity> entities;
  entities.reserve(count);

  for (auto _ : state) {
    for (std::size_t i = 0; i < count; ++i) {
      entities.push_back(world.createEntity());
    }
    for (Entity e : entities) {
      world.removeEntity(e);
    }
    entities.clear();
    benchmark::ClobberMemory();
  }

  // Each iteration does `count` creates and `count` destroys.
  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count) * 2);
}

BENCHMARK(BM_CreateEntitiesRecycled)->Args({4096})->Args({65536});

// ------------------------------------------------------------
// Add / remove components (immediate)
// ------------------------------------------------------------

static void BM_AddPosition(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    World world;
    fillPositionWorld(world, count);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_AddPosition)->Args({4096})->Args({65536});

// Position -> Position + Velocity, i.e. a full archetype migration per entity.
static void BM_AddVelocityMigration(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    std::vector<Entity> entities;
    entities.reserve(count);
    fillPositionWorld(world, count, &entities);
    state.ResumeTiming();

    for (Entity e : entities) {
      world.addComponent<Velocity>(e, 1.0f, 2.0f, 3.0f);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_AddVelocityMigration)->Args({4096})->Args({65536});

// The reverse migration: Position + Velocity -> Position.
static void BM_RemoveVelocityMigration(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    std::vector<Entity> entities;
    entities.reserve(count);
    fillPositionVelocityWorld(world, count, &entities);
    state.ResumeTiming();

    for (Entity e : entities) {
      world.removeComponent<Velocity>(e);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_RemoveVelocityMigration)->Args({4096})->Args({65536});

// Destroying entities from the middle exercises swap-and-pop repeatedly,
// which is a different cost profile from destroying back-to-front.
static void BM_RemoveEntitiesRandomOrder(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    std::vector<Entity> entities;
    entities.reserve(count);
    fillPositionVelocityWorld(world, count, &entities);

    std::mt19937 rng(0x5EED);
    std::shuffle(entities.begin(), entities.end(), rng);
    state.ResumeTiming();

    for (Entity e : entities) {
      world.removeEntity(e);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_RemoveEntitiesRandomOrder)->Args({4096})->Args({65536});

// ------------------------------------------------------------
// Queries
// ------------------------------------------------------------

static void BM_EachPosition(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  World world;
  fillPositionWorld(world, count);

  for (auto _ : state) {
    world.each<Position>([](Entity, Position &p) {
      p.x += 1.0f;
      p.y += 1.0f;
      p.z += 1.0f;
    });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(count) *
                          sizeof(Position));
}

BENCHMARK(BM_EachPosition)->Args({4096})->Args({65536})->Args({1048576});

static void BM_EachPositionVelocity(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  World world;
  fillPositionVelocityWorld(world, count);

  for (auto _ : state) {
    world.each<Position, Velocity>([](Entity, Position &p, Velocity &v) {
      p.x += v.x;
      p.y += v.y;
      p.z += v.z;
    });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(count) *
                          (sizeof(Position) + sizeof(Velocity)));
}

BENCHMARK(BM_EachPositionVelocity)
    ->Args({4096})
    ->Args({65536})
    ->Args({1048576});

static void BM_EachPositionManyArchetypes(benchmark::State &state) {
  const std::size_t perArchetype = static_cast<std::size_t>(state.range(0));

  World world;
  fillManyArchetypesWorld(world, perArchetype);

  constexpr std::size_t matchingArchetypes = 4;

  for (auto _ : state) {
    world.each<Position>([](Entity, Position &p) { p.x += 1.0f; });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(perArchetype * matchingArchetypes));
}

BENCHMARK(BM_EachPositionManyArchetypes)
    ->Args({64})
    ->Args({4096})
    ->Args({65536});

static void BM_EachPositionVelocityManyArchetypes(benchmark::State &state) {
  const std::size_t perArchetype = static_cast<std::size_t>(state.range(0));

  World world;
  fillManyArchetypesWorld(world, perArchetype);

  constexpr std::size_t matchingArchetypes = 2;

  for (auto _ : state) {
    world.each<Position, Velocity>([](Entity, Position &p, Velocity &v) {
      p.x += v.x;
      p.y += v.y;
      p.z += v.z;
    });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(perArchetype * matchingArchetypes));
}

BENCHMARK(BM_EachPositionVelocityManyArchetypes)
    ->Args({64})
    ->Args({4096})
    ->Args({65536});

// Isolates archetype *filtering* cost: many archetypes exist, but only a
// tiny fraction match. Ideally this should scale with matches, not with
// total archetype count.
static void BM_EachSparseMatch(benchmark::State &state) {
  const std::size_t archetypeCount = static_cast<std::size_t>(state.range(0));

  World world;

  // Many distinct archetypes via a unique Tag-like component set, none of
  // which have Velocity...
  for (std::size_t i = 0; i < archetypeCount; ++i) {
    Entity e = world.createEntity();
    world.addComponent<Position>(e, 1.0f, 2.0f, 3.0f);
    if (i % 2)
      world.addComponent<Health>(e, 100);
    if (i % 3)
      world.addComponent<Tag>(e, static_cast<std::uint32_t>(i));
  }

  // ...plus one small archetype that does.
  constexpr std::size_t matching = 64;
  for (std::size_t i = 0; i < matching; ++i) {
    Entity e = world.createEntity();
    world.addComponent<Position>(e, 1.0f, 2.0f, 3.0f);
    world.addComponent<Velocity>(e, 1.0f, 2.0f, 3.0f);
  }

  for (auto _ : state) {
    world.each<Position, Velocity>([](Entity, Position &p, Velocity &v) {
      p.x += v.x;
    });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(matching));
}

BENCHMARK(BM_EachSparseMatch)->Args({64})->Args({1024})->Args({8192});

// ------------------------------------------------------------
// Deferred iteration
// ------------------------------------------------------------

// Baseline: what does eachDeferred cost when NO commands are queued?
// Compare directly against BM_EachPositionVelocity to isolate the pure
// overhead of the WorldCtxt + empty flush.
static void BM_EachDeferredNoCommands(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  World world;
  fillPositionVelocityWorld(world, count);

  for (auto _ : state) {
    eachDeferred<Position, Velocity>(
        world, [](WorldCtxt &, Entity, Position &p, Velocity &v) {
          p.x += v.x;
          p.y += v.y;
          p.z += v.z;
        });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_EachDeferredNoCommands)
    ->Args({4096})
    ->Args({65536})
    ->Args({1048576});

// Every visited entity queues a component add, so this measures record +
// flush + archetype migration for the whole batch.
static void BM_EachDeferredAddComponent(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    fillPositionVelocityWorld(world, count);
    state.ResumeTiming();

    eachDeferred<Position, Velocity>(
        world, [](WorldCtxt &ctx, Entity e, Position &, Velocity &) {
          ctx.addComponent<Health>(e, 100);
        });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_EachDeferredAddComponent)->Args({4096})->Args({65536});

static void BM_EachDeferredRemoveComponent(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    fillPositionVelocityWorld(world, count);
    state.ResumeTiming();

    eachDeferred<Position, Velocity>(
        world, [](WorldCtxt &ctx, Entity e, Position &, Velocity &) {
          ctx.removeComponent<Velocity>(e);
        });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_EachDeferredRemoveComponent)->Args({4096})->Args({65536});

// Mass destruction — the "explosion killed everything" case.
static void BM_EachDeferredDestroyAll(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    fillPositionVelocityWorld(world, count);
    state.ResumeTiming();

    eachDeferred<Position, Velocity>(
        world, [](WorldCtxt &ctx, Entity e, Position &, Velocity &) {
          ctx.destroyEntity(e);
        });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_EachDeferredDestroyAll)->Args({4096})->Args({65536});

// Each visited entity spawns one new fully-configured entity.
static void BM_EachDeferredSpawn(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    state.PauseTiming();
    World world;
    fillPositionVelocityWorld(world, count);
    state.ResumeTiming();

    eachDeferred<Position, Velocity>(
        world, [&world](WorldCtxt &ctx, Entity, Position &p, Velocity &) {
          Entity spawned = ctx.createEntity(world);
          ctx.addComponent<Position>(spawned, p.x, p.y, p.z);
          ctx.addComponent<Velocity>(spawned, 1.0f, 2.0f, 3.0f);
        });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_EachDeferredSpawn)->Args({4096})->Args({65536});

// Self-balancing steady state: a fixed fraction dies and an equal number
// spawns each pass, so entity count stays flat and this can run across
// iterations without PauseTiming. Closest thing here to a real frame.
static void BM_DeferredSteadyStateChurn(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));
  const std::size_t churnEvery = 16; // ~6% of entities recycled per pass

  World world;
  fillPositionVelocityWorld(world, count);

  std::size_t tick = 0;

  for (auto _ : state) {
    std::size_t index = 0;
    const std::size_t offset = tick++ % churnEvery;

    eachDeferred<Position, Velocity>(
        world, [&](WorldCtxt &ctx, Entity e, Position &p, Velocity &v) {
          p.x += v.x;
          p.y += v.y;
          p.z += v.z;

          if (index % churnEvery == offset) {
            ctx.destroyEntity(e);
            Entity spawned = ctx.createEntity(world);
            ctx.addComponent<Position>(spawned, 1.0f, 2.0f, 3.0f);
            ctx.addComponent<Velocity>(spawned, 1.0f, 2.0f, 3.0f);
          }
          ++index;
        });
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
}

BENCHMARK(BM_DeferredSteadyStateChurn)->Args({4096})->Args({65536});

// ------------------------------------------------------------
// Baseline: raw arrays
//
// Approximate lower bound for the component-processing work itself,
// with no ECS indirection at all.
// ------------------------------------------------------------

static void BM_RawArraysPositionVelocity(benchmark::State &state) {
  const std::size_t count = static_cast<std::size_t>(state.range(0));

  std::vector<Position> positions(count, Position{1.0f, 2.0f, 3.0f});
  std::vector<Velocity> velocities(count, Velocity{1.0f, 2.0f, 3.0f});

  for (auto _ : state) {
    for (std::size_t i = 0; i < count; ++i) {
      positions[i].x += velocities[i].x;
      positions[i].y += velocities[i].y;
      positions[i].z += velocities[i].z;
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(count));
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(count) *
                          (sizeof(Position) + sizeof(Velocity)));
}

BENCHMARK(BM_RawArraysPositionVelocity)
    ->Args({4096})
    ->Args({65536})
    ->Args({1048576});

// ------------------------------------------------------------

BENCHMARK_MAIN();
