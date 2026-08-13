#pragma once

#include <ecs/Archetype.h>
#include <ecs/Utility.h>
#include <unordered_map>

//TODO: delete empty archetypes?

class World {
public:
  int counter = 0;

  World();

  Entity createEntity();

  template <typename T>
  T& getComponent(Entity e) {
    Archetype& at = getArchetypeForEntity(e);
    return at.getComponent<T>(e);
  }

  //add a component to an entity, moving it to the correct archetype
  template <typename T, typename... Args>
  T &addComponentToEntity(Entity e, Args &&...args) {
    size_t oldAtId = entityToAtIdMap[e];
    const ComponentMask &oldMask = archetypes[oldAtId].getMask();
    ComponentMask newMask(oldMask);
    newMask.set(getComponentID<T>());

    auto it = maskToAtIdMap.find(newMask);

    //if the archetype doesn't exist yet, create it and add the component
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

    //move data from the old archetype to the new one
    archetypes[oldAtId].swapAndPopColsInto(e, archetypes[newAtId]);

    entityToAtIdMap[e] = newAtId;

    //add the new component to the new archetype
    return archetypes[newAtId].addDataToColumn<T>(
        std::forward<Args>(args)...);
  }

  //remove a component from an entity, moving it to the correct archetype
  template <typename T>
  void removeComponentFromEntity(Entity e) {
    size_t oldAtId = entityToAtIdMap[e];
    const ComponentMask &oldMask = archetypes[oldAtId].getMask();
    ComponentMask newMask(oldMask);
    ComponentID compId = getComponentID<T>();
    newMask.reset(compId);

    auto it = maskToAtIdMap.find(newMask);

    //TODO: with the setup now it is actually impossible that the archetype doesn't exist
    //that might change though
    size_t newAtId;
    if (it == maskToAtIdMap.end()) {
      // the archetype doesn't exist yet
      archetypes.emplace_back(archetypes[oldAtId].createAndRemoveComp(newMask, compId));
      newAtId = archetypes.size() - 1;
      maskToAtIdMap.insert({newMask, newAtId});
    } else {
      // the archetype already exists
      newAtId = it->second;
    }

    archetypes[oldAtId].swapAndPopColsInto(e, archetypes[newAtId]);

    entityToAtIdMap[e] = newAtId;
  }

  Archetype &getArchetypeForEntity(Entity e);

private:
  // Note that the first archetype (index=0) is always an empty archetype
  std::vector<Archetype> archetypes;
  std::unordered_map<ComponentMask, size_t> maskToAtIdMap;
  std::vector<size_t> entityToAtIdMap;
};
