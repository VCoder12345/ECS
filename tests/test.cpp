#include "catch2/matchers/catch_matchers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ecs/Archetype.h>
#include <ecs/World.h>

#include <random>

using Catch::Matchers::WithinAbs;

struct Position {
  float x, y, z;
};

struct Velocity {
  float x, y, z;
};

struct Health {
  int hearts;
};

struct StringComponent {
  std::string value;

  StringComponent(std::string value) : value(std::move(value)) {}
};

struct LifetimeComponent {
  static inline int constructed = 0;
  static inline int destroyed = 0;
  static inline int moved = 0;

  int value;

  LifetimeComponent(int v) : value(v) { ++constructed; }

  LifetimeComponent(LifetimeComponent &&other) noexcept : value(other.value) {
    ++constructed;
    ++moved;
  }

  ~LifetimeComponent() { ++destroyed; }

  static void reset() {
    constructed = 0;
    destroyed = 0;
    moved = 0;
  }
};

TEST_CASE("Entities receive unique IDs", "[ecs]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  REQUIRE(e1 != e2);
  REQUIRE(e2 != e3);
  REQUIRE(e1 != e3);

  REQUIRE(e1 + 1 == e2);
  REQUIRE(e2 + 1 == e3);
}

TEST_CASE("Adding a component moves entity to correct archetype",
          "[ecs][component]") {
  World world;

  Entity e = world.createEntity();

  REQUIRE(world.getArchetypeForEntity(e).getMask().none());

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);

  auto &archetype = world.getArchetypeForEntity(e);

  REQUIRE(archetype.getMask().test(getComponentID<Position>()));
  REQUIRE_FALSE(archetype.getMask().test(getComponentID<Velocity>()));
  REQUIRE_FALSE(archetype.getMask().test(getComponentID<Health>()));
}

TEST_CASE("Component data is constructed correctly", "[ecs][component]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);

  Position &p = world.getComponent<Position>(e);

  REQUIRE_THAT(p.x, WithinAbs(1.0f, 0.001f));
  REQUIRE_THAT(p.y, WithinAbs(2.0f, 0.001f));
  REQUIRE_THAT(p.z, WithinAbs(3.0f, 0.001f));

  p.z = 4.0f;
  Position &p2 = world.getComponent<Position>(e);
  REQUIRE_THAT(p2.z, WithinAbs(4.0f, 0.001f));
}

TEST_CASE("Adding component preserves existing components",
          "[ecs][migration]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Health>(e, 100);

  Position &position = world.getComponent<Position>(e);
  Health &health = world.getComponent<Health>(e);

  REQUIRE_THAT(position.x, WithinAbs(1.0f, 0.001f));
  REQUIRE_THAT(position.y, WithinAbs(2.0f, 0.001f));
  REQUIRE_THAT(position.z, WithinAbs(3.0f, 0.001f));
  REQUIRE(health.hearts == 100);
}

TEST_CASE("Entity row index is preserved during migration",
          "[ecs][migration]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Position>(e2, 4.0f, 5.0f, 6.0f);

  world.addComponentToEntity<Health>(e1, 100);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 4.0f);
  REQUIRE(world.getComponent<Health>(e1).hearts == 100);
}

TEST_CASE("Multiple entities preserve their component data",
          "[ecs][entities]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);

  world.addComponentToEntity<Position>(e2, 4.0f, 5.0f, 6.0f);

  world.addComponentToEntity<Position>(e3, 7.0f, 8.0f, 9.0f);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e1).y == 2.0f);
  REQUIRE(world.getComponent<Position>(e1).z == 3.0f);

  REQUIRE(world.getComponent<Position>(e2).x == 4.0f);
  REQUIRE(world.getComponent<Position>(e2).y == 5.0f);
  REQUIRE(world.getComponent<Position>(e2).z == 6.0f);

  REQUIRE(world.getComponent<Position>(e3).x == 7.0f);
  REQUIRE(world.getComponent<Position>(e3).y == 8.0f);
  REQUIRE(world.getComponent<Position>(e3).z == 9.0f);
}

TEST_CASE("Archetype migration preserves other entities", "[ecs][migration]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e2, 2.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e3, 3.0f, 0.0f, 0.0f);

  // e1 is removed from the Position-only archetype.
  // e3 should be swapped into e1's old row.
  world.addComponentToEntity<Health>(e1, 100);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 2.0f);
  REQUIRE(world.getComponent<Position>(e3).x == 3.0f);

  REQUIRE(world.getComponent<Health>(e1).hearts == 100);
}

TEST_CASE("Repeated archetype migration preserves all components",
          "[ecs][migration]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e2, 2.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e3, 3.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Health>(e2, 200);
  world.addComponentToEntity<Health>(e1, 100);
  world.addComponentToEntity<Health>(e3, 300);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 2.0f);
  REQUIRE(world.getComponent<Position>(e3).x == 3.0f);

  REQUIRE(world.getComponent<Health>(e1).hearts == 100);
  REQUIRE(world.getComponent<Health>(e2).hearts == 200);
  REQUIRE(world.getComponent<Health>(e3).hearts == 300);
}

TEST_CASE("Column grows without losing components", "[ecs][column]") {
  World world;

  std::vector<Entity> entities;

  for (int i = 0; i < 100; ++i) {
    Entity e = world.createEntity();
    entities.push_back(e);

    world.addComponentToEntity<Position>(e, static_cast<float>(i),
                                         static_cast<float>(i + 1),
                                         static_cast<float>(i + 2));
  }

  for (int i = 0; i < 100; ++i) {
    Position &p = world.getComponent<Position>(entities[i]);

    REQUIRE(p.x == static_cast<float>(i));
    REQUIRE(p.y == static_cast<float>(i + 1));
    REQUIRE(p.z == static_cast<float>(i + 2));
  }
}

TEST_CASE("Non-trivial components survive column growth", "[ecs][lifetime]") {
  World world;

  std::vector<Entity> entities;

  for (int i = 0; i < 100; ++i) {
    Entity e = world.createEntity();
    entities.push_back(e);

    world.addComponentToEntity<StringComponent>(e, "component_" +
                                                       std::to_string(i));
  }

  for (int i = 0; i < 100; ++i) {
    StringComponent &component =
        world.getComponent<StringComponent>(entities[i]);

    REQUIRE(component.value == "component_" + std::to_string(i));
  }
}

TEST_CASE("Components are eventually destroyed",
          "[ecs][lifetime]")
{
    LifetimeComponent::reset();

    {
        World world;

        for (int i = 0; i < 100; ++i) {
            Entity e = world.createEntity();

            world.addComponentToEntity<LifetimeComponent>(e, i);
        }
    }

    REQUIRE(
        LifetimeComponent::constructed ==
        LifetimeComponent::destroyed);
}

TEST_CASE("Entities with same components share archetype",
          "[ecs][archetype]")
{
    World world;

    Entity e1 = world.createEntity();
    Entity e2 = world.createEntity();

    world.addComponentToEntity<Position>(
        e1, 1.0f, 2.0f, 3.0f);

    world.addComponentToEntity<Position>(
        e2, 4.0f, 5.0f, 6.0f);

    REQUIRE(
        &world.getArchetypeForEntity(e1) ==
        &world.getArchetypeForEntity(e2));
}

TEST_CASE("Adding multiple components produces correct archetype",
          "[ecs][archetype]")
{
    World world;

    Entity e = world.createEntity();

    world.addComponentToEntity<Position>(
        e, 1.0f, 2.0f, 3.0f);

    world.addComponentToEntity<Velocity>(
        e, 4.0f, 5.0f, 6.0f);

    world.addComponentToEntity<Health>(
        e, 100);

    const auto& mask =
        world.getArchetypeForEntity(e).getMask();

    REQUIRE(mask.test(getComponentID<Position>()));
    REQUIRE(mask.test(getComponentID<Velocity>()));
    REQUIRE(mask.test(getComponentID<Health>()));

    REQUIRE(world.getComponent<Position>(e).x == 1.0f);
    REQUIRE(world.getComponent<Position>(e).y == 2.0f);
    REQUIRE(world.getComponent<Position>(e).z == 3.0f);

    REQUIRE(world.getComponent<Velocity>(e).x == 4.0f);
    REQUIRE(world.getComponent<Velocity>(e).y == 5.0f);
    REQUIRE(world.getComponent<Velocity>(e).z == 6.0f);

    REQUIRE(world.getComponent<Health>(e).hearts == 100);
}

TEST_CASE("Randomized ECS migration stress test", "[ecs][stress]")
{
    constexpr int entityCount = 500;
    constexpr int operations = 5000;

    std::mt19937 rng(0x12345678);

    World world;

    std::vector<Entity> entities;
    entities.reserve(entityCount);

    for (int i = 0; i < entityCount; ++i) {
        entities.push_back(world.createEntity());
    }

    struct Expected {
        bool hasPosition = false;
        bool hasVelocity = false;
        bool hasHealth = false;

        Position position{};
        Velocity velocity{};
        Health health{};
    };

    std::unordered_map<Entity, Expected> expected;

    for (Entity e : entities) {
        expected.emplace(e, Expected{});
    }

    for (int operation = 0; operation < operations; ++operation) {
        // Pick a random entity.
        Entity e = entities[rng() % entities.size()];

        // Pick a random component.
        int component = rng() % 3;

        Expected& model = expected.at(e);

        switch (component) {
        case 0:
            if (!model.hasPosition) {
                const float value =
                    static_cast<float>(operation);

                world.addComponentToEntity<Position>(
                    e,
                    value,
                    value + 1.0f,
                    value + 2.0f);

                model.hasPosition = true;
                model.position = {
                    value,
                    value + 1.0f,
                    value + 2.0f
                };
            }
            break;

        case 1:
            if (!model.hasVelocity) {
                const float value =
                    static_cast<float>(operation) * 0.5f;

                world.addComponentToEntity<Velocity>(
                    e,
                    value,
                    value + 10.0f,
                    value + 20.0f);

                model.hasVelocity = true;
                model.velocity = {
                    value,
                    value + 10.0f,
                    value + 20.0f
                };
            }
            break;

        case 2:
            if (!model.hasHealth) {
                const int value = operation + 1000;

                world.addComponentToEntity<Health>(
                    e,
                    value);

                model.hasHealth = true;
                model.health = {value};
            }
            break;
        }

        // Validate every entity after every operation.
        for (Entity checkEntity : entities) {
            Expected& expectedValue =
                expected.at(checkEntity);

            if (expectedValue.hasPosition) {
                Position& actual =
                    world.getComponent<Position>(checkEntity);

                REQUIRE(actual.x == expectedValue.position.x);
                REQUIRE(actual.y == expectedValue.position.y);
                REQUIRE(actual.z == expectedValue.position.z);
            }

            if (expectedValue.hasVelocity) {
                Velocity& actual =
                    world.getComponent<Velocity>(checkEntity);

                REQUIRE(actual.x == expectedValue.velocity.x);
                REQUIRE(actual.y == expectedValue.velocity.y);
                REQUIRE(actual.z == expectedValue.velocity.z);
            }

            if (expectedValue.hasHealth) {
                Health& actual =
                    world.getComponent<Health>(checkEntity);

                REQUIRE(actual.hearts ==
                        expectedValue.health.hearts);
            }
        }
    }
}
