#include <ecs/World.h>

World::World() {
  // create the empty archetype: which is the archetype with no components,
  // which all entities start in
  archetypes.emplace_back(Archetype::createEmpty());
  maskToAtIdMap.insert({emptyCompMask(), 0});
}

// TODO: replace counter by the size of the entityToAtIdMap
Entity World::createEntity() {
  ZoneScoped;
  // if there are any unused entity ids, reuse one of those
  // otherwise, create a new entity id by incrementing the counter
  Entity newEntity;
  if (unusedEntityIds.size() > 0) {
    newEntity = unusedEntityIds.back();
    unusedEntityIds.pop_back();
    entityToAtIdMap[newEntity] = 0;
  } else {
    newEntity = counter;
    ++counter;
    //TODO: check if counter < MAX_ENTITIES
    entityToAtIdMap.push_back(0);
  }

  // let the entity be part of the empty archetype
  archetypes[0].addEntity(newEntity);

  return newEntity;
}

// Remove an entity from the world, returning its id to the pool of unused ids
void World::removeEntity(Entity e) { 
  unusedEntityIds.push_back(e); 
  
  getArchetypeForEntity(e).removeEntity(e);

  entityToAtIdMap[e] = 0;
}

Archetype &World::getArchetypeForEntity(Entity e) {
  size_t atId = entityToAtIdMap[e];
  return archetypes[atId];
}
