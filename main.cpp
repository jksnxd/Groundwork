#include <iostream>

const int MAX = 12;
const int SPARSE_FACTOR = 2;
struct SparseSet {
  int dense[MAX];
  int sparse[MAX * SPARSE_FACTOR];

  SparseSet() {
	  for (int i=0; i<MAX*SPARSE_FACTOR;i++) {
		  sparse[i] = -1;
	  }
  }

  int n = 0;
  void add(int x) {
    dense[n] = x;
    sparse[x] = n;
    n++;
  }

  void remove(int x) {
    int index = contains(x);
    if (index == -1) {
	    return;
    }

    int last_element = dense[n - 1];
    dense[index] = last_element;
    sparse[last_element] = index;

    sparse[x] = -1;
    --n;
  }

  int contains(int x) {
    if (x >= MAX) {
	    return -1;
    }

    if (sparse[x] != -1 && sparse[x] < n && dense[sparse[x]] == x) {
	    return (sparse[x]);
    }

    return -1;
  }

  void print() {
    std::cout << "Dense: ";
    for (int i = 0; i < n; i++) {
      std::cout << dense[i];
    }
    std::cout << std::endl;

    std::cout << "Sparse: ";
    for (int i = 0; i < MAX; i++) {
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

  void addComponent(int entityID, const T& component) {
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

int main() {

  ComponentStorage<Health> healthStore;

  healthStore.addComponent(3, Health(50));
  healthStore.addComponent(9, Health(100));


  std::cout << healthStore.getComponent(9)->value << std::endl;

  std::cout << healthStore.getComponent(3)->value << std::endl;


}
