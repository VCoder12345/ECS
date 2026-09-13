#pragma once
#include <ecs/World.h>
#include <ecs/Context.h>

template <typename... Components, typename F>
void eachDeferred(World &world, F &&func) {
  WorldCtxt ctx;          
  world.each<Components...>([&](Entity e, auto&... comps) {
    func(ctx, e, comps...);
  });
  ctx.flush(world);                 
}
