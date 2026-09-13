#pragma once

#include <bitset>
#include <cstdint>

constexpr int ECS_MAX_COMPONENTS = 4096;

using EntityIndex = uint32_t;

// An entity consists of an unique index
// but also a generation to distinguish between different reused entities in
// deferred calls
struct Entity {
  EntityIndex index;
  uint32_t generation;

  bool operator==(const Entity &o) const {
    return index == o.index && generation == o.generation;
  }
};

// define a custom hash function for Entity so that it can be used in
// unordered_map
namespace std {
template <> struct hash<Entity> {
  // note that we only hash the index, because there will be no entities with
  // the same index and different generation in the same frame
  // except for deferred actions
  size_t operator()(const Entity &entity) const {
    return std::hash<EntityIndex>{}(entity.index);
  }
};
} // namespace std

using ComponentID = uint32_t;
using ComponentMask = std::bitset<ECS_MAX_COMPONENTS>;

// Note that ColumnIndex has to support at least ECS_MAX_COMPONENTS, so it
// should be at least 12 bits. Using uint16_t is a good choice here.
using ColumnIndex = uint16_t;

inline ComponentMask emptyCompMask() {
  ComponentMask mask;
  return mask;
}

inline ComponentID getNextComponentID() {
  static ComponentID id = 0;
  return id++;
}

template <typename T> ComponentID getComponentID() {
  static ComponentID id = getNextComponentID();
  return id;
}

template <typename... ComponentTs> ComponentMask getCompTypeMask() {
  ComponentMask mask;

  (mask.set(getComponentID<ComponentTs>()), ...);
  return mask;
}
