#pragma once
#include "vulkan/vulkan.h"

#include <vector>

#include "vulkan_utils/swapchain_handler.h"
#include "vulkan_utils/device_manager.h"
#include "vulkan_utils/vulkan_types.h"

namespace VulkanUtils {
class TraditionalGraphicsPipeline {
 public:
  TraditionalGraphicsPipeline(DeviceManager& devManager, const SwapChainHandler& swapchainHandler);
  ~TraditionalGraphicsPipeline();

  VkPipeline getPipeline() { return graphicsPipeline; }
  VkPipelineLayout getLayout() { return pipelineLayout; }
  void bindDescriptors(VkCommandBuffer commandBuffer) {
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0, // Position for global data
        1, 
        &staticDescriptorSet,
        0,
        nullptr);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        1, // Position for dynamic data
        1, 
        &dynamicDescriptorSet,
        0,
        nullptr);
  }

  // potentially dynamic refs
  void updateTextureDescriptorSet(
      VkImageView textureImageView,
      VkSampler textureSampler);
 private:
  const VkDevice device;

  VkShaderModule createShaderModule(const std::vector<char>& code);
  void createDescriptorPool();

  VkDescriptorSetLayout createDynamicDescriptorSetLayout();

  // One time always points to the same thing
  void updateStaticDescriptorSet();
  VkDescriptorSetLayout createStaticDescriptorSetLayout();

  void allocateDescriptorSets();

  VkDescriptorSetLayout staticDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet staticDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSetLayout dynamicDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet dynamicDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout;

  VkPipeline graphicsPipeline;

  // These should probably not be members of the pipeline
  SceneUBO* scene = nullptr;
  LightUBO* light = nullptr;

  VkBuffer sceneUniformBuffer;
  VkDeviceMemory sceneUniformMemory;
  VkBuffer lightUniformBuffer;
  VkDeviceMemory lightUniformMemory;

};

}
