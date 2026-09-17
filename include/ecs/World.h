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
    EntityRecord &record = entityToAtIdMap[e.index];
    Archetype &at = archetypes[record.archetypeId];
    return at.getComponent<T>(record.row);
  }

  size_t createArchetype(Archetype &&at, const ComponentMask &mask) {
    archetypes.emplace_back(std::move(at));
    size_t atId = archetypes.size() - 1;
    maskToAtIdMap.insert({mask, atId});

    //update queryCache
    for(auto &[queryMask, matches] : queryCache) {
      if ((mask & queryMask) == queryMask) {
        matches.push_back(atId);
      }
    }

    return atId;
  }

  // add a component to an entity, moving it to the correct archetype
  template <typename T, typename... Args>
  T &addComponent(Entity e, Args &&...args) {
    ZoneScoped;
    assert(isAlive(e));
    EntityRecord &record = entityToAtIdMap[e.index];
    const ComponentMask &oldMask = archetypes[record.archetypeId].getMask();
    ComponentMask newMask(oldMask);
    newMask.set(getComponentID<T>());

    auto it = maskToAtIdMap.find(newMask);

    // if the archetype doesn't exist yet, create it and add the component
    size_t newAtId;
    if (it == maskToAtIdMap.end()) {
      // the archetype doesn't exist yet
      newAtId = createArchetype(
          archetypes[record.archetypeId].createAndAddComp<T>(newMask), newMask);
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
    EntityRecord &record = entityToAtIdMap[e.index];
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
      newAtId = createArchetype(
          archetypes[record.archetypeId].createAndRemoveComp(newMask, compId),
          newMask);
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

    auto it = queryCache.find(requiredMask);

    if (it == queryCache.end()) {
      // it's not yet in the cache, so we look for the corresponding archetypes
      // and cache them
      std::vector<size_t> foundAts;
      for (size_t i = 0; i < archetypes.size(); ++i) {
        Archetype &arch = archetypes[i];
        // does the archetype have the required components?
        if ((arch.getMask() & requiredMask) != requiredMask)
          continue;

        foundAts.push_back(i);

        arch.eachEntity<Components...>(std::forward<F>(func));
      }

      queryCache.emplace(std::move(requiredMask), std::move(foundAts));
    } else {
      // it is already in the cache, so we just iterate over the cached
      // archetypes
      for (size_t i : it->second) {
        archetypes[i].eachEntity<Components...>(std::forward<F>(func));
      }
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
  // maps a componentMask for a query to all the relevant archetype ids
  std::unordered_map<ComponentMask, std::vector<size_t>> queryCache;

  void moveEntityToNewAt(Entity e, EntityRecord &record, size_t newAtId);
};
