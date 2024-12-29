const int MAX_FRAMES_IN_FLIGHT = 2;

#define NOMINMAX

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
#include <fstream>
#include <limits>



static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

class VulkanEngine {
private:
  VkInstance instance;
  VkPhysicalDevice pChosenDevice = VK_NULL_HANDLE;
  VkDevice logicalDevice;
  VkQueue graphicsQueue;
  VkQueue presentQueue;

  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;

  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  SDL_Window* window;

  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  bool framebufferResized = false;

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
      vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
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

      // Get surface capabilities
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

      // Get supported formats
      uint32_t formatCount;
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
      if (formatCount != 0) {
          details.formats.resize(formatCount);
          vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
      }

      // Get supported present modes
      uint32_t presentModeCount;
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
      if (presentModeCount != 0) {
          details.presentModes.resize(presentModeCount);
          vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
      }
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
      if (formats.empty()) {
          throw std::runtime_error("No surface formats available!");
      }
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

  VkShaderModule createShaderModule(const std::vector<char>& code) {
      VkShaderModuleCreateInfo createInfo{};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = code.size();
      createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

      VkShaderModule shaderModule;
      if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
          throw std::runtime_error("failed to create shader module!");
      }
      return shaderModule;
  }

  void createGraphicsPipeline() {
      auto vertShaderCode = readFile("C:/Users/silve/Documents/Groundwork/shaders/vert.spv");
      auto fragShaderCode = readFile("C:/Users/silve/Documents/Groundwork/shaders/frag.spv");
      
      VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
      VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

      VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
      vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
      vertShaderStageInfo.module = vertShaderModule;
      vertShaderStageInfo.pName = "main";

      VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
      fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      fragShaderStageInfo.module = fragShaderModule;
      fragShaderStageInfo.pName = "main";

      VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

      std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
      };

      VkPipelineDynamicStateCreateInfo dynamicState{};
      dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
      dynamicState.pDynamicStates = dynamicStates.data();


      VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
      vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
      vertexInputInfo.vertexBindingDescriptionCount = 0;
      vertexInputInfo.pVertexBindingDescriptions = nullptr;
      vertexInputInfo.vertexAttributeDescriptionCount = 0;
      vertexInputInfo.pVertexAttributeDescriptions = nullptr;

      VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
      inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
      inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      inputAssembly.primitiveRestartEnable = VK_FALSE;

      VkPipelineViewportStateCreateInfo viewportState{};
      viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
      viewportState.viewportCount = 1;
      viewportState.scissorCount = 1;

      VkPipelineRasterizationStateCreateInfo rasterizer{};
      rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
      rasterizer.depthClampEnable = VK_FALSE;
      rasterizer.rasterizerDiscardEnable = VK_FALSE;
      rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
      rasterizer.lineWidth = 1.0f;
      rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
      rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
      rasterizer.depthBiasEnable = VK_FALSE;

      VkPipelineMultisampleStateCreateInfo multisampling{};
      multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
      multisampling.sampleShadingEnable = VK_FALSE;
      multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

      VkPipelineColorBlendAttachmentState colorBlendAttachment{};
      colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = VK_FALSE;

      VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
      pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutInfo.setLayoutCount = 0;
      pipelineLayoutInfo.pushConstantRangeCount = 0;

      if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
          throw std::runtime_error("failed to create pipeline layout!");
      }

      VkPipelineColorBlendStateCreateInfo colorBlending{};
      colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      colorBlending.logicOpEnable = VK_FALSE;
      colorBlending.logicOp = VK_LOGIC_OP_COPY;
      colorBlending.attachmentCount = 1;
      colorBlending.pAttachments = &colorBlendAttachment;
      colorBlending.blendConstants[0] = 0.0f;
      colorBlending.blendConstants[1] = 0.0f;
      colorBlending.blendConstants[2] = 0.0f;
      colorBlending.blendConstants[3] = 0.0f;

      VkGraphicsPipelineCreateInfo pipelineInfo{};
      pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipelineInfo.stageCount = 2;
      pipelineInfo.pStages = shaderStages;
      pipelineInfo.pVertexInputState = &vertexInputInfo;
      pipelineInfo.pInputAssemblyState = &inputAssembly;
      pipelineInfo.pViewportState = &viewportState;
      pipelineInfo.pRasterizationState = &rasterizer;
      pipelineInfo.pMultisampleState = &multisampling;
      pipelineInfo.pColorBlendState = &colorBlending;
      pipelineInfo.pDynamicState = &dynamicState;
      pipelineInfo.layout = pipelineLayout;
      pipelineInfo.renderPass = renderPass;
      pipelineInfo.subpass = 0;
      pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

      if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
          throw std::runtime_error("failed to create graphics pipeline!");
      }

      vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
      vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
  }

  void createRenderPass() {
      VkAttachmentDescription colorAttachment{};
      colorAttachment.format = swapChainImageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VkAttachmentReference colorAttachmentRef{};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkSubpassDescription subpass{};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;

      VkSubpassDependency dependency{};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkRenderPassCreateInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = 1;
      renderPassInfo.pAttachments = &colorAttachment;
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      renderPassInfo.dependencyCount = 1;
      renderPassInfo.pDependencies = &dependency;

      if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
          throw std::runtime_error("failed to create render pass!");
      }
  }

  void cleanup() {
      cleanupSwapChain();
      vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
      vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
      vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

      for (auto imageView : swapChainImageViews) {
          vkDestroyImageView(logicalDevice, imageView, nullptr);
      }

      for (auto framebuffer : swapChainFramebuffers) {
          vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
      }

      vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
          vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
          vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
          vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
      }

      vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
      vkDestroyDevice(logicalDevice, nullptr);
  }

  void createFramebuffers() {
      if (swapChainImageViews.empty()) {
          throw std::runtime_error("No swap chain image views available!");
      }

      swapChainFramebuffers.resize(swapChainImageViews.size());

      for (size_t i = 0; i < swapChainImageViews.size(); i++) {
          VkImageView attachments[] = {
              swapChainImageViews[i]
          };

          VkFramebufferCreateInfo framebufferInfo{};
          framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
          framebufferInfo.renderPass = renderPass;
          framebufferInfo.attachmentCount = 1;
          framebufferInfo.pAttachments = attachments;
          framebufferInfo.width = swapChainExtent.width;
          framebufferInfo.height = swapChainExtent.height;
          framebufferInfo.layers = 1;

          if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
              throw std::runtime_error("failed to create framebuffer!");
          }

         
      }
  }

  void createCommandPool() {
      QueueFamilyIndices queueFamilyIndices = findQueueFamilies(pChosenDevice);

      VkCommandPoolCreateInfo poolInfo{};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

      if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
          throw std::runtime_error("failed to create command pool!");
      }
  }

  void createCommandBuffer() {
      commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

      if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
          throw std::runtime_error("failed to allocate command buffers!");
      }
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = 0;
      beginInfo.pInheritanceInfo = nullptr;

      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
          throw std::runtime_error("failed to begin recording command buffer!");
      }

      VkRenderPassBeginInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassInfo.renderPass = renderPass;
      renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
      renderPassInfo.renderArea.offset = { 0, 0 };
      renderPassInfo.renderArea.extent = swapChainExtent;

      VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

      VkViewport viewport{};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = static_cast<float>(swapChainExtent.width);
      viewport.height = static_cast<float>(swapChainExtent.height);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

      VkRect2D scissor{};
      scissor.offset = { 0, 0 };
      scissor.extent = swapChainExtent;
      vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
      vkCmdDraw(commandBuffer, 3, 1, 0, 0);

      vkCmdEndRenderPass(commandBuffer);

      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
          throw std::runtime_error("failed to record command buffer!");
      }
  }

  void createSyncObjects() {
      imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
      inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

      VkSemaphoreCreateInfo semaphoreInfo{};
      semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkFenceCreateInfo fenceInfo{};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
          if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
              vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
              vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

              throw std::runtime_error("failed to create synchronization objects for a frame!");
          }
      }
  }

  void drawFrame() {
      uint32_t currentFrame = 0;

      vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
      vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

      uint32_t imageIndex;
      VkResult result = vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX,
          imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

      if (result != VK_SUCCESS) {
          throw std::runtime_error("failed to acquire swap chain image!");
      }
      else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
          recreateSwapChain();
          return;
      }
      else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
          throw std::runtime_error("failed to acquire swap chain image!");
      }

      if (imageIndex >= swapChainFramebuffers.size()) {
          throw std::runtime_error("swap chain image index out of range!");
      }

      vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);


      vkResetCommandBuffer(commandBuffers[currentFrame], 0);
      recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
      VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = waitSemaphores;
      submitInfo.pWaitDstStageMask = waitStages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

      VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame]};
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = signalSemaphores;

      if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
          throw std::runtime_error("failed to submit draw command buffer!");
      }

      VkPresentInfoKHR presentInfo{};
      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = signalSemaphores;

      VkSwapchainKHR swapChains[] = { swapchain };
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = swapChains;
      presentInfo.pImageIndices = &imageIndex;

      
      result = vkQueuePresentKHR(presentQueue, &presentInfo);

      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          recreateSwapChain();
      }
      else if (result != VK_SUCCESS) {
          throw std::runtime_error("failed to present swap chain image!");
      }

      currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void cleanupSwapChain() {
      for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
          vkDestroyFramebuffer(logicalDevice, swapChainFramebuffers[i], nullptr);
      }

      for (size_t i = 0; i < swapChainImageViews.size(); i++) {
          vkDestroyImageView(logicalDevice, swapChainImageViews[i], nullptr);
      }

      vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
  }

  void recreateSwapChain() {
      vkDeviceWaitIdle(logicalDevice);

      cleanupSwapChain();

      setupSwapchain();
      createImageViews();
      createFramebuffers();
  }

  void init(SDL_Window* window) {
      createInstance();
      if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
          throw std::runtime_error("failed to create surface");
      }

      this->window = window;

      getPhysicalDevice();
      createLogicalDevice();
      setupSwapchain();
      createImageViews();
      createRenderPass();
      createGraphicsPipeline();
      createFramebuffers();
      createCommandPool();
      createCommandBuffer();
      createSyncObjects();
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

  SDL_Window* window = SDL_CreateWindow("Title", 500, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!window) {
      throw std::runtime_error(SDL_GetError());
  }

  vulkanEngine.init(window);

  SDL_Event e;
  bool window_open = true;
  while (window_open) {
      vulkanEngine.drawFrame();
      while (SDL_PollEvent(&e) != 0) {
          if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
              vulkanEngine.recreateSwapChain();
          }
          /*else if (e.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
              window_open = false;
          }*/
      }
  }

  // SDL_Delay(10000);

  SDL_DestroyWindow(window);
  SDL_Quit();
}
