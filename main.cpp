#include <iostream>

const int MAX = 12;
const int SPARSE_FACTOR = 2;
struct SparseSet {
  int dense[MAX];
  int sparse[MAX * SPARSE_FACTOR];

  int n = 0;
  void add(int x) {
    dense[n] = x;
    sparse[x] = n;
    n++;
  }

  void remove(int x) {
    int dense_location = sparse[x];
    int last_element = dense[n - 1];

    if (x != last_element) {
      dense[dense_location] = last_element;
      sparse[last_element] = dense_location;
    }
    dense[n - 1] = 0;
    sparse[x] = 0;
    n--;
  }

  int contains(int x) {
    return dense[sparse[x]];
  }

  void print() {
    std::cout << "Dense: ";
    for (int i = 0; i < MAX; i++) {
      std::cout << dense[i];
    }
    std::cout << std::endl;

    std::cout << "Sparse: ";
    for (int i = 0; i < MAX * SPARSE_FACTOR; i++) {
      std::cout << sparse[i];
    }
    std::cout << std::endl;
  }
};

struct Health {
  int value;
  Health(int value = 0) : value(value) {}
  static const Health Invalid;
};

const Health Health::Invalid = Health(-1);

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
    int last_valid_index = sizeof(sparseSet.dense) - 1;

    if (remove_index != last_valid_index) {
      components[remove_index] = std::move(components[last_valid_index]);

      int last_entity_id = sparseSet.dense[last_valid_index];
      sparseSet.sparse[last_entity_id] = remove_index;
    }
    sparseSet.remove(entityID);
  }

  T *getComponent(int entityID) {

    if (!sparseSet.contains(entityID)) {
      return nullptr;
    }

    int index = sparseSet.sparse[entityID];
    return &components[index];
  }

};

int main() {
  SparseSet s1 = SparseSet{};
  s1.add(2);
  s1.add(3);
  s1.add(9);

  ComponentStorage<Health> healthStore;
  healthStore.addComponent(3, Health(50));

  std::cout << healthStore.getComponent(3) << std::endl;

  healthStore.removeComponent(3);
  if (healthStore.getComponent(3) != nullptr) {
    std::cout << healthStore.getComponent(3)->value << std::endl;
  } else {
    std::cout << "component doesnt exist" << std::endl;
  }
}
