#include "componentstorage.cpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

struct Registry {
public:
  template <typename T> void addComponent(int entityID, const T &component) {
    auto &storage = getStorage<T>();
    storage.addComponent(entityID, component);
  }

  template <typename T> void removeComponent(int entityID) {
    auto &storage = getStorage<T>();
    storage.removeComponent(entityID);
  }

  template <typename T> T *getComponent(int entityID) {
    auto &storage = getStorage<T>();
    return storage.getComponent(entityID);
  }

private:
  std::unordered_map<std::type_index, std::shared_ptr<void>> componentStorages;

  template <typename T> ComponentStorage<T> &getStorage() {
    std::type_index type_index = std::type_index(typeid(T));

    if (componentStorages.find(type_index) == componentStorages.end()) {
      componentStorages[type_index] = std::make_shared<ComponentStorage<T>>();
    }

    return *std::static_pointer_cast<ComponentStorage<T>>(
        componentStorages[type_index]);
  }
};
