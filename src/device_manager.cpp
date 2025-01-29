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

}
