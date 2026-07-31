#pragma once

#include <bitset>
#include <cstdint>

constexpr int ECS_MAX_COMPONENTS = 4096;

using Entity = uint32_t;
using ComponentID = uint32_t;
using ComponentMask = std::bitset<ECS_MAX_COMPONENTS>;

inline ComponentID getNextComponentID() {
  static ComponentID id = 0;
  return id++;
}

template<typename T>
ComponentID getComponentID() {
  static ComponentID id = getNextComponentID();
  return id;
}

template<typename... ComponentTs>
ComponentMask getCompTypeMask() {
  ComponentMask mask;

  (mask.set(getComponentID<ComponentTs>()), ...);
  return mask;
}
