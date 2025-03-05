#include "vulkan_utils/device_manager.h"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

namespace VulkanUtils {
namespace {
bool checkDeviceExtensionSupport(const VkPhysicalDevice device, const std::vector<const char*>& requiredDeviceExtensions) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

  for (const auto& extension : availableExtensions)
    requiredExtensions.erase(extension.extensionName);

  return requiredExtensions.empty();
}

bool isDeviceSuitable(
    const VkPhysicalDevice device,
    const VkSurfaceKHR surface,
    const std::vector<const char*>& requiredDeviceExtensions) {
  QueueFamilyIndices indices = findQueueFamilies(device, surface);

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  const bool extensionsSupported = checkDeviceExtensionSupport(device, requiredDeviceExtensions);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  // WSL only exposes one graphics card, normally the integrated one
  return /* deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && */
      indices.isComplete() &&
      extensionsSupported &&
      swapChainAdequate;
}

uint32_t findMemoryType(
    VkPhysicalDevice physicalDevice,
    const uint32_t typeFilter,
    const VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;

  throw std::runtime_error("Failed to find suitable memory type!");
}

}

DeviceManager::DeviceManager(
    const VkInstance _instance,
    const VkSurfaceKHR surface,
    const std::vector<const char*>& requiredDeviceExtensions,
    const std::optional<std::vector<const char*>>& validationLayers)
 : instance(_instance)
{
  pickPhysicalDevice(surface, requiredDeviceExtensions);
  createLogicalDevice(surface, requiredDeviceExtensions, validationLayers);

  const auto graphicsQueue =
      findQueueFamilies(physicalDevice, surface).graphicsFamily.value();
  commandPoolWrapper.emplace(
      device,
      graphicsQueue,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  transientPool.emplace(
      device,
      graphicsQueue,
      VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
}

DeviceManager::~DeviceManager() {
  if (device != VK_NULL_HANDLE)
    vkDestroyDevice(device, nullptr);
}

void DeviceManager::pickPhysicalDevice(const VkSurfaceKHR surface, const std::vector<const char*>& requiredDeviceExtensions) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0)
    throw std::runtime_error("failed to find GPUs with Vulkan support!");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  std::cout << "devs: " << deviceCount << std::endl;

  for (const VkPhysicalDevice device : devices) {
    if (isDeviceSuitable(device, surface, requiredDeviceExtensions)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE)
    throw std::runtime_error("failed to find a suitable GPU!");
}

void DeviceManager::createLogicalDevice(
    const VkSurfaceKHR surface,
    const std::vector<const char*>& requiredDeviceExtensions,
    const std::optional<std::vector<const char*>>& validationLayers) {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // Currently no custom features required
  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
  createInfo.pEnabledFeatures = &deviceFeatures;

  // Largely depricated. Probably can safely be removed
  if (validationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers->size());
    createInfo.ppEnabledLayerNames = validationLayers->data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

VkResult DeviceManager::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // There are other modes

  VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
  if (result != VK_SUCCESS)
    return result;

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      physicalDevice,
      memRequirements.memoryTypeBits,
      properties);

  result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
  if (result != VK_SUCCESS) {
    vkDestroyBuffer(device, buffer, nullptr);
    return result;
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
  return VK_SUCCESS;
}

VkResult DeviceManager::createImage(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory,
    VkImageLayout finalLayout) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1; // for now no mipmapping. Probably change in the future
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags = 0; // Optional

  VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
  if (result != VK_SUCCESS)
    return result;

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      physicalDevice,
      memRequirements.memoryTypeBits,
      properties);

  result = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
  if (result != VK_SUCCESS) {
    vkDestroyImage(device, image, nullptr);
    return result;
  }

  vkBindImageMemory(device, image, imageMemory, 0);

 if (finalLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
    transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, finalLayout);
    vkDeviceWaitIdle(device); // band aid lol
 }

  return VK_SUCCESS;
}

VkResult DeviceManager::createImageView(
    VkImage image, 
    VkFormat format, 
    VkImageAspectFlags aspectFlags, 
    VkImageView& imageView) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  return vkCreateImageView(device, &viewInfo, nullptr, &imageView);
}

void DeviceManager::transitionImageLayout(
    VkImage image,
    VkFormat /* format // necessary if there's ever other image formats*/,
    VkImageLayout oldLayout,
    VkImageLayout newLayout) {
  ScopedCommandBuffer commandBuffer = createScopedCommandBuffer();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    // Transitioning from undefined to shader read-only
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(
      commandBuffer.get(),
      sourceStage,
      destinationStage,
      0,
      0,
      nullptr,
      0,
      nullptr,
      1,
      &barrier);
}

}
