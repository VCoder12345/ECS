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

  void flush(World &world) override {}
};

class WorldCtxt {
public:
  Entity createEntity() { 
    return {0, 0}; 
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

  void flush(World &world) {}

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
