#include "components/health.cpp"
#include "components/position.cpp"
#include "game/scene.cpp"
#include <SDL3/SDL.h>

int main() {
  Scene active_scene = Scene{};

  active_scene.registry.addComponent(1, Position(5, 20, 4));
  active_scene.registry.addComponent(1, Health(100));

  active_scene.registry.addComponent(9, Position(100, 200, 2));
  active_scene.registry.addComponent(9, Health(25));

  SDL_Init(SDL_INIT_AUDIO);
}
