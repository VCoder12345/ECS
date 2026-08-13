#pragma once

#include <cassert>
#include <ecs/Utility.h>
#include <iostream>
#include <new>
#include <unordered_map>
#include <vector>

// A struct that holds function pointers for moving and destroying components of
// a specific type.
class ComponentOps {
public:
  void (*moveConstruct)(void *, void *);
  void (*destroy)(void *);

  template <typename T> static ComponentOps create() {
    return {[](void *src, void *dst) {
              new (dst) T(std::move(*static_cast<T *>(src)));
            },
            [](void *obj) { static_cast<T *>(obj)->~T(); }};
  }
};

// A class that represents a column of components of a specific type in an
// archetype.
class Column {
public:
  ComponentID compId;

  // Create a new column for components of type T.
  template <typename T> static Column create() {
    Column column;

    column.compId = getComponentID<T>();
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
        elementSize(other.elementSize), alignment(other.alignment),
        ops(other.ops), compId(other.compId) {
    other.data = nullptr;
    other.size = 0;
    other.capacity = 0;
    other.compId = 0;
  }

  Column &operator=(Column &&other) noexcept {
    if (this != &other) {
      if (data) {
        ::operator delete(data, std::align_val_t(alignment));
      }
      compId = other.compId;
      data = other.data;
      size = other.size;
      capacity = other.capacity;
      elementSize = other.elementSize;
      alignment = other.alignment;
      ops = other.ops;
      other.data = nullptr;
      other.size = 0;
      other.capacity = 0;
      other.compId = 0;
    }
    return *this;
  }

  ~Column() {
    if (data) {
      char *loc = static_cast<char *>(data);
      for (size_t i = 0; i < size; ++i) {
        ops.destroy(loc);
        loc += elementSize;
      }

      ::operator delete(data, std::align_val_t(alignment));
    }
  }

  // Create a new column with the same structure (type, size, alignment) but no
  // data. Note that the capacity of the new column is not the same as the
  // original column, it is initialized to 16.
  // And the size of the new column is 0.
  Column copyStructureToEmptyColumn() const {
    Column col;
    col.compId = compId;
    col.elementSize = elementSize;
    col.alignment = alignment;

    col.data = ::operator new(col.capacity * col.elementSize,
                              std::align_val_t(col.alignment));
    col.ops = ops;

    return col;
  }

  // Move the element at index from this column to the other column, and remove
  // it
  void swapAndPopInto(uint16_t index, Column &oCol) {
    assert(data != nullptr && oCol.data != nullptr);
    assert(index < size && size > 0);

    char *loc = static_cast<char *>(data) + index * elementSize;
    char *newLoc = oCol.allocateSlot();

    ops.moveConstruct(loc, newLoc);

    removeAt(index);

    oCol.size++;
  }

  // Remove the element at index from this column, and destroy it
  void removeAt(uint16_t index) {
    assert(index < size);
    assert(size > 0);

    char *loc = static_cast<char *>(data) + index * elementSize;

    ops.destroy(loc);

    // If the element being removed is not the last element, move the last
    // element (swap and pop) into the location of the removed element
    if (index < size - 1) {
      char *lastLoc = static_cast<char *>(data) + (size - 1) * elementSize;
      ops.moveConstruct(lastLoc, loc);
      ops.destroy(lastLoc);
    }
    --size;
  }

  // makes sure that there is enough space for a new element, and returns the
  // location of the new element Note that it does not increment the size of the
  // column, that is the responsibility of the caller
  char *allocateSlot() {
    if (size >= capacity) {
      capacity *= 2;

      void *newData =
          ::operator new(capacity * elementSize, std::align_val_t(alignment));

      char *oldLoc = (char *)data;
      char *newLoc = (char *)newData;
      for (size_t i = 0; i < size; ++i) {
        ops.moveConstruct(oldLoc, newLoc);
        ops.destroy(oldLoc);

        if (i < size - 1) {
          oldLoc += elementSize;
          newLoc += elementSize;
        }
      }

      ::operator delete(data, std::align_val_t(alignment));
      data = newData;
    }

    return (char *)data + size * elementSize;
  }

  // Add a new element of type T to the column, constructing it in place with
  // the provided arguments. Returns a reference to the new element.
  template <typename T, typename... Args> T &emplace(Args &&...args) {
    assert(sizeof(T) == elementSize);
    assert(alignof(T) == alignment);

    char *loc = allocateSlot();
    T *result = new (loc) T(std::forward<Args>(args)...);
    ++size;

    return *result;
  }

  // Get a reference to the element at index, casted to type T.
  // NOTE: this does not check that the type T is the same as the type of the
  // data element
  template <typename T> T &get(size_t index) {
    return *reinterpret_cast<T *>((char *)data + index * elementSize);
  }

  size_t getSize() const { return size; }

private:
  void *data = nullptr;
  size_t size = 0;
  size_t capacity = 16;
  size_t elementSize;
  size_t alignment;
  ComponentOps ops;

  Column() = default;
};

// A class that represents an archetype, which is a collection of entities that
// share the same set of components. Each archetype has a unique component mask
// that identifies the components it contains, and a set of columns that store
// the actual component data for each entity in the archetype.
class Archetype {
public:
  // Create a new archetype with the specified component types
  template <typename... CompTypes> static Archetype create() {
    Archetype archetype;

    (archetype.registerColumn<CompTypes>(), ...);
    archetype.mask = getCompTypeMask<CompTypes...>();

    return archetype;
  }

  // create an empty archetype with no components
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

  // Create a new archetype with the same components as this one, but with an additional component of type T
  template <typename T> Archetype createAndAddComp(const ComponentMask &mask) {
    Archetype archetype;

    archetype.mask = mask;

    for (const Column &col : columns) {
      archetype.columns.emplace_back(col.copyStructureToEmptyColumn());
    }

    archetype.compColumnMap = compColumnMap;

    archetype.registerColumn<T>();

    //DEBUG: check that the new archetype is valid
    archetype.assertValid();

    return archetype;
  }

  //Create a new archetype with the same components as this one, but with the component of the compId removed
  Archetype createAndRemoveComp(const ComponentMask &mask, ComponentID compId) {
    Archetype archetype;

    archetype.mask = mask;

    for (const Column &col : columns) {
      if (col.compId != compId) {
        archetype.columns.emplace_back(col.copyStructureToEmptyColumn());
        archetype.compColumnMap.emplace(
            col.compId, static_cast<uint16_t>(archetype.columns.size() - 1));
      }
    }

    //DEBUG: check that the new archetype is valid
    archetype.assertValid();

    return archetype;
  }

  size_t getColumnIndex(Entity e) { return entityColumnMap[e]; }

  // Add an entity to the archetype, mapping it to the next available index in the columns
  void addEntity(Entity e) {
    entityColumnMap.insert({e, entities.size()});
    entities.push_back(e);
  }

  // TODO: add error handling for non-existent entity or component
  template <typename T> T &getComponent(Entity e) {
    size_t index = entityColumnMap[e];
    Column &col = columns[compColumnMap[getComponentID<T>()]];

    return col.get<T>(index);
  }

  // Move the entity into oAt, transferring components shared by both
  // archetypes and destroying components that are absent from oAt.
  void swapAndPopColsInto(Entity e, Archetype &oAt) {
    assert(entities.size() > 0);

    size_t index = entityColumnMap[e];

    // copy each column that exists on the target archetype over
    for (size_t i = 0; i < columns.size(); ++i) {
      auto it = oAt.compColumnMap.find(columns[i].compId);
      if (it != oAt.compColumnMap.end()) {
        size_t oColIndex = it->second;
        columns[i].swapAndPopInto(index, oAt.columns[oColIndex]);
      } else {
        columns[i].removeAt(index);
      }
    }

    // remove the entity from this archetype
    entityColumnMap.erase(e);

    // if the entity being removed is not the last entity, move the last entity into its place (swap and pop)
    if (index + 1 < entities.size()) {
      Entity lastEntity = entities.back();
      entityColumnMap[lastEntity] = index;
      entities[index] = lastEntity;
    }
    entities.pop_back();

    oAt.addEntity(e);
  }

  // only for debugging, checks if the archetype is valid
  void assertValid() const {
    assert(columns.size() == compColumnMap.size());

    for (size_t i = 0; i < columns.size(); ++i) {
      const Column &col = columns[i];

      assert(mask.test(col.compId));
      assert(col.getSize() == entities.size());

      auto it = compColumnMap.find(col.compId);
      assert(it != compColumnMap.end());
      assert(it->second == i);
    }
  }

  // Add a new component of type T to the archetype, constructing it in place with the provided arguments
  template <typename T, typename... Args> T &addDataToColumn(Args &&...args) {
    ComponentID id = getComponentID<T>();
    assert(compColumnMap.find(id) != compColumnMap.end());
    assert(mask.test(id));

    size_t colIndex = compColumnMap[id];
    return columns[colIndex].emplace<T>(std::forward<Args>(args)...);
  }

  std::vector<Entity> &getEntities() { return entities; }

private:
  ComponentMask mask;
  std::unordered_map<ComponentID, uint16_t> compColumnMap;
  std::unordered_map<Entity, uint16_t> entityColumnMap;
  std::vector<Entity> entities;
  std::vector<Column> columns;

  Archetype() {}

  // Register a new column for components of type T in the archetype
  template <typename T> void registerColumn() {
    ComponentID id = getComponentID<T>();
    columns.emplace_back(Column::create<T>());

    compColumnMap.emplace(id, static_cast<uint16_t>(columns.size() - 1));
  }
};
