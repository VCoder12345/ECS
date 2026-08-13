#include <ecs/World.h>

World::World() {
  // create the empty archetype: which is the archetype with no components,
  // which all entities start in
  archetypes.emplace_back(Archetype::createEmpty());
  maskToAtIdMap.insert({emptyCompMask(), 0});
}

//TODO: replace counter by the size of the entityToAtIdMap
Entity World::createEntity() { 
  //let the entity be part of the empty archetype
  Entity newEntity = counter;
  ++counter;
  entityToAtIdMap.push_back(0);
  archetypes[0].addEntity(newEntity);

  return newEntity;
}

Archetype &World::getArchetypeForEntity(Entity e) {
  size_t atId = entityToAtIdMap[e];
  return archetypes[atId];
}
