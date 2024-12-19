#include "components/health.hpp"
#include "components/position.hpp"
#include "game/scene.hpp"

int main() {
  Scene active_scene = Scene{};

  active_scene.registry.addComponent(1, Position(5, 20, 4));
  active_scene.registry.addComponent(1, Health(100));

  active_scene.registry.addComponent(9, Position(100, 200, 2));
  active_scene.registry.addComponent(9, Health(25));
}
