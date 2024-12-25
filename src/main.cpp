#include "SDL3/SDL_log.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
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
  VkPhysicalDevice pChosenDevice = VK_NULL_HANDLE;
  VkDevice logicalDevice;
  VkQueue graphicsQueue;

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  

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

  bool isDeviceSuitable(VkPhysicalDevice device) {
      vkGetPhysicalDeviceProperties(device, &deviceProperties);
      vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

      std::cout << deviceProperties.deviceName << std::endl;
      std::cout << deviceFeatures.geometryShader << std::endl;
      std::cout << deviceProperties.deviceType << std::endl;

      return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceFeatures.geometryShader;
  }

  void getPhysicalDevice() {
    uint32_t pDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &pDeviceCount, nullptr);
    VkPhysicalDevice* pDevices = (VkPhysicalDevice*)malloc(pDeviceCount * sizeof(VkPhysicalDevice));
    VkPhysicalDeviceProperties* pDeviceProperties = (VkPhysicalDeviceProperties*)malloc(pDeviceCount * sizeof(VkPhysicalDeviceProperties));
    vkEnumeratePhysicalDevices(instance, &pDeviceCount, pDevices);


    for (int i=0; i<pDeviceCount;i++) {
      vkGetPhysicalDeviceProperties(pDevices[i], &pDeviceProperties[i]);
    }
    
    bool foundSuitableDevice = false;
    for (int i = 0; i < pDeviceCount; i++) {
        if (isDeviceSuitable(pDevices[i])) {
            pChosenDevice = pDevices[i];
            foundSuitableDevice = true;
            break;
        }
    }
    if (!foundSuitableDevice) {
        std::cout << "No suitable device found in getPhysicalDevice()" << std::endl;
    }

    free(pDevices);
    free(pDeviceProperties);
  }

  void createLogicalDevice() {
    int8_t index = -1;
    if (pChosenDevice == VK_NULL_HANDLE) {
      std::cout << "physical device is null, call getPhysicalDevice()" << std::endl;
      return;
    }

    std::cout << "physical device has been found" << std::endl;

    uint32_t propertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pChosenDevice, &propertyCount, nullptr);
    VkQueueFamilyProperties* pQueueFamilyProperties = (VkQueueFamilyProperties*)malloc(propertyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(pChosenDevice, &propertyCount, pQueueFamilyProperties);
    
    for (int i=0;i<propertyCount;i++) {
      if (pQueueFamilyProperties[i].queueFlags && VK_QUEUE_COMPUTE_BIT) {
        index = i;
      } else {
        std::cout << "No device property with VK_QUEUE_COMPUTE_BIT found" << std::endl;
      }
    }
    
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = index;
    queueInfo.queueCount = 1;
    float priority = 1.0;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.pQueueCreateInfos = &queueInfo;

    vkCreateDevice(pChosenDevice, &deviceInfo, nullptr, &logicalDevice);
    vkGetDeviceQueue(logicalDevice, 1, 0, &graphicsQueue);

    if (logicalDevice == VK_NULL_HANDLE) {
        std::cout << "The logical device has failed to be created" << std::endl;
    }

  }
};

void createConsole() {
  // SDL defines some overridden SDL_Main macro, so this is so I can do quick debugging with std::cout
  AllocConsole();
  freopen("CONIN$", "r", stdin);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
}

int main(int argc, char * argv[]) {

  //if (strcmp(argv[1], "--debug") == 0) {
  //     createConsole();
  // }
 
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
  vulkanEngine.getPhysicalDevice();
  vulkanEngine.createLogicalDevice();

  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n",
                 SDL_GetError());
    return 1;
  }

  SDL_Delay(10000);

  SDL_DestroyWindow(window);
  SDL_Quit();
}
