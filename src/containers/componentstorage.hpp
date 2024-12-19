#include "sparseset.cpp"

template <typename T> struct ComponentStorage {
  SparseSet sparseSet;
  T components[50];

  void addComponent(int entityID, const T &component);

  void removeComponent(int entityID);

  T *getComponent(int entityID);
};
