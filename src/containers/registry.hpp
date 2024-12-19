#include "componentstorage.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>

struct Registry {
public:
  template <typename T> void addComponent(int entityID, const T &);

  template <typename T> void removeCommponent(int entityID);

  template <typename T> T *getComponent(int entityID);

private:
  std::unordered_map<std::type_index, std::shared_ptr<void>> componentStorages;

  template <typename T> ComponentStorage<T> &getStorage();
};
