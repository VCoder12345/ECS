#include <ecs/World.h>

Entity World::createEntity() { 
  //let the entity be part of the empty archetype
  Entity newEntity = counter;
  ++counter;
  entityToAtIdMap.push_back(0);
  archetypes[0].addEntity(newEntity);

  return newEntity; 
}
