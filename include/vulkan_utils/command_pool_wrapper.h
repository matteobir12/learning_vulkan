#pragma once

#include "vulkan/vulkan.h"

#include <functional>
#include <stdexcept>
#include <vector>

namespace VulkanUtils {
class CommandPoolWrapper {
 public:
  CommandPoolWrapper(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0)
   : device(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
      throw std::runtime_error("Failed to create command pool!");
  }

  CommandPoolWrapper(CommandPoolWrapper&) = delete;
  ~CommandPoolWrapper() {
    if (commandPool != VK_NULL_HANDLE)
      vkDestroyCommandPool(device, commandPool, nullptr);
  }

  std::vector<VkCommandBuffer> allocateCommandBuffers(
      uint32_t count, 
      VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = count;

    std::vector<VkCommandBuffer> commandBuffers(count);
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
      throw std::runtime_error("Failed to allocate command buffers!");

    return commandBuffers;
    }

  void resetPool(VkCommandPoolResetFlags flags = 0) {
    vkResetCommandPool(device, commandPool, flags);
  }

  VkCommandPool getCommandPool() const { return commandPool; }

 private:
  const VkDevice device;
  VkCommandPool commandPool = VK_NULL_HANDLE;
};

}
