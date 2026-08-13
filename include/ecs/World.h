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

  template <typename T>
  T& getComponent(Entity e) {
    Archetype& at = getArchetypeForEntity(e);
    return at.getComponent<T>(e);
  }

  template <typename T, typename... Args>
  T &addComponentToEntity(Entity e, Args &&...args) {
    size_t oldAtId = entityToAtIdMap[e];
    const ComponentMask &oldMask = archetypes[oldAtId].getMask();
    ComponentMask newMask(oldMask);
    newMask.set(getComponentID<T>());

    auto it = maskToAtIdMap.find(newMask);

    size_t newAtId;
    if (it == maskToAtIdMap.end()) {
      // the archetype doesn't exist yet
      archetypes.emplace_back(archetypes[oldAtId].createAndAddComp<T>(newMask));
      newAtId = archetypes.size() - 1;
      maskToAtIdMap.insert({newMask, newAtId});
    } else {
      // the archetype already exists
      newAtId = it->second;
    }

    archetypes[oldAtId].swapAndPopColsInto(e, archetypes[newAtId]);

    entityToAtIdMap[e] = newAtId;

    return archetypes[newAtId].addDataToColumn<T>(
        std::forward<Args>(args)...);
  }

  Archetype &getArchetypeForEntity(Entity e) {
    size_t atId = entityToAtIdMap[e];
    return archetypes[atId];
  }

private:
  // Note that the first archetype (index=0) is always an empty archetype
  std::vector<Archetype> archetypes;
  std::unordered_map<ComponentMask, size_t> maskToAtIdMap;
  std::vector<size_t> entityToAtIdMap;
};
