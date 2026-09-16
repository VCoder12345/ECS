#include "ecs/Profiling.h"
#include <ecs/Column.h>

Column::Column(Column &&other) noexcept
    : compId(other.compId), data(other.data), size(other.size),
      capacity(other.capacity), elementSize(other.elementSize),
      alignment(other.alignment), ops(other.ops) {
  other.data = nullptr;
  other.size = 0;
  other.capacity = 0;
  other.compId = 0;
}

Column::~Column() {
  if (data) {
    char *loc = static_cast<char *>(data);
    for (size_t i = 0; i < size; ++i) {
      ops.destroy(loc);
      loc += elementSize;
    }

    ::operator delete(data, std::align_val_t(alignment));
  }
}
Column Column::copyStructureToEmptyColumn() const {
  Column col;
  col.compId = compId;
  col.elementSize = elementSize;
  col.alignment = alignment;

  col.data = ::operator new(col.capacity * col.elementSize,
                            std::align_val_t(col.alignment));
  col.ops = ops;

  return col;
}

void Column::swapAndPopInto(size_t index, Column &oCol) {
  ZoneScoped;
  assert(data != nullptr && oCol.data != nullptr);
  assert(index < size && size > 0);

  char *loc = static_cast<char *>(data) + index * elementSize;
  char *newLoc = oCol.allocateSlot();

  ops.moveConstruct(loc, newLoc);

  removeAt(index);

  oCol.size++;
}

void Column::removeAt(size_t index) {
  ZoneScoped;
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

char *Column::allocateSlot() {
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

size_t Column::getSize() const { return size; }
