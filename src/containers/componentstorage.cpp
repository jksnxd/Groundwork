#include "sparseset.cpp"

template <typename T> struct ComponentStorage {
  SparseSet sparseSet;
  T components[50];

  void addComponent(int entityID, const T &component) {
    sparseSet.add(entityID);
    int ent_index = sparseSet.sparse[entityID];
    components[ent_index] = component;
  }

  void removeComponent(int entityID) {
    int remove_index = sparseSet.sparse[entityID];
    int last_valid_index = sparseSet.n - 1;

    if (remove_index != last_valid_index) {
      components[remove_index] = std::move(components[last_valid_index]);

      int last_entity_id = sparseSet.dense[last_valid_index];
      sparseSet.sparse[last_entity_id] = remove_index;
    }
    sparseSet.remove(entityID);
  }

  T *getComponent(int entityID) {
    int index = sparseSet.sparse[entityID];

    int containsCheck = sparseSet.contains(entityID);

    if (containsCheck == -1) {
      return nullptr;
    }

    return &components[index];
  }
};
