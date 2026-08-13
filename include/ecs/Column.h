#pragma once
#include <cassert>
#include <ecs/Utility.h>
#include <new>

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

  Column(Column &&other) noexcept;

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

  ~Column();

  // Create a new column with the same structure (type, size, alignment) but no
  // data. Note that the capacity of the new column is not the same as the
  // original column, it is initialized to 16.
  // And the size of the new column is 0.
  Column copyStructureToEmptyColumn() const;

  // Move the element at index from this column to the other column, and remove
  // it
  void swapAndPopInto(uint16_t index, Column &oCol);

  // Remove the element at index from this column, and destroy it
  void removeAt(uint16_t index);

  // makes sure that there is enough space for a new element, and returns the
  // location of the new element Note that it does not increment the size of the
  // column, that is the responsibility of the caller
  char *allocateSlot();

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
    assert(sizeof(T) == elementSize);
    assert(compId == getComponentID<T>());
    assert(alignof(T) == alignment);

    return *reinterpret_cast<T *>((char *)data + index * elementSize);
  }

  size_t getSize() const;

private:
  void *data = nullptr;
  size_t size = 0;
  size_t capacity = 16;
  size_t elementSize;
  size_t alignment;
  ComponentOps ops;

  Column() = default;
};
