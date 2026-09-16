#pragma once
#include "ecs/Profiling.h"
#include <ecs/Context.h>
#include <ecs/World.h>

// Call a function for each entity with the specified components, passing a
// WorldCtxt to allow deferred modifications
//Note that nested eachDeferred's will lead to undefined behaviour
template <typename... Components, typename F>
void eachDeferred(World &world, F &&func) {
  ZoneScoped;
  WorldCtxt ctx;
  world.each<Components...>(
      [&](Entity e, auto &...comps) { func(ctx, e, comps...); });
  ctx.flush(world);
}
