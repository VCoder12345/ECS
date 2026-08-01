#pragma once

#include <ecs/Archetype.h>
#include <ecs/Utility.h>
#include <unordered_map>

class World {
public:
  int counter = 0;

  World() {
    archetypes.emplace_back(Archetype::createEmpty());
    maskToAtIdMap.insert({emptyCompMask(), 0});
  }

  Entity createEntity();

  template <typename T> void addComponentToEntity(Entity e, T &&component) {
    size_t oldAtId = entityToAtIdMap[e];
    Archetype &oldAt = archetypes[oldAtId];
    const ComponentMask &oldMask = oldAt.getMask();
    ComponentMask newMask(oldMask);
    newMask.set(getComponentID<T>());

    size_t newAtId = maskToAtIdMap[newMask];
    if (newAtId == 0) {
      // the archetype doesn't exist yet
      archetypes.emplace_back(oldAt.createAndAddComp<T>(newMask));
      maskToAtIdMap.insert({newMask, archetypes.size() - 1});
      newAtId = archetypes.size() - 1;
    }

    Archetype &newAt = archetypes[newAtId];
    oldAt.swapAndPopColsInto(e, newAt);

    entityToAtIdMap[e] = newAtId;
  }

private:
  // Note that the first archetype (index=0) is always an empty archetype
  std::vector<Archetype> archetypes;
  std::unordered_map<ComponentMask, size_t> maskToAtIdMap;
  std::vector<size_t> entityToAtIdMap;
};
