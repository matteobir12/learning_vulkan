#pragma once

#include "vulkan/vulkan.h"

#include <optional>
#include <vector>

#include "vulkan_utils/command_pool_wrapper.h"
#include "vulkan_utils/scoped_command_buffer.h"

namespace VulkanUtils {
class DeviceManager {
 public:
  DeviceManager(
      VkInstance _instance,
      VkSurfaceKHR surface,
      const std::vector<const char*>& requiredDeviceExtensions,
      const std::optional<std::vector<const char*>>& validationLayers = std::nullopt);

  ~DeviceManager();

  DeviceManager(const DeviceManager&) = delete;

  VkDevice getDevice() const { return device; }
  VkQueue getGraphicsQueue() const { return graphicsQueue; }
  VkQueue getPresentQueue() const { return presentQueue; }
  CommandPoolWrapper& getCommandPoolWrapper() { return *commandPoolWrapper; };

  VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }

  ScopedCommandBuffer createScopedCommandBuffer() {
    return ScopedCommandBuffer(device, transientPool->getCommandPool(), getGraphicsQueue());
  }

  // Buffer would need to be destroyed after... maybe investigate custom allocators?
  VkResult createBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer& buffer,
      VkDeviceMemory& bufferMemory);

  VkResult createVertexBuffer(
      VkDeviceSize size,
      VkBuffer& buffer,
      VkDeviceMemory& bufferMemory) {
    return createBuffer(
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        bufferMemory);
  }

  VkResult createIndexBuffer(
      VkDeviceSize size,
      VkBuffer& buffer,
      VkDeviceMemory& bufferMemory) {
    return createBuffer(
        size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        bufferMemory);
  }

  VkResult createUniformBuffer(
      VkDeviceSize size,
      VkBuffer& buffer,
      VkDeviceMemory& bufferMemory) {
    return createBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        bufferMemory);
  }

  VkResult createStagingBuffer(
      VkDeviceSize size,
      VkBuffer& buffer,
      VkDeviceMemory& bufferMemory) {
    return createBuffer(size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        buffer, 
        bufferMemory);
  }

  // finalLayout should probably be VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  // currently no form of texture streaming supported.. once on the gpu it doesn't
  // come off
  VkResult createImage(
      uint32_t width,
      uint32_t height,
      VkFormat format,  
      VkImageTiling tiling,
      VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkImage& image,
      VkDeviceMemory& imageMemory,
      VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  VkResult createImageView(
      VkImage image, 
      VkFormat format, 
      VkImageAspectFlags aspectFlags, 
      VkImageView& imageView);

void transitionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout);

 private:
  void pickPhysicalDevice(
      const VkSurfaceKHR surface,
      const std::vector<const char*>& requiredDeviceExtensions);
  void createLogicalDevice(
      const VkSurfaceKHR surface,
      const std::vector<const char*>& requiredDeviceExtensions,
      const std::optional<std::vector<const char*>>& validationLayers);

  VkInstance instance;

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;

  std::optional<CommandPoolWrapper> commandPoolWrapper;
  std::optional<CommandPoolWrapper> transientPool;

  VkQueue graphicsQueue;
  VkQueue presentQueue;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

inline QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
  QueueFamilyIndices indices;
  
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport)
      indices.presentFamily = i;

    if (indices.isComplete())
      break;

    i++;
  }

  return indices;
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

inline SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
      details.presentModes.resize(presentModeCount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

}
