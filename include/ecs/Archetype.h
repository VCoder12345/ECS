#pragma once

#include <cassert>
#include <ecs/Utility.h>
#include <new>
#include <vector>

class Column {
public:
  template <typename T> static Column create() {
    Column column;

    column.elementSize = sizeof(T);
    column.alignment = alignof(T);
    column.capacity = 16;
    column.size = 0;

    column.data = ::operator new(column.capacity * column.elementSize,
                                 std::align_val_t(column.alignment));

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

private:
  void *data = nullptr;
  size_t size = 0;
  size_t capacity = 0;
  size_t elementSize;
  size_t alignment;

  Column() = default;
};

class Archetype {
public:


  template <typename... CompTypes> static Archetype create() {
    Archetype archetype;
    // TODO: resize to max componentID instead
    archetype.compColumnMap.resize(ECS_MAX_COMPONENTS);
    (archetype.registerColumn<CompTypes>(), ...);
    archetype.mask = getCompTypeMask<CompTypes...>();

    return archetype;
  }

  Archetype(const Archetype&) = delete;
  Archetype& operator=(const Archetype&) = delete;

  Archetype(Archetype&&) = default;
  Archetype& operator=(Archetype&&) = default;


  ~Archetype()= default;

  ComponentMask getMask() const { return mask; }

private:
  ComponentMask mask;
  std::vector<Column> columns;
  std::vector<uint16_t> compColumnMap;

  Archetype() {}

  template <typename T> void registerColumn() {
    ComponentID id = getComponentID<T>();
    columns.emplace_back(Column::create<T>());
    assert(id < compColumnMap.size());
    compColumnMap[id] = columns.size() - 1;
  }
};
