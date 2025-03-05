#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <iostream>

namespace VulkanUtils {

// This kinda sucks... I don't know why I wrote it
class ScopedCommandBuffer {
 public:
  ScopedCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue)
    : device(device), commandPool(commandPool), queue(queue)
  {
      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.commandPool = commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = 1;

      if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
          throw std::runtime_error("Failed to allocate command buffer!");
      }

      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
          throw std::runtime_error("Failed to begin recording command buffer!");
      }
  }

  ~ScopedCommandBuffer() {
    VkResult endResult = vkEndCommandBuffer(commandBuffer);
    if (endResult != VK_SUCCESS) {
      std::cerr << "Warning: Failed to end command buffer (error code: " << endResult << ")." << std::endl;
    } else if (std::uncaught_exceptions() == 0) {
      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;

      VkResult submitResult = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
      if (submitResult != VK_SUCCESS)
        std::cerr << "Warning: Failed to submit command buffer (error code: " << submitResult << ")." << std::endl;

      // This blocks...
      vkQueueWaitIdle(queue);
    }

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
  }

  VkCommandBuffer get() const { return commandBuffer; }

  ScopedCommandBuffer(const ScopedCommandBuffer&) = delete;
  ScopedCommandBuffer& operator=(const ScopedCommandBuffer&) = delete;

  ScopedCommandBuffer(ScopedCommandBuffer&& other) noexcept
    : device(other.device), commandPool(other.commandPool), queue(other.queue), commandBuffer(other.commandBuffer)
  {
    other.commandBuffer = VK_NULL_HANDLE;
  }

  ScopedCommandBuffer& operator=(ScopedCommandBuffer&& other) noexcept {
    if (this != &other) {
      if (commandBuffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
      }
      device = other.device;
      commandPool = other.commandPool;
      queue = other.queue;
      commandBuffer = other.commandBuffer;
      other.commandBuffer = VK_NULL_HANDLE;
    }
    return *this;
  }

 private:
  VkDevice device;
  VkCommandPool commandPool;
  VkQueue queue;
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
};

} // namespace VulkanUtils
