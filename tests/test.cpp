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

TEST_CASE("Components are eventually destroyed", "[ecs][lifetime]") {
  LifetimeComponent::reset();

  {
    World world;

    for (int i = 0; i < 100; ++i) {
      Entity e = world.createEntity();

      world.addComponentToEntity<LifetimeComponent>(e, i);
    }
  }

  REQUIRE(LifetimeComponent::constructed == LifetimeComponent::destroyed);
}

TEST_CASE("Entities with same components share archetype", "[ecs][archetype]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);

  world.addComponentToEntity<Position>(e2, 4.0f, 5.0f, 6.0f);

  REQUIRE(&world.getArchetypeForEntity(e1) == &world.getArchetypeForEntity(e2));
}

TEST_CASE("Adding multiple components produces correct archetype",
          "[ecs][archetype]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);

  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);

  world.addComponentToEntity<Health>(e, 100);

  const auto &mask = world.getArchetypeForEntity(e).getMask();

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

TEST_CASE("Randomized ECS stress test", "[ecs][stress]") {
  constexpr int entityCount = 500;
  constexpr int operations = 1000;

  std::mt19937 rng(0x12345679);

  World world;

  std::vector<Entity> entities;
  entities.reserve(entityCount);

  for (int i = 0; i < entityCount; ++i) {
    entities.push_back(world.createEntity());
  }

  struct ComponentData {
    bool hasPosition = false;
    bool hasVelocity = false;
    bool hasHealth = false;

    Position position;
    Velocity velocity;
    Health health;
  };

  std::vector<ComponentData> componentData(entityCount);

  for (int op = 0; op < operations; ++op) {
    int eIndex = rng() % entityCount;
    Entity e = entities[eIndex];
    ComponentData &data = componentData[eIndex];

    int action = rng() % 3;
    switch (action) {
    case 0: // Add Position
      if (!data.hasPosition) {
        world.addComponentToEntity<Position>(e, op + 1.0f, op + 2.0f,
                                             op + 3.0f);
        data.hasPosition = true;
        data.position = {op + 1.0f, op + 2.0f, op + 3.0f};
      }
      break;
    case 1: // Add Velocity
      if (!data.hasVelocity) {
        world.addComponentToEntity<Velocity>(e, op + 10.0f, op + 20.0f,
                                             op + 30.0f);
        data.hasVelocity = true;
        data.velocity = {op + 10.0f, op + 20.0f, op + 30.0f};
      }
      break;
    case 2: // Add Health
      if (!data.hasHealth) {
        world.addComponentToEntity<Health>(e, op + 100);
        data.hasHealth = true;
        data.health = {op + 100};
      }
      break;
    }
  }

  for (int i = 0; i < entityCount; ++i) {
    Entity e = entities[i];
    ComponentData &data = componentData[i];
    if (data.hasPosition) {
      Position &p = world.getComponent<Position>(e);
      REQUIRE(p.x == data.position.x);
      REQUIRE(p.y == data.position.y);
      REQUIRE(p.z == data.position.z);
    }
    if (data.hasVelocity) {
      Velocity &v = world.getComponent<Velocity>(e);
      REQUIRE(v.x == data.velocity.x);
      REQUIRE(v.y == data.velocity.y);
      REQUIRE(v.z == data.velocity.z);
    }
    if (data.hasHealth) {
      Health &h = world.getComponent<Health>(e);
      REQUIRE(h.hearts == data.health.hearts);
    }
  }
}

TEST_CASE("Remove components from entities with multiple components",
          "[ecs][remove]") {
  World world;
  Entity e = world.createEntity();
  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Health>(e, 100);
  REQUIRE(world.getArchetypeForEntity(e).getMask().test(
      getComponentID<Position>()));
  REQUIRE(world.getArchetypeForEntity(e).getMask().test(
      getComponentID<Velocity>()));
  REQUIRE(
      world.getArchetypeForEntity(e).getMask().test(getComponentID<Health>()));
  world.removeComponentFromEntity<Velocity>(e);
  REQUIRE(world.getArchetypeForEntity(e).getMask().test(
      getComponentID<Position>()));
  REQUIRE_FALSE(world.getArchetypeForEntity(e).getMask().test(
      getComponentID<Velocity>()));
  REQUIRE(
      world.getArchetypeForEntity(e).getMask().test(getComponentID<Health>()));
}

TEST_CASE("Remove component preserves remaining data", "[ecs][remove][data]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);

  world.removeComponentFromEntity<Position>(e);

  auto &at = world.getArchetypeForEntity(e);

  REQUIRE_FALSE(at.getMask().test(getComponentID<Position>()));
  REQUIRE(at.getMask().test(getComponentID<Velocity>()));

  Velocity &v = world.getComponent<Velocity>(e);

  REQUIRE(v.x == 4.0f);
  REQUIRE(v.y == 5.0f);
  REQUIRE(v.z == 6.0f);
}

TEST_CASE("Remove middle component preserves surrounding data",
          "[ecs][remove][middle]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Health>(e, 100);

  world.removeComponentFromEntity<Velocity>(e);

  auto &at = world.getArchetypeForEntity(e);

  REQUIRE(at.getMask().test(getComponentID<Position>()));
  REQUIRE_FALSE(at.getMask().test(getComponentID<Velocity>()));
  REQUIRE(at.getMask().test(getComponentID<Health>()));

  Position &p = world.getComponent<Position>(e);
  Health &h = world.getComponent<Health>(e);

  REQUIRE(p.x == 1.0f);
  REQUIRE(p.y == 2.0f);
  REQUIRE(p.z == 3.0f);
  REQUIRE(h.hearts == 100);
}

TEST_CASE("Remove first component preserves remaining data",
          "[ecs][remove][first]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Health>(e, 100);

  world.removeComponentFromEntity<Position>(e);

  auto &at = world.getArchetypeForEntity(e);

  REQUIRE_FALSE(at.getMask().test(getComponentID<Position>()));
  REQUIRE(at.getMask().test(getComponentID<Velocity>()));
  REQUIRE(at.getMask().test(getComponentID<Health>()));

  Velocity &v = world.getComponent<Velocity>(e);
  Health &h = world.getComponent<Health>(e);

  REQUIRE(v.x == 4.0f);
  REQUIRE(v.y == 5.0f);
  REQUIRE(v.z == 6.0f);
  REQUIRE(h.hearts == 100);
}

TEST_CASE("Remove last component preserves remaining data",
          "[ecs][remove][last]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Health>(e, 100);

  world.removeComponentFromEntity<Health>(e);

  auto &at = world.getArchetypeForEntity(e);

  REQUIRE(at.getMask().test(getComponentID<Position>()));
  REQUIRE(at.getMask().test(getComponentID<Velocity>()));
  REQUIRE_FALSE(at.getMask().test(getComponentID<Health>()));

  Position &p = world.getComponent<Position>(e);
  Velocity &v = world.getComponent<Velocity>(e);

  REQUIRE(p.x == 1.0f);
  REQUIRE(p.y == 2.0f);
  REQUIRE(p.z == 3.0f);

  REQUIRE(v.x == 4.0f);
  REQUIRE(v.y == 5.0f);
  REQUIRE(v.z == 6.0f);
}

TEST_CASE("Remove component from middle entity preserves other entities",
          "[ecs][remove][swap-pop]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 1.0f, 1.0f);
  world.addComponentToEntity<Position>(e2, 2.0f, 2.0f, 2.0f);
  world.addComponentToEntity<Position>(e3, 3.0f, 3.0f, 3.0f);

  world.addComponentToEntity<Velocity>(e1, 10.0f, 10.0f, 10.0f);
  world.addComponentToEntity<Velocity>(e2, 20.0f, 20.0f, 20.0f);
  world.addComponentToEntity<Velocity>(e3, 30.0f, 30.0f, 30.0f);

  // e2 is in the middle, so removing its component exercises swap-and-pop.
  world.removeComponentFromEntity<Velocity>(e2);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 2.0f);
  REQUIRE(world.getComponent<Position>(e3).x == 3.0f);

  REQUIRE(world.getComponent<Velocity>(e1).x == 10.0f);
  REQUIRE(world.getComponent<Velocity>(e3).x == 30.0f);
}

TEST_CASE("Remove component from last entity",
          "[ecs][remove][swap-pop][last]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, static_cast<float>(e1), 2.0f, 3.0f);
  world.addComponentToEntity<Position>(e2, static_cast<float>(e2), 2.0f, 3.0f);
  world.addComponentToEntity<Position>(e3, static_cast<float>(e3), 2.0f, 3.0f);

  world.addComponentToEntity<Velocity>(e1, static_cast<float>(e1) + 10.0f,
                                       20.0f, 30.0f);
  world.addComponentToEntity<Velocity>(e2, static_cast<float>(e2) + 10.0f,
                                       20.0f, 30.0f);
  world.addComponentToEntity<Velocity>(e3, static_cast<float>(e3) + 10.0f,
                                       20.0f, 30.0f);

  // e3 is last, so this exercises the non-swap branch.
  world.removeComponentFromEntity<Velocity>(e3);

  REQUIRE(world.getComponent<Position>(e1).x == static_cast<float>(e1));
  REQUIRE(world.getComponent<Position>(e2).x == static_cast<float>(e2));
  REQUIRE(world.getComponent<Position>(e3).x == static_cast<float>(e3));

  REQUIRE(world.getComponent<Velocity>(e1).x == static_cast<float>(e1) + 10.0f);
  REQUIRE(world.getComponent<Velocity>(e2).x == static_cast<float>(e2) + 10.0f);
}

TEST_CASE("Remove all components from entity", "[ecs][remove][empty]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Health>(e, 100);

  world.removeComponentFromEntity<Position>(e);
  world.removeComponentFromEntity<Velocity>(e);
  world.removeComponentFromEntity<Health>(e);

  auto &at = world.getArchetypeForEntity(e);

  REQUIRE(at.getMask() == emptyCompMask());
  REQUIRE(at.getEntities().size() == 1);
  REQUIRE(at.getEntities()[0] == e);
}

TEST_CASE("Remove and re-add component", "[ecs][remove][readd]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);

  world.removeComponentFromEntity<Position>(e);

  world.addComponentToEntity<Position>(e, 10.0f, 20.0f, 30.0f);

  Position &p = world.getComponent<Position>(e);
  Velocity &v = world.getComponent<Velocity>(e);

  REQUIRE(p.x == 10.0f);
  REQUIRE(p.y == 20.0f);
  REQUIRE(p.z == 30.0f);

  REQUIRE(v.x == 4.0f);
  REQUIRE(v.y == 5.0f);
  REQUIRE(v.z == 6.0f);
}

TEST_CASE("Remove component does not affect other archetypes",
          "[ecs][remove][isolation]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e1, 4.0f, 5.0f, 6.0f);

  world.addComponentToEntity<Position>(e2, 10.0f, 20.0f, 30.0f);
  world.addComponentToEntity<Health>(e2, 100);

  world.removeComponentFromEntity<Velocity>(e1);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e1).y == 2.0f);
  REQUIRE(world.getComponent<Position>(e1).z == 3.0f);

  REQUIRE(world.getComponent<Position>(e2).x == 10.0f);
  REQUIRE(world.getComponent<Position>(e2).y == 20.0f);
  REQUIRE(world.getComponent<Position>(e2).z == 30.0f);

  REQUIRE(world.getComponent<Health>(e2).hearts == 100);
}

TEST_CASE("Different component removal orders", "[ecs][remove][orders]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  for (Entity e : {e1, e2}) {
    world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
    world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);
    world.addComponentToEntity<Health>(e, 100);
  }

  world.removeComponentFromEntity<Position>(e1);
  world.removeComponentFromEntity<Health>(e2);

  REQUIRE(world.getArchetypeForEntity(e1).getMask().test(
      getComponentID<Velocity>()));
  REQUIRE(
      world.getArchetypeForEntity(e1).getMask().test(getComponentID<Health>()));
  REQUIRE_FALSE(world.getArchetypeForEntity(e1).getMask().test(
      getComponentID<Position>()));

  REQUIRE(world.getArchetypeForEntity(e2).getMask().test(
      getComponentID<Position>()));
  REQUIRE(world.getArchetypeForEntity(e2).getMask().test(
      getComponentID<Velocity>()));
  REQUIRE_FALSE(
      world.getArchetypeForEntity(e2).getMask().test(getComponentID<Health>()));
}

TEST_CASE("Removal works with different component insertion orders",
          "[ecs][remove][archetype]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  // Creates Position -> Velocity -> Health.
  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e1, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Health>(e1, 100);

  // Reaches the same component mask through a different order.
  world.addComponentToEntity<Health>(e2, 200);
  world.addComponentToEntity<Velocity>(e2, 40.0f, 50.0f, 60.0f);
  world.addComponentToEntity<Position>(e2, 10.0f, 20.0f, 30.0f);

  world.removeComponentFromEntity<Velocity>(e1);
  world.removeComponentFromEntity<Position>(e2);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e1).y == 2.0f);
  REQUIRE(world.getComponent<Position>(e1).z == 3.0f);
  REQUIRE(world.getComponent<Health>(e1).hearts == 100);

  REQUIRE(world.getComponent<Velocity>(e2).x == 40.0f);
  REQUIRE(world.getComponent<Velocity>(e2).y == 50.0f);
  REQUIRE(world.getComponent<Velocity>(e2).z == 60.0f);
  REQUIRE(world.getComponent<Health>(e2).hearts == 200);
}

TEST_CASE("querying multiple entities", "[ecs][querying][multiple]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Position>(e2, 4.0f, 5.0f, 6.0f);
  world.addComponentToEntity<Position>(e3, 7.0f, 8.0f, 9.0f);

  int count = 0;

  world.each<Position>([&](Entity e, Position &p) {
    ++count;
    p.x += 10.0f;
  });

  REQUIRE(count == 3);

  REQUIRE(world.getComponent<Position>(e1).x == 11.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 14.0f);
  REQUIRE(world.getComponent<Position>(e3).x == 17.0f);
}

TEST_CASE("querying only matches entities with requested component",
          "[ecs][querying][filter]") {
  World world;

  Entity positionEntity = world.createEntity();
  Entity velocityEntity = world.createEntity();
  Entity emptyEntity = world.createEntity();

  world.addComponentToEntity<Position>(positionEntity, 1.0f, 2.0f, 3.0f);

  world.addComponentToEntity<Velocity>(velocityEntity, 4.0f, 5.0f, 6.0f);

  int count = 0;
  Entity found = 0;

  world.each<Position>([&](Entity e, Position &) {
    ++count;
    found = e;
  });

  REQUIRE(count == 1);
  REQUIRE(found == positionEntity);
}

TEST_CASE("query matches multiple archetypes", "[ecs][querying][archetypes]") {
  World world;

  Entity positionOnly = world.createEntity();
  Entity positionVelocity = world.createEntity();
  Entity positionHealth = world.createEntity();
  Entity allComponents = world.createEntity();

  world.addComponentToEntity<Position>(positionOnly, 1.0f, 1.0f, 1.0f);

  world.addComponentToEntity<Position>(positionVelocity, 2.0f, 2.0f, 2.0f);
  world.addComponentToEntity<Velocity>(positionVelocity, 3.0f, 3.0f, 3.0f);

  world.addComponentToEntity<Position>(positionHealth, 4.0f, 4.0f, 4.0f);
  world.addComponentToEntity<Health>(positionHealth, 40);

  world.addComponentToEntity<Position>(allComponents, 5.0f, 5.0f, 5.0f);
  world.addComponentToEntity<Velocity>(allComponents, 6.0f, 6.0f, 6.0f);
  world.addComponentToEntity<Health>(allComponents, 50);

  int count = 0;

  world.each<Position>([&](Entity, Position &p) {
    ++count;
    p.x += 100.0f;
  });

  REQUIRE(count == 4);

  REQUIRE(world.getComponent<Position>(positionOnly).x == 101.0f);
  REQUIRE(world.getComponent<Position>(positionVelocity).x == 102.0f);
  REQUIRE(world.getComponent<Position>(positionHealth).x == 104.0f);
  REQUIRE(world.getComponent<Position>(allComponents).x == 105.0f);
}

TEST_CASE("query multiple components", "[ecs][querying][multiple-components]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e1, 10.0f, 20.0f, 30.0f);

  world.addComponentToEntity<Position>(e2, 4.0f, 5.0f, 6.0f);

  world.addComponentToEntity<Velocity>(e3, 40.0f, 50.0f, 60.0f);

  int count = 0;

  world.each<Position, Velocity>([&](Entity e, Position &p, Velocity &v) {
    ++count;

    p.x += v.x;
    p.y += v.y;
    p.z += v.z;

    REQUIRE(e == e1);
  });

  REQUIRE(count == 1);

  REQUIRE(world.getComponent<Position>(e1).x == 11.0f);
  REQUIRE(world.getComponent<Position>(e1).y == 22.0f);
  REQUIRE(world.getComponent<Position>(e1).z == 33.0f);

  // These should not have been touched.
  REQUIRE(world.getComponent<Position>(e2).x == 4.0f);
  REQUIRE(world.getComponent<Position>(e2).y == 5.0f);
  REQUIRE(world.getComponent<Position>(e2).z == 6.0f);
}

TEST_CASE("query multiple components across multiple archetypes",
          "[ecs][querying][multiple-components][archetypes]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();

  // Position + Velocity
  world.addComponentToEntity<Position>(e1, 1.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(e1, 10.0f, 0.0f, 0.0f);

  // Position + Velocity + Health
  world.addComponentToEntity<Position>(e2, 2.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(e2, 20.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Health>(e2, 100);

  // Position only
  world.addComponentToEntity<Position>(e3, 3.0f, 0.0f, 0.0f);

  int count = 0;

  world.each<Position, Velocity>([&](Entity, Position &p, Velocity &v) {
    ++count;
    p.x += v.x;
  });

  REQUIRE(count == 2);

  REQUIRE(world.getComponent<Position>(e1).x == 11.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 22.0f);

  // Does not match Position + Velocity.
  REQUIRE(world.getComponent<Position>(e3).x == 3.0f);
}

TEST_CASE("query visits every matching entity exactly once",
          "[ecs][querying][count]") {
  World world;

  constexpr int entityCount = 100;

  std::vector<Entity> entities;

  for (int i = 0; i < entityCount; ++i) {
    Entity e = world.createEntity();
    entities.push_back(e);

    world.addComponentToEntity<Position>(e, static_cast<float>(i), 0.0f, 0.0f);
  }

  int count = 0;

  world.each<Position>([&](Entity, Position &) { ++count; });

  REQUIRE(count == entityCount);
}

TEST_CASE("query returns correct entity alongside component",
          "[ecs][querying][entity]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 10.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e2, 20.0f, 0.0f, 0.0f);

  world.each<Position>([&](Entity e, Position &p) {
    if (e == e1) {
      REQUIRE(p.x == 10.0f);
    } else if (e == e2) {
      REQUIRE(p.x == 20.0f);
    } else {
      FAIL("Query returned an unexpected entity");
    }
  });
}

TEST_CASE("query works after adding components and moving entities",
          "[ecs][querying][migration]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e2, 2.0f, 0.0f, 0.0f);

  // e1 moves from {Position} to {Position, Velocity}.
  world.addComponentToEntity<Velocity>(e1, 10.0f, 0.0f, 0.0f);

  int count = 0;

  world.each<Position>([&](Entity, Position &p) {
    ++count;
    p.x += 100.0f;
  });

  REQUIRE(count == 2);
  REQUIRE(world.getComponent<Position>(e1).x == 101.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 102.0f);
}

TEST_CASE("query works after removing components", "[ecs][querying][removal]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(e1, 10.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e2, 2.0f, 0.0f, 0.0f);

  // e1 becomes Position-only.
  world.removeComponentFromEntity<Velocity>(e1);

  int count = 0;

  world.each<Position>([&](Entity, Position &p) {
    ++count;
    p.x += 10.0f;
  });

  REQUIRE(count == 2);
  REQUIRE(world.getComponent<Position>(e1).x == 11.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 12.0f);
}

TEST_CASE("query multiple components after component removal",
          "[ecs][querying][removal][multiple-components]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();

  world.addComponentToEntity<Position>(e1, 1.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(e1, 10.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(e2, 2.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(e2, 20.0f, 0.0f, 0.0f);

  // e1 no longer matches Position + Velocity.
  world.removeComponentFromEntity<Velocity>(e1);

  int count = 0;

  world.each<Position, Velocity>([&](Entity e, Position &p, Velocity &v) {
    ++count;

    REQUIRE(e == e2);

    p.x += v.x;
  });

  REQUIRE(count == 1);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e2).x == 22.0f);
}

TEST_CASE("query can modify all requested components",
          "[ecs][querying][mutation]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(e, 1.0f, 2.0f, 3.0f);
  world.addComponentToEntity<Velocity>(e, 4.0f, 5.0f, 6.0f);

  world.each<Position, Velocity>([](Entity, Position &p, Velocity &v) {
    p.x += v.x;
    p.y += v.y;
    p.z += v.z;

    v.x *= 2.0f;
    v.y *= 2.0f;
    v.z *= 2.0f;
  });

  Position &p = world.getComponent<Position>(e);
  Velocity &v = world.getComponent<Velocity>(e);

  REQUIRE(p.x == 5.0f);
  REQUIRE(p.y == 7.0f);
  REQUIRE(p.z == 9.0f);

  REQUIRE(v.x == 8.0f);
  REQUIRE(v.y == 10.0f);
  REQUIRE(v.z == 12.0f);
}

TEST_CASE("query ignores entities without required components",
          "[ecs][querying][filtering]") {
  World world;

  Entity positionOnly = world.createEntity();
  Entity velocityOnly = world.createEntity();
  Entity both = world.createEntity();
  Entity healthOnly = world.createEntity();

  world.addComponentToEntity<Position>(positionOnly, 1.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Velocity>(velocityOnly, 2.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(both, 3.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(both, 4.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Health>(healthOnly, 100);

  int count = 0;

  world.each<Position, Velocity>([&](Entity e, Position &, Velocity &) {
    ++count;
    REQUIRE(e == both);
  });

  REQUIRE(count == 1);
}

TEST_CASE("empty query produces no iterations", "[ecs][querying][empty]") {
  World world;

  int count = 0;

  world.each<Position>([&](Entity, Position &) { ++count; });

  REQUIRE(count == 0);
}

TEST_CASE("query works with entities distributed across many archetypes",
          "[ecs][querying][many-archetypes]") {
  World world;

  Entity positionOnly = world.createEntity();
  Entity positionVelocity = world.createEntity();
  Entity positionHealth = world.createEntity();
  Entity positionVelocityHealth = world.createEntity();

  world.addComponentToEntity<Position>(positionOnly, 1.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(positionVelocity, 2.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Velocity>(positionVelocity, 20.0f, 0.0f, 0.0f);

  world.addComponentToEntity<Position>(positionHealth, 3.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Health>(positionHealth, 30);

  world.addComponentToEntity<Position>(positionVelocityHealth, 4.0f, 0.0f,
                                       0.0f);
  world.addComponentToEntity<Velocity>(positionVelocityHealth, 40.0f, 0.0f,
                                       0.0f);
  world.addComponentToEntity<Health>(positionVelocityHealth, 40);

  int count = 0;
  float sum = 0.0f;

  world.each<Position>([&](Entity, Position &p) {
    ++count;
    sum += p.x;
  });

  REQUIRE(count == 4);
  REQUIRE(sum == 10.0f);
}

TEST_CASE("query remains correct after swap and pop migrations",
          "[ecs][querying][swap-pop]") {
  World world;

  Entity e1 = world.createEntity();
  Entity e2 = world.createEntity();
  Entity e3 = world.createEntity();
  Entity e4 = world.createEntity();

  // Put all four in the same archetype.
  world.addComponentToEntity<Position>(
      e1, 1.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Position>(
      e2, 2.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Position>(
      e3, 3.0f, 0.0f, 0.0f);
  world.addComponentToEntity<Position>(
      e4, 4.0f, 0.0f, 0.0f);

  // Removing this causes another entity to be moved into e2's old slot.
  world.removeComponentFromEntity<Position>(e2);

  int count = 0;
  float sum = 0.0f;

  world.each<Position>([&](Entity, Position& p) {
    ++count;
    sum += p.x;
  });

  REQUIRE(count == 3);
  REQUIRE(sum == 8.0f);

  REQUIRE(world.getComponent<Position>(e1).x == 1.0f);
  REQUIRE(world.getComponent<Position>(e3).x == 3.0f);
  REQUIRE(world.getComponent<Position>(e4).x == 4.0f);
}

TEST_CASE("query component order does not depend on column order",
          "[ecs][querying][component-order]") {
  World world;

  Entity e = world.createEntity();

  world.addComponentToEntity<Position>(
      e, 1.0f, 2.0f, 3.0f);

  world.addComponentToEntity<Velocity>(
      e, 10.0f, 20.0f, 30.0f);

  world.each<Velocity, Position>(
      [&](Entity entity, Velocity& v, Position& p) {
        REQUIRE(entity == e);

        REQUIRE(v.x == 10.0f);
        REQUIRE(v.y == 20.0f);
        REQUIRE(v.z == 30.0f);

        REQUIRE(p.x == 1.0f);
        REQUIRE(p.y == 2.0f);
        REQUIRE(p.z == 3.0f);
      });
}
