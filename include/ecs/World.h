#pragma once

#include <ecs/Archetype.h>
#include <ecs/Utility.h>
#include <unordered_map>

#include <ecs/Profiling.h>

struct EntityRecord {
  size_t archetypeId;
  size_t row;
};

// TODO: delete empty archetypes?

class World {
public:
  World();

  Entity createEntity();

  Entity genEntityId();
  void materializeEntity(Entity e);

  void removeEntity(Entity e);

  bool isAlive(Entity e) const;

  template <typename T> T &getComponent(Entity e) {
    assert(isAlive(e));
    EntityRecord& record = entityToAtIdMap[e.index];
    Archetype &at = archetypes[record.archetypeId];
    return at.getComponent<T>(record.row);
  }

  // add a component to an entity, moving it to the correct archetype
  template <typename T, typename... Args>
  T &addComponent(Entity e, Args &&...args) {
    ZoneScoped;
    assert(isAlive(e));
    EntityRecord& record = entityToAtIdMap[e.index];
    const ComponentMask &oldMask = archetypes[record.archetypeId].getMask();
    ComponentMask newMask(oldMask);
    newMask.set(getComponentID<T>());

    auto it = maskToAtIdMap.find(newMask);

    // if the archetype doesn't exist yet, create it and add the component
    size_t newAtId;
    if (it == maskToAtIdMap.end()) {
      // the archetype doesn't exist yet
      archetypes.emplace_back(archetypes[record.archetypeId].createAndAddComp<T>(newMask));
      newAtId = archetypes.size() - 1;
      maskToAtIdMap.insert({newMask, newAtId});
    } else {
      // the archetype already exists
      newAtId = it->second;
    }

    moveEntityToNewAt(e, record, newAtId);

    // add the new component to the new archetype
    return archetypes[newAtId].addDataToColumn<T>(std::forward<Args>(args)...);
  }


  // remove a component from an entity, moving it to the correct archetype
  template <typename T> void removeComponent(Entity e) {
    ZoneScoped;

    assert(isAlive(e));
    EntityRecord& record = entityToAtIdMap[e.index];
    const ComponentMask &oldMask = archetypes[record.archetypeId].getMask();
    ComponentMask newMask(oldMask);
    ComponentID compId = getComponentID<T>();
    newMask.reset(compId);

    auto it = maskToAtIdMap.find(newMask);

    // TODO: with the setup now it is actually impossible that the archetype
    // doesn't exist that might change though
    size_t newAtId;
    if (it == maskToAtIdMap.end()) {
      // the archetype doesn't exist yet
      archetypes.emplace_back(
          archetypes[record.archetypeId].createAndRemoveComp(newMask, compId));
      newAtId = archetypes.size() - 1;
      maskToAtIdMap.insert({newMask, newAtId});
    } else {
      // the archetype already exists
      newAtId = it->second;
    }

    moveEntityToNewAt(e, record, newAtId);
  }

  Archetype &getArchetypeForEntity(Entity e);

  template <typename... Components, typename F> void each(F &&func) {
    ZoneScoped;
    ComponentMask requiredMask;
    (requiredMask.set(getComponentID<Components>()), ...);

    for (Archetype &arch : archetypes) {
      // does the archetype have the required components?
      if ((arch.getMask() & requiredMask) != requiredMask)
        continue;

      arch.eachEntity<Components...>(std::forward<F>(func));
    }
  }

  template <typename T> bool hasComponent(Entity e) {
    return getArchetypeForEntity(e).getMask().test(getComponentID<T>());
  }

private:
  // Note that the first archetype (index=0) is always an empty archetype
  std::vector<Archetype> archetypes;
  std::unordered_map<ComponentMask, size_t> maskToAtIdMap;
  std::vector<EntityRecord> entityToAtIdMap;

  int counter = 0;

  std::vector<EntityIndex> unusedEntityIds;
  std::vector<uint32_t> entityGenerations;

  void moveEntityToNewAt(Entity e, EntityRecord &record, size_t newAtId);
};
