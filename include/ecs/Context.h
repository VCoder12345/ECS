#pragma once

#include <ecs/World.h>

class WorldCtxt {
public:
  Entity createEntity() {}

  void destroyEntity(Entity e) {}

  template <typename T> void removeComponent(Entity e) {}

  template <typename T, typename... Args>
  T &addComponent(Entity e, Args &&...args) {}
};
