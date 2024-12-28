#include "SDL3/SDL_log.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "components/health.cpp"
#include "components/position.cpp"
#include "game/scene.cpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>

class VulkanEngine {
private:
  VkInstance instance;
  VkPhysicalDevice pChosenDevice = VK_NULL_HANDLE;
  VkDevice logicalDevice;
  VkQueue graphicsQueue;

  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;


  SDL_Window* window;

  struct QueueFamilyIndices {
      std::optional<uint32_t> graphicsFamily;
      std::optional<uint32_t> presentFamily;

      bool isComplete() {
          return graphicsFamily.has_value() && presentFamily.has_value();
      }
  };

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
      QueueFamilyIndices indices;

      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

      for (uint32_t i = 0; i < queueFamilyCount; i++) {
          if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
              indices.graphicsFamily = i;
          }

          VkBool32 presentSupport = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
          if (presentSupport) {
              indices.presentFamily = i;
          }

          if (indices.isComplete()) break;
      }

      return indices;
  }

  struct SwapChainSupportDetails {
      VkSurfaceCapabilitiesKHR capabilities;
      std::vector<VkSurfaceFormatKHR> formats;
      std::vector<VkPresentModeKHR> presentModes;
  };
  

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

    std::cout << *instance_extensions << std::endl;

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
      QueueFamilyIndices indices = findQueueFamilies(device);
      vkGetPhysicalDeviceProperties(device, &deviceProperties);
      vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

      return indices.isComplete() &&
          deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
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
      if (pChosenDevice == VK_NULL_HANDLE) {
          throw std::runtime_error("physical device is null");
      }

      QueueFamilyIndices indices = findQueueFamilies(pChosenDevice);

      std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
      std::set<uint32_t> uniqueQueueFamilies = {
          indices.graphicsFamily.value(),
          indices.presentFamily.value()
      };

      float queuePriority = 1.0f;
      for (uint32_t queueFamily : uniqueQueueFamilies) {
          VkDeviceQueueCreateInfo queueCreateInfo{};
          queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
          queueCreateInfo.queueFamilyIndex = queueFamily;
          queueCreateInfo.queueCount = 1;
          queueCreateInfo.pQueuePriorities = &queuePriority;
          queueCreateInfos.push_back(queueCreateInfo);
      }

      const std::vector<const char*> deviceExtensions = {
          VK_KHR_SWAPCHAIN_EXTENSION_NAME
      };

      VkDeviceCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
      createInfo.pQueueCreateInfos = queueCreateInfos.data();
      createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
      createInfo.ppEnabledExtensionNames = deviceExtensions.data();
      createInfo.pEnabledFeatures = &deviceFeatures;

      if (vkCreateDevice(pChosenDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
          throw std::runtime_error("failed to create logical device");
      }

      vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
      for (const auto& availableFormat : availableFormats) {
          if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
              return availableFormat;
          }
      }

      return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
      return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
      for (const auto& availablePresentMode : availablePresentModes) {
          if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
              return availablePresentMode;
          }
      }

      return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
      SwapChainSupportDetails details;

      return details;
  }

  void setupSwapchain() {
      QueueFamilyIndices indices = findQueueFamilies(pChosenDevice);

      VkSurfaceCapabilitiesKHR capabilities;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pChosenDevice, surface, &capabilities);

      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(pChosenDevice, surface, &formatCount, nullptr);
      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(pChosenDevice, surface, &formatCount, formats.data());

      uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

      VkSwapchainCreateInfoKHR swapInfo{};
      swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      swapInfo.surface = surface;
      swapInfo.minImageCount = capabilities.minImageCount;
      swapInfo.imageFormat = formats[0].format;
      swapInfo.imageColorSpace = formats[0].colorSpace;
      swapInfo.imageExtent = capabilities.currentExtent;
      swapInfo.imageArrayLayers = 1;
      swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

      if (indices.graphicsFamily != indices.presentFamily) {
          swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
          swapInfo.queueFamilyIndexCount = 2;
          swapInfo.pQueueFamilyIndices = queueFamilyIndices;
      }
      else {
          swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      }

      swapInfo.preTransform = capabilities.currentTransform;
      swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
      swapInfo.clipped = VK_TRUE;

      if (vkCreateSwapchainKHR(logicalDevice, &swapInfo, nullptr, &swapchain) != VK_SUCCESS) {
          throw std::runtime_error("failed to create swap chain!");
      }

      uint32_t imageCount = capabilities.minImageCount;
      vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
      swapChainImages.resize(imageCount);
      vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapChainImages.data());

      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pChosenDevice);
      VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
      swapChainImageFormat = surfaceFormat.format;
      VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
      swapChainExtent = extent;
  }

  void createImageViews() {
      swapChainImageViews.resize(swapChainImages.size());
      for (size_t i = 0; i < swapChainImages.size(); i++) {
          VkImageViewCreateInfo createInfo{};
          createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
          createInfo.image = swapChainImages[i];
          createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
          createInfo.format = swapChainImageFormat;
          createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
          createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
          createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
          createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
          createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          createInfo.subresourceRange.baseMipLevel = 0;
          createInfo.subresourceRange.levelCount = 1;
          createInfo.subresourceRange.baseArrayLayer = 0;
          createInfo.subresourceRange.layerCount = 1;

          if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
              throw std::runtime_error("failed to create image views!");
          }
      }
  }

  void init(SDL_Window* window) {
      createInstance();
      if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
          throw std::runtime_error("failed to create surface");
      }

      window = window;

      getPhysicalDevice();
      createLogicalDevice();
      setupSwapchain();
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

  SDL_Window* window = SDL_CreateWindow("Title", 500, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN);
  if (!window) {
      throw std::runtime_error(SDL_GetError());
  }

  vulkanEngine.init(window);

  // SDL_Delay(10000);

  SDL_DestroyWindow(window);
  SDL_Quit();
}
