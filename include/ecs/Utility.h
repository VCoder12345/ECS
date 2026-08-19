#pragma once

#include <bitset>
#include <cstdint>

constexpr int ECS_MAX_COMPONENTS = 4096;

using Entity = uint32_t;
using ComponentID = uint32_t;
using ComponentMask = std::bitset<ECS_MAX_COMPONENTS>;
//Note that ColumnIndex has to support at least ECS_MAX_COMPONENTS, so it should be at least 12 bits. Using uint16_t is a good choice here.
using ColumnIndex = uint16_t;

inline ComponentMask emptyCompMask() {
  ComponentMask mask;
  return mask;
}

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
