#include <iostream>

const int MAX = 12;
const int SPARSE_FACTOR = 2;
struct SparseSet {
  int dense[MAX];
  int sparse[MAX * SPARSE_FACTOR];

  SparseSet() {
    for (int i = 0; i < MAX * SPARSE_FACTOR; i++) {
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
