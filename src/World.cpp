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

  Entity newEntity = genEntityId();

  materializeEntity(newEntity);

  return newEntity;
}

Entity World::genEntityId() {
  // if there are any unused entity ids, reuse one of those
  // otherwise, create a new entity id by incrementing the counter
  Entity newEntity;
  if (unusedEntityIds.size() > 0) {
    newEntity.index = unusedEntityIds.back();
    unusedEntityIds.pop_back();
    entityToAtIdMap[newEntity.index] = 0;
  } else {
    newEntity.index = counter;
    ++counter;
    // TODO: check if counter < MAX_ENTITIES
    entityToAtIdMap.push_back(0);
    entityGenerations.push_back(0);
  }

  newEntity.generation = entityGenerations[newEntity.index];

  return newEntity;
}

void World::materializeEntity(Entity e) {
  // let the entity be part of the empty archetype
  archetypes[0].addEntity(e);
}

// Remove an entity from the world, returning its id to the pool of unused ids
void World::removeEntity(Entity e) {
  unusedEntityIds.push_back(e.index);
  entityGenerations[e.index]++; // increment generation to invalidate old
                                // references

  getArchetypeForEntity(e).removeEntity(e);

  entityToAtIdMap[e.index] = 0;
}

Archetype &World::getArchetypeForEntity(Entity e) {
  size_t atId = entityToAtIdMap[e.index];
  return archetypes[atId];
}
