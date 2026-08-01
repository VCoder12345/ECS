#pragma once

#include <cassert>
#include <ecs/Utility.h>
#include <new>
#include <unordered_map>
#include <vector>

class ComponentOps {
public:
  void (*move)(void *, void *);
  void (*destroy)(void *);

  template <typename T> static ComponentOps create() {
    return {[](void *src, void *dst) {
              new (dst) T(std::move(*static_cast<T *>(src)));
            },
            [](void *obj) { static_cast<T *>(obj)->~T(); }};
  }
};

class Column {
public:
  template <typename T> static Column create() {
    Column column;

    column.elementSize = sizeof(T);
    column.alignment = alignof(T);
    column.size = 0;

    column.data = ::operator new(column.capacity * column.elementSize,
                                 std::align_val_t(column.alignment));

    column.ops = ComponentOps::create<T>();

    return column;
  }

  Column(const Column &) = delete;
  Column &operator=(const Column &) = delete;

  Column(Column &&other) noexcept
      : data(other.data), size(other.size), capacity(other.capacity),
        elementSize(other.elementSize), alignment(other.alignment) {
    other.data = nullptr;
    other.size = 0;
    other.capacity = 0;
  }

  Column &operator=(Column &&other) noexcept {
    if (this != &other) {
      if (data) {
        ::operator delete(data, std::align_val_t(alignment));
      }
      data = other.data;
      size = other.size;
      capacity = other.capacity;
      elementSize = other.elementSize;
      alignment = other.alignment;
      other.data = nullptr;
      other.size = 0;
      other.capacity = 0;
    }
    return *this;
  }

  ~Column() {
    if (data) {
      ::operator delete(data, std::align_val_t(alignment));
    }
  }

  Column copyStructureToEmptyColumn() const {
    Column col;
    col.elementSize = elementSize;
    col.alignment = alignment;

    col.data = ::operator new(col.capacity * col.elementSize,
                              std::align_val_t(col.alignment));
    return col;
  }

  void swapAndPopInto(uint16_t index, Column &oCol) {
    assert(data != nullptr && oCol.data != nullptr);
    assert(index < size && oCol.size > 0);

    char *loc = static_cast<char *>(data) + index * elementSize;
    char* newLoc = oCol.accomodateNewElement();

    ops.move(loc, newLoc);
    ops.destroy(loc);

    if (index < size - 1) {
      char* lastLoc = static_cast<char*>(data) + (size - 1) * elementSize;
      ops.move(lastLoc, loc);
      ops.destroy(lastLoc);
    }
    --size;
  }

  char* accomodateNewElement() {
    if (size >= capacity) {
      capacity *= 2;

      void *newData =
          ::operator new(capacity * elementSize, std::align_val_t(alignment));

      char *oldLoc = (char *)data;
      char *newLoc = (char *)newData;
      for (size_t i = 0; i < size; ++i) {
        ops.move(oldLoc, newLoc); 
        ops.destroy(oldLoc);

        if (i < size - 1) {
          oldLoc += elementSize;
          newLoc += elementSize;
        }
      }

      operator delete(data);
      data = newData;
    }
    ++size;

    return (char*)data + (size - 1) * elementSize;
  }

private:
  void *data = nullptr;
  size_t size = 0;
  size_t capacity = 16;
  size_t elementSize;
  size_t alignment;
  ComponentOps ops;

  Column() = default;
};

class Archetype {
public:

  template <typename... CompTypes> static Archetype create() {
    Archetype archetype;

    (archetype.registerColumn<CompTypes>(), ...);
    archetype.mask = getCompTypeMask<CompTypes...>();

    return archetype;
  }

  static Archetype createEmpty() {
    Archetype archetype;

    return archetype;
  }

  Archetype(const Archetype &) = delete;
  Archetype &operator=(const Archetype &) = delete;

  Archetype(Archetype &&) = default;
  Archetype &operator=(Archetype &&) = default;

  ~Archetype() = default;

  const ComponentMask &getMask() const { return mask; }

  template <typename T> Archetype createAndAddComp(const ComponentMask &mask) {
    Archetype archetype;

    archetype.mask = mask;

    for (const Column &col : columns) {
      archetype.columns.emplace_back(col.copyStructureToEmptyColumn());
    }

    archetype.compColumnMap = compColumnMap;

    archetype.registerColumn<T>();

    return archetype;
  }

  size_t getColumnIndex(Entity e) {
    return entityColumnMap[e];
  }

  void addEntity(Entity e) {
    entityColumnMap.insert({ e, columns.size() - 1 });
    entities.push_back(e);
  }

  //assumes that oAt was created from the archetype, so that only the last column differs
  void swapAndPopColsInto(Entity e, Archetype& oAt) {
    assert(columns.size() < oAt.columns.size());
    size_t index = entityColumnMap[e];

    for (size_t i = 0; i < columns.size(); ++i) {
      columns[i].swapAndPopInto(index, oAt.columns[i]);
    }

    entityColumnMap.erase(e);

    if (index < entities.size() - 1) {
      Entity lastEntity = entities.back();
      entityColumnMap[lastEntity] = index;
      entities[index] = lastEntity;
    }
    entities.pop_back();

    oAt.addEntity(e);
  }

private:
  ComponentMask mask;
  std::unordered_map<ComponentID, uint16_t> compColumnMap;
  std::unordered_map<Entity, uint16_t> entityColumnMap;
  std::vector<Entity> entities;
  std::vector<Column> columns;

  Archetype() {}

  template <typename T> void registerColumn() {
    ComponentID id = getComponentID<T>();
    columns.emplace_back(Column::create<T>());

    compColumnMap.emplace(id, static_cast<uint16_t>(columns.size() - 1));
  }
};
