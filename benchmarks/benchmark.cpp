#include <benchmark/benchmark.h>

#include <ecs/World.h>

#include <cstddef>
#include <cstdint>
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

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static World makePositionWorld(std::size_t count) {
  World world;

  for (std::size_t i = 0; i < count; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e,
        1.0f,
        2.0f,
        3.0f
    );
  }

  return world;
}

static World makePositionVelocityWorld(std::size_t count) {
  World world;

  for (std::size_t i = 0; i < count; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e,
        1.0f,
        2.0f,
        3.0f
    );

    world.addComponentToEntity<Velocity>(
        e,
        1.0f,
        2.0f,
        3.0f
    );
  }

  return world;
}

// Creates several different archetypes.
//
// Some match Position.
// Some don't.
//
// This is useful for measuring the cost of archetype filtering.
static World makeManyArchetypesWorld(std::size_t entitiesPerArchetype) {
  World world;

  // Position
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e, 1.0f, 2.0f, 3.0f
    );
  }

  // Velocity
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Velocity>(
        e, 1.0f, 2.0f, 3.0f
    );
  }

  // Health
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Health>(
        e, 100
    );
  }

  // Position + Velocity
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e, 1.0f, 2.0f, 3.0f
    );

    world.addComponentToEntity<Velocity>(
        e, 1.0f, 2.0f, 3.0f
    );
  }

  // Position + Health
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e, 1.0f, 2.0f, 3.0f
    );

    world.addComponentToEntity<Health>(
        e, 100
    );
  }

  // Velocity + Health
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Velocity>(
        e, 1.0f, 2.0f, 3.0f
    );

    world.addComponentToEntity<Health>(
        e, 100
    );
  }

  // Position + Velocity + Health
  for (std::size_t i = 0; i < entitiesPerArchetype; ++i) {
    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e, 1.0f, 2.0f, 3.0f
    );

    world.addComponentToEntity<Velocity>(
        e, 1.0f, 2.0f, 3.0f
    );

    world.addComponentToEntity<Health>(
        e, 100
    );
  }

  return world;
}

// ------------------------------------------------------------
// Entity creation
// ------------------------------------------------------------

static void BM_CreateEntities(benchmark::State& state) {
  const std::size_t count =
      static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    World world;

    for (std::size_t i = 0; i < count; ++i) {
      benchmark::DoNotOptimize(world.createEntity());
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(count)
  );
}

BENCHMARK(BM_CreateEntities)
    ->RangeMultiplier(4)
    ->Range(1024, 1 << 18);


// ------------------------------------------------------------
// Add components
// ------------------------------------------------------------

static void BM_AddPosition(benchmark::State& state) {
  const std::size_t count =
      static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    World world;

    for (std::size_t i = 0; i < count; ++i) {
      Entity e = world.createEntity();

      world.addComponentToEntity<Position>(
          e, 1.0f, 2.0f, 3.0f
      );
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(count)
  );
}

BENCHMARK(BM_AddPosition)
    ->RangeMultiplier(4)
    ->Range(1024, 1 << 16);


// ------------------------------------------------------------
// Archetype migration
//
// Every entity starts as:
//
//     Position
//
// and then moves to:
//
//     Position + Velocity
// ------------------------------------------------------------

static void BM_AddVelocityMigration(
    benchmark::State& state) {

  const std::size_t count =
      static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    World world;

    std::vector<Entity> entities;
    entities.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
      Entity e = world.createEntity();

      world.addComponentToEntity<Position>(
          e, 1.0f, 2.0f, 3.0f
      );

      entities.push_back(e);
    }

    for (Entity e : entities) {
      world.addComponentToEntity<Velocity>(
          e, 1.0f, 2.0f, 3.0f
      );
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(count)
  );
}

BENCHMARK(BM_AddVelocityMigration)
    ->RangeMultiplier(4)
    ->Range(1024, 1 << 16);


// ------------------------------------------------------------
// Position query
// ------------------------------------------------------------

static void BM_EachPosition(benchmark::State& state) {
  const std::size_t count =
      static_cast<std::size_t>(state.range(0));

  World world = makePositionWorld(count);

  for (auto _ : state) {
    world.each<Position>(
        [](Entity, Position& p) {
          p.x += 1.0f;
          p.y += 1.0f;
          p.z += 1.0f;
        }
    );
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(count)
  );
}

BENCHMARK(BM_EachPosition)
    ->RangeMultiplier(4)
    ->Range(1024, 1 << 20);


// ------------------------------------------------------------
// Position + Velocity query
// ------------------------------------------------------------

static void BM_EachPositionVelocity(
    benchmark::State& state) {

  const std::size_t count =
      static_cast<std::size_t>(state.range(0));

  World world = makePositionVelocityWorld(count);

  for (auto _ : state) {
    world.each<Position, Velocity>(
        [](Entity, Position& p, Velocity& v) {
          p.x += v.x;
          p.y += v.y;
          p.z += v.z;
        }
    );
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(count)
  );
}

BENCHMARK(BM_EachPositionVelocity)
    ->RangeMultiplier(4)
    ->Range(1024, 1 << 20);


// ------------------------------------------------------------
// Position query with many archetypes
// ------------------------------------------------------------

static void BM_EachPositionManyArchetypes(
    benchmark::State& state) {

  const std::size_t entitiesPerArchetype =
      static_cast<std::size_t>(state.range(0));

  World world =
      makeManyArchetypesWorld(entitiesPerArchetype);

  // There are four matching archetypes:
  //
  // Position
  // Position + Velocity
  // Position + Health
  // Position + Velocity + Health
  //
  // This benchmark therefore exercises archetype filtering.

  constexpr std::size_t matchingArchetypes = 4;

  for (auto _ : state) {
    world.each<Position>(
        [](Entity, Position& p) {
          p.x += 1.0f;
        }
    );
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(
          entitiesPerArchetype * matchingArchetypes
      )
  );
}

BENCHMARK(BM_EachPositionManyArchetypes)
    ->RangeMultiplier(4)
    ->Range(64, 1 << 16);


// ------------------------------------------------------------
// Position + Velocity with many archetypes
// ------------------------------------------------------------

static void BM_EachPositionVelocityManyArchetypes(
    benchmark::State& state) {

  const std::size_t entitiesPerArchetype =
      static_cast<std::size_t>(state.range(0));

  World world =
      makeManyArchetypesWorld(entitiesPerArchetype);

  // Matching archetypes:
  //
  // Position + Velocity
  // Position + Velocity + Health
  //
  constexpr std::size_t matchingArchetypes = 2;

  for (auto _ : state) {
    world.each<Position, Velocity>(
        [](Entity, Position& p, Velocity& v) {
          p.x += v.x;
          p.y += v.y;
          p.z += v.z;
        }
    );
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(
          entitiesPerArchetype * matchingArchetypes
      )
  );
}

BENCHMARK(BM_EachPositionVelocityManyArchetypes)
    ->RangeMultiplier(4)
    ->Range(64, 1 << 16);


// ------------------------------------------------------------
// Baseline: raw arrays
//
// This gives you an approximate lower bound for the actual
// component-processing work.
// ------------------------------------------------------------

static void BM_RawArraysPositionVelocity(
    benchmark::State& state) {

  const std::size_t count =
      static_cast<std::size_t>(state.range(0));

  std::vector<Position> positions(count);
  std::vector<Velocity> velocities(count);

  for (std::size_t i = 0; i < count; ++i) {
    positions[i] = {1.0f, 2.0f, 3.0f};
    velocities[i] = {1.0f, 2.0f, 3.0f};
  }

  for (auto _ : state) {
    for (std::size_t i = 0; i < count; ++i) {
      positions[i].x += velocities[i].x;
      positions[i].y += velocities[i].y;
      positions[i].z += velocities[i].z;
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(
      state.iterations() *
      static_cast<int64_t>(count)
  );
}

BENCHMARK(BM_RawArraysPositionVelocity)
    ->RangeMultiplier(4)
    ->Range(1024, 1 << 20);


// ------------------------------------------------------------

BENCHMARK_MAIN();
