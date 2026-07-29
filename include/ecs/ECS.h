#pragma once

#include <cstdint>

typedef uint32_t Entity;

class ECS {
public:
  int counter = 0;

  Entity createEntity();
};
