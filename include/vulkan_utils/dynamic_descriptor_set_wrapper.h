#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <stdexcept>
#include <cstring>
#include "vulkan_utils/device_manager.h"

namespace VulkanUtils {

template <uint32_t NumFrames, class T>
class DescriptorSetWrapper {
 public:
  DescriptorSetWrapper(DeviceManager& devManager,
                        VkDescriptorPool descriptorPool,
                        VkDescriptorSetLayout descriptorSetLayout,
                        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        uint32_t bindingDst = 0);
  ~DescriptorSetWrapper();

  DescriptorSetWrapper(const DescriptorSetWrapper&) = delete;
  DescriptorSetWrapper& operator=(const DescriptorSetWrapper&) = delete;
  DescriptorSetWrapper(DescriptorSetWrapper&&) = default;
  DescriptorSetWrapper& operator=(DescriptorSetWrapper&&) = default;

  VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {
    return descriptorSets_[frameIndex];
  }

  void RecordInCommandBuffer(VkCommandBuffer cmdBuf);

  T& operator->() {
    if (!object)
      throw std::runtime_error("object should exist");
    return *object;
  }

 private:
  void copyToGPU(VkCommandBuffer cmdBuffer);

  void AllocateDescriptorSets(
      VkDescriptorPool descriptorPool,
      VkDescriptorSetLayout layout);

  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
  uint8_t* stagingMappedPtr = nullptr;

  VkBuffer gpuBuffer = VK_NULL_HANDLE;
  VkDeviceMemory gpuMemory = VK_NULL_HANDLE;

  std::array<VkDescriptorSet, NumFrames> descriptorSets{};

  static constexpr VkDeviceSize PER_FRAME_SIZE = sizeof(T);
  DeviceManager& devManager;

  uint32_t currentFrame = 0;

  T* object = nullptr;
};

}
