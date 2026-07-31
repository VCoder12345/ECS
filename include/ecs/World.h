#pragma once

#include <ecs/Utility.h>
#include <ecs/Archetype.h>
#include <memory>
#include <unordered_map>

class World {
public:
  int counter = 0;

  Entity createEntity();

  template <typename T>
  void addComponentToEntity(Entity e, T component) {

  }

private:
  std::vector<Archetype> archetypes;
  std::unordered_map<ComponentMask, size_t> maskToAtIdMap;
  std::vector<size_t> entityToAtIdMap;
};

