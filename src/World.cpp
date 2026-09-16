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
    entityToAtIdMap[newEntity.index] = {0,0};
  } else {
    newEntity.index = counter;
    ++counter;
    // TODO: check if counter < MAX_ENTITIES
    entityToAtIdMap.push_back({0,0});
    entityGenerations.push_back(0);
  }

  newEntity.generation = entityGenerations[newEntity.index];

  return newEntity;
}

void World::materializeEntity(Entity e) {
  // let the entity be part of the empty archetype
  size_t row = archetypes[0].addEntity(e);
  entityToAtIdMap[e.index] = {0, row};
}

// Remove an entity from the world, returning its id to the pool of unused ids
void World::removeEntity(Entity e) {
  assert(isAlive(e));
  unusedEntityIds.push_back(e.index);
  entityGenerations[e.index]++; // increment generation to invalidate old
                                // references
  
  EntityRecord& record = entityToAtIdMap[e.index];
  auto lastEntity = archetypes[record.archetypeId].removeEntity(record.row);


  if (lastEntity.has_value()) {
    entityToAtIdMap[lastEntity.value().index].row = record.row;
  }

  entityToAtIdMap[e.index] = {0,0};
}
bool World::isAlive(Entity e) const {
  return e.index < entityGenerations.size() &&
         entityGenerations[e.index] == e.generation;
}
Archetype &World::getArchetypeForEntity(Entity e) {
  const EntityRecord &rec = entityToAtIdMap[e.index];
  return archetypes[rec.archetypeId];
}

void World::moveEntityToNewAt(Entity e, EntityRecord &record, size_t newAtId) {
  auto [newRow, lastEntity] = archetypes[record.archetypeId].swapAndPopColsInto(
      e, record.row, archetypes[newAtId]);

  if (lastEntity.has_value()) {
    entityToAtIdMap[lastEntity.value().index].row = record.row;
  }

  record.archetypeId = newAtId;
  record.row = newRow;
}
