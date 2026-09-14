# Archetype ECS

A performance-oriented, archetype-based Entity Component System written in C++20. The project explores the data-oriented storage, memory management, and query mechanisms behind ECS frameworks such as Flecs, EnTT, and Bevy ECS, with an emphasis on building the core systems from scratch.

## Features

* **Archetype-based storage:** Entities sharing the same component set are stored in contiguous, columnar arrays for efficient iteration and cache locality.
* **Dynamic archetype management:** Supports adding and removing components by migrating entities between archetypes while maintaining densely packed storage.
* **Type-safe queries:** Variadic queries such as `world.each<Position, Velocity>(...)` iterate across all matching archetypes.
* **Deferred world mutations:** A command-buffer system allows entities to be created, destroyed, or modified safely during query execution.

## Design Highlights

The project focuses on the practical challenges of implementing an archetype ECS: managing component layouts, allocating and maintaining contiguous column storage, migrating entities between archetypes, and keeping entity references consistent as densely packed arrays change.

A second major challenge is supporting **world mutations during iteration**. Adding or removing components changes an entity's archetype, while destruction typically uses swap-and-pop to preserve dense storage. A deferred command buffer separates these mutations from query execution, applying them after iteration completes. The current implementation groups deferred operations by component type to reduce per-command overhead and type-erasure costs.

## Example

```cpp
World world;

Entity e = world.createEntity();
world.addComponent<Position>(e, 0.0f, 0.0f, 0.0f);
world.addComponent<Velocity>(e, 1.0f, 0.0f, 0.0f);
world.addComponent<Health>(e, 100);

// Type-safe, archetype-based query
world.each<Position, Velocity>([](Entity, Position& p, Velocity& v) {
    p.x += v.x;
    p.y += v.y;
    p.z += v.z;
});

// Deferred mutation during iteration
eachDeferred<Position, Health>(world,
    [&](WorldCtxt& ctx, Entity entity, Position&, Health& h) {
        if (h.hearts <= 0) {
            ctx.destroyEntity(entity);
        }
    });
```

## Building

The project uses CMake presets for **Debug**, **Release**, **Benchmark**, and **Profiling**

For example:
```sh
cmake --preset debug
cmake --build --preset debug
```

## Testing & Performance

* **Testing:** [Catch2](https://github.com/catchorg/Catch2)
* **Benchmarking:** [Google Benchmark](https://github.com/google/benchmark)
* **Profiling:** [Tracy](https://github.com/wolfpld/tracy)
