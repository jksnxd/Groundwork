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
		// dense = {3, 4, 5};
		// sparse = {0,0,0,0,1,2}
		int dense_location = sparse[x];
		int last_element = dense[n-1];

		if (x != last_element) {
			dense[dense_location] = last_element;
			sparse[last_element] = dense_location;
		} 

		dense[n-1] = 0;

		sparse[x] = 0;
		n--;

	}

	void print() {
		std::cout << "Dense: ";
		for (int i=0; i<MAX;i++) {
			std::cout << dense[i];
		}
		std::cout << std::endl;

		
		std::cout << "Sparse: ";
		for (int i=0; i<MAX*SPARSE_FACTOR;i++) {
			std::cout << sparse[i];
		}
		std::cout << std::endl;
	}
};

struct Health {
	int value;
	Health(int value=0) : value(value) {}
};

template <typename T>
struct ComponentStorage {
	SparseSet sparseSet;
	T components[50];

	
	void addComponent(int entityID, const T& component) {
		//insert entity into sparseset
		sparseSet.add(entityID);
		int ent_index = sparseSet.sparse[entityID];
		components[ent_index] = component;
	}

	T* getComponent(int entityID) {
		int index = sparseSet.sparse[entityID];
		return &components[index];
	}
};


// Lets implement ComponentStore
// I will allocate a ComponentStore for each type of Component type (Health, Transform, Position...etc)
// Each ComponentStore will hold a SparseSet that maps this relationship.
// the sparse array will hold the indicies/positions of the entity ids, dense will hold the entity ids.
// a component array will exist that holds the different versions of that component, this will be stored at a mappable index as well.




int main() {
	SparseSet s1 = SparseSet{};
	s1.add(2);
	s1.add(3);
	s1.add(9);

	ComponentStorage<Health> healthStore;
	healthStore.addComponent(3, Health(50));

	std::cout << healthStore.getComponent(3);

}
