#pragma once
#include "vulkan/vulkan.h"

#include <vector>

#include "vulkan_utils/swapchain_handler.h"

namespace VulkanUtils {
class TraditionalGraphicsPipeline {
 public:
  TraditionalGraphicsPipeline(const VkDevice _device, const SwapChainHandler& swapchainHandler);
  ~TraditionalGraphicsPipeline();

  VkPipeline getPipeline() { return graphicsPipeline; }
 private:
  VkShaderModule createShaderModule(const std::vector<char>& code);
  VkPipelineLayout pipelineLayout;

  VkPipeline graphicsPipeline;
  const VkDevice device;
};

}
