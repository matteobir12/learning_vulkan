#include "vulkan_utils/dynamic_descriptor_set_wrapper.h"

namespace VulkanUtils {

template <uint32_t NumFrames, class T>
DescriptorSetWrapper<NumFrames, T>::DescriptorSetWrapper(
        DeviceManager& devManager,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        VkBufferUsageFlags usageFlags,
        uint32_t bindingDst)
    : devManager(devManager)
{
    VkDeviceSize totalSize = PER_FRAME_SIZE * NumFrames;

    devManager_.createBuffer(
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    vkMapMemory(device, stagingMemory, 0, totalSize, 0, reinterpret_cast<void**>(&stagingMappedPtr));
    if (!stagingMappedPtr)
      throw std::runtime_error("Failed to map staging buffer memory!");

    devManager_.createBuffer(
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageFlags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        gpuBuffer,
        gpuMemory);

    allocateDescriptorSets(descriptorPool, descriptorSetLayout, bindingDst);
}

template <uint32_t NumFrames, class T>
DescriptorSetWrapper<NumFrames, T>::~DescriptorSetWrapper() {
  if (stagingMappedPtr) {
    vkUnmapMemory(device, stagingMemory);
    stagingMappedPtr = nullptr;
  }
  if (stagingBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    stagingBuffer = VK_NULL_HANDLE;
  }
  if (stagingMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, stagingMemory, nullptr);
    stagingMemory = VK_NULL_HANDLE;
  }

  if (gpuBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, gpuBuffer, nullptr);
    gpuBuffer = VK_NULL_HANDLE;
  }
  if (gpuMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device_, gpuMemory, nullptr);
    gpuMemory = VK_NULL_HANDLE;
  }
}

template <uint32_t NumFrames, class T>
void DescriptorSetWrapper<NumFrames, T>::copyToGPU(VkCommandBuffer cmdBuffer) {
  VkBufferCopy region{};
  region.srcOffset = frameIndex * PER_FRAME_SIZE;
  region.dstOffset = frameIndex * PER_FRAME_SIZE;
  region.size = PER_FRAME_SIZE;

  vkCmdCopyBuffer(cmdBuffer, stagingBuffer, gpuBuffer, 1, &region);
  // barrier?
}

template <uint32_t NumFrames, class T>
void DescriptorSetWrapper<NumFrames, T>::AllocateDescriptorSets(
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout layout) {
  std::array<VkDescriptorSetLayout, NumFrames> layouts;
  layouts.fill(layout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = NumFrames;
  allocInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(devManager.getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate descriptor sets!");

  std::array<VkWriteDescriptorSet, NumFrames> writes{};
  std::array<VkDescriptorBufferInfo, NumFrames> bufferInfos{};

  for (uint32_t i = 0; i < NumFrames; i++) {
    bufferInfos[i].buffer = gpuBuffer;
    bufferInfos[i].offset = i * PER_FRAME_SIZE;
    bufferInfos[i].range = PER_FRAME_SIZE;

    writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[i].dstSet = descriptorSets[i];
    writes[i].dstBinding = 0;
    writes[i].dstArrayElement = 0;

    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[i].descriptorCount = 1;
    writes[i].pBufferInfo = &bufferInfos[i];
  }

  vkUpdateDescriptorSets(devManager.getDevice(), NumFrames, &writes.data(), 0, nullptr);
}

template <uint32_t NumFrames, class T>
void DescriptorSetWrapper<NumFrames, T>::RecordInCommandBuffer(VkCommandBuffer cmdBuf) {
  if (!object) {
    std::cout << "expected object to be valid" << std::endl;
    return;
  }

  VkDeviceSize offset = currentFrame * PER_FRAME_SIZE;
  std::memcpy(stagingMappedPtr + offset, object, sizeof(T));

  copyToGPU(currentFrame, cmdBuf);

  uint32_t dynamicOffset = static_cast<uint32_t>(currentFrame * sizeof(T));

  vkCmdBindDescriptorSets(
      cmdBuf,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      0,
      1,
      &descriptorSets[currentFrame],
      1,
      &dynamicOffset);

  currentFrame = currentFrame % NumFrames;
}

}