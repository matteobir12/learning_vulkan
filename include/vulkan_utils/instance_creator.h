#pragma once

#include <array>
#include <iostream>
#include <optional>
#include <vector>

#include "vulkan/vulkan.h"
#include "vulkan_utils/window_and_surface_manager.h"

namespace VulkanUtils {

class VulkanInstanceWrapper {
 public:
  VulkanInstanceWrapper(
      VulkanUtils::WindowAndSurfaceManager& windowManager,
      const std::optional<std::vector<const char*>>& validationLayers = std::nullopt);
  ~VulkanInstanceWrapper();

  inline VkInstance getInstance() { return instance; }

  VulkanInstanceWrapper(const VulkanInstanceWrapper&) = delete;
 private:
  bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);
  void setupDebugMessenger();

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT /*  messageType */,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* /* pUserData */)
  {
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
      std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }

  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  const bool enableValidationLayers;
};

}
