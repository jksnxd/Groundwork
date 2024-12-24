#include "SDL3/SDL_log.h"
#include "components/health.cpp"
#include "components/position.cpp"
#include "game/scene.cpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>

class VulkanEngine {
private:
  VkInstance instance;

public:
  void createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // this part is about getting the vkinstance setup with info from SDL
    // windowing
    uint32_t count_instance_extensions;
    const char *const *instance_extensions =
        SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);
    if (instance_extensions == NULL) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No extensions found");
    }
    int count_extensions = count_instance_extensions + 1;

#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif
    // allocate memory for all of the extensions, move instance_exteions into
    // extensions array
    const char **extensions =
        (const char **)SDL_malloc(count_extensions * sizeof(const char *));
    extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    SDL_memcpy(&extensions[1], instance_extensions,
               count_instance_extensions * sizeof(const char *));

    createInfo.enabledExtensionCount = count_extensions;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
      throw std::runtime_error("failed to create instance!");
    }

    SDL_free(extensions);
  }
};

int main() {
  /*Scene active_scene = Scene{};*/
  /**/
  /*active_scene.registry.addComponent(1, Position(5, 20, 4));*/
  /*active_scene.registry.addComponent(1, Health(100));*/
  /**/
  /*active_scene.registry.addComponent(9, Position(100, 200, 2));*/
  /*active_scene.registry.addComponent(9, Health(25));*/

  VulkanEngine vulkanEngine;

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window =
      SDL_CreateWindow("Title", 500, 600, SDL_WINDOW_FULLSCREEN);

  vulkanEngine.createInstance();

  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n",
                 SDL_GetError());
    return 1;
  }

  /*SDL_Delay(10000);*/

  SDL_DestroyWindow(window);
  SDL_Quit();
}
