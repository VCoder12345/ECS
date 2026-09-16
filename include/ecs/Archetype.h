#pragma once

#include <ecs/Column.h>
#include <ecs/Utility.h>

#include <cassert>
#include <optional>
#include <unordered_map>
#include <vector>

#include <ecs/Profiling.h>

// A class that represents an archetype, which is a collection of entities that
// share the same set of components. Each archetype has a unique component mask
// that identifies the components it contains, and a set of columns that store
// the actual component data for each entity in the archetype.
class Archetype {
public:
  // Create a new archetype with the specified component types
  template <typename... CompTypes> static Archetype create() {
    ZoneScoped;
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

  // Create a new archetype with the same components as this one, but with an
  // additional component of type T
  template <typename T>
  Archetype createAndAddComp(const ComponentMask &newMask) {
    ZoneScoped;
    Archetype archetype;

    archetype.mask = newMask;

    for (const Column &col : columns) {
      archetype.columns.emplace_back(col.copyStructureToEmptyColumn());
    }

    archetype.compColumnMap = compColumnMap;

    archetype.registerColumn<T>();

    // DEBUG: check that the new archetype is valid
    assert(archetype.validate());

    return archetype;
  }

  // Create a new archetype with the same components as this one, but with the
  // component of the compId removed
  Archetype createAndRemoveComp(const ComponentMask &newMask,
                                ComponentID compId) {
    ZoneScoped;
    Archetype archetype;

    archetype.mask = newMask;

    for (const Column &col : columns) {
      if (col.compId != compId) {
        archetype.columns.emplace_back(col.copyStructureToEmptyColumn());
        archetype.compColumnMap.emplace(
            col.compId, static_cast<ColumnIndex>(archetype.columns.size() - 1));
      }
    }

    // DEBUG: check that the new archetype is valid
    assert(archetype.validate());

    return archetype;
  }

  // Add an entity to the archetype, mapping it to the next available index in
  // the columns
  size_t addEntity(Entity e) {
    ZoneScoped;
    entities.push_back(e);

    return entities.size() - 1;
  }

  template <typename T> Column &getColumn() {
    ZoneScoped;
    ComponentID id = getComponentID<T>();
    assert(compColumnMap.find(id) != compColumnMap.end());
    assert(mask.test(id));
    size_t colIndex = compColumnMap[id];
    return columns[colIndex];
  }

  // TODO: add error handling for non-existent entity or component
  template <typename T> T &getComponentAt(size_t index) {
    ZoneScoped;
    Column &col = getColumn<T>();

    return col.get<T>(index);
  }

  template <typename T> ColumnIterator<T> getColumnIterator() {
    ZoneScoped;
    Column &col = getColumn<T>();

    return col.getIterator<T>();
  }

  template <typename T> T &getComponent(size_t row) {
    ZoneScoped;

    return getComponentAt<T>(row);
  }

  std::optional<Entity> removeEntity(size_t row) {
    ZoneScoped;
    assert(entities.size() > 0);

    // remove the components for the entity from each column
    for (Column &col : columns) {
      col.removeAt(row);
    }

    return removeEntityAt(row);
  }

  // Move the entity into oAt, transferring components shared by both
  // archetypes and destroying components that are absent from oAt.
  // returns the row where the entity is stored in the columns and the
  // last-entity (the one that is swapped)
  std::pair<size_t, std::optional<Entity>> swapAndPopColsInto(Entity e, size_t row,
                                               Archetype &oAt) {
    ZoneScoped;
    assert(entities.size() > 0);

    // copy each column that exists on the target archetype over
    for (size_t i = 0; i < columns.size(); ++i) {
      auto it = oAt.compColumnMap.find(columns[i].compId);
      if (it != oAt.compColumnMap.end()) {
        size_t oColIndex = it->second;
        columns[i].swapAndPopInto(row, oAt.columns[oColIndex]);
      } else {
        columns[i].removeAt(row);
      }
    }

    auto lastEntityOpt = removeEntityAt(row);

    return {oAt.addEntity(e), lastEntityOpt};
  }

  // only for debugging, checks if the archetype is valid
  bool validate() const {
    for (std::size_t i = 0; i < columns.size(); ++i) {
      const Column &col = columns[i];

      if (!mask.test(col.compId))
        return false;

      if (col.getSize() != entities.size())
        return false;

      auto it = compColumnMap.find(col.compId);

      if (it == compColumnMap.end())
        return false;

      if (it->second != i)
        return false;
    }

    return true;
  }

  // Add a new component of type T to the archetype, constructing it in place
  // with the provided arguments
  template <typename T, typename... Args> T &addDataToColumn(Args &&...args) {
    ZoneScoped;
    ComponentID id = getComponentID<T>();
    assert(compColumnMap.find(id) != compColumnMap.end());
    assert(mask.test(id));

    size_t colIndex = compColumnMap[id];
    return columns[colIndex].emplace<T>(std::forward<Args>(args)...);
  }

  std::vector<Entity> &getEntities() { return entities; }

  template <typename... Components, typename F> void eachEntity(F &&func) {
    ZoneScoped;

    auto componentIters = std::tuple{getColumnIterator<Components>()...};

    // NOTE: we assume that entities and columns are structured in the same way
    // i.e. entity on index 2 has column data for each column at index 2
    for (size_t i = 0; i < entities.size(); ++i) {
      Entity e = entities[i];

      std::apply([&](auto &...iters) { func(e, iters.getAndNext()...); },
                 componentIters);
    }
  }

private:
  ComponentMask mask;
  std::unordered_map<ComponentID, ColumnIndex> compColumnMap;
  std::vector<Entity> entities;
  std::vector<Column> columns;

  Archetype() {}

  // Register a new column for components of type T in the archetype
  template <typename T> void registerColumn() {
    ComponentID id = getComponentID<T>();
    columns.emplace_back(Column::create<T>());

    compColumnMap.emplace(id, static_cast<ColumnIndex>(columns.size() - 1));
  }

  // remove the entity from this archetype
  std::optional<Entity> removeEntityAt(size_t index) {
    ZoneScoped;

    std::optional<Entity> moved;

    // if the entity being removed is not the last entity, move the last entity
    // into its place (swap and pop)
    if (index + 1 < entities.size()) {
      Entity lastEntity = entities.back();
      entities[index] = lastEntity;

      moved = lastEntity;
    }
    entities.pop_back();

    return moved;
  }
};
