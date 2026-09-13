#pragma once

#include "ecs/Utility.h"
#include <ecs/World.h>
#include <memory>
#include <unordered_map>
#include <vector>

class ICompQueue {
public:
  virtual void flush(World &) = 0;
  virtual ~ICompQueue() = default;
};

template <typename T> class CompQueue : public ICompQueue {
public:
  std::vector<std::pair<Entity, T>> addQueue;
  std::vector<Entity> removeQueue;

  void flush(World &world) override {
    for (auto &[e, comp] : addQueue) {
      if (world.isAlive(e))
        world.addComponent<T>(e, std::move(comp));
    }

    for (Entity e : removeQueue) {
      if (world.isAlive(e))
        world.removeComponent<T>(e);
    }
  }
};

class WorldCtxt {
public:
  Entity createEntity(World &world) { 
    Entity e = world.genEntityId();

    createQueue.push_back(e);
    return e;
  }

  void destroyEntity(Entity e) {
    removeQueue.push_back(e);
  }

  template <typename T> void removeComponent(Entity e) {
    getAddQueue<T>().removeQueue.push_back(e);
  }

  template <typename T, typename... Args>
  void addComponent(Entity e, Args &&...args) {
    getAddQueue<T>().addQueue.emplace_back(e, T(std::forward<Args>(args)...));
  }

  void flush(World &world) {
    for (Entity e : createQueue) {
      world.materializeEntity(e);
    }

    for (Entity e : removeQueue) {
      if (world.isAlive(e))
        world.removeEntity(e);
    }

    for (auto &[id, queue] : compQueues) {
      queue->flush(world);
    }
  }

private:
  std::unordered_map<ComponentID, std::unique_ptr<ICompQueue>> compQueues;
  std::vector<Entity> createQueue;
  std::vector<Entity> removeQueue;

  template <typename T> CompQueue<T> &getAddQueue() {
    ComponentID id = getComponentID<T>();
    auto it = compQueues.find(id);

    if (it == compQueues.end()) {
      // not found --> have to create it
      auto queue = std::make_unique<CompQueue<T>>();
      CompQueue<T> *ptr = queue.get();
      compQueues.emplace(id, std::move(queue));
      return *ptr;
    }

    CompQueue<T> *queue = static_cast<CompQueue<T> *>(it->second.get());

    return *queue;
  }
};
