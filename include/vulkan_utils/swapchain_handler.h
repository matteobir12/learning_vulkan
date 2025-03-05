#pragma once

#include "vulkan/vulkan.h"

#include <stdexcept>
#include <vector>

#include "vulkan_utils/device_manager.h"
#include "vulkan_utils/window_and_surface_manager.h"


namespace VulkanUtils {
class SwapChainHandler {
 public:
  SwapChainHandler(const DeviceManager& dev_manager, const WindowAndSurfaceManager& window);
  ~SwapChainHandler();

  VkRenderPass getRenderPass() const { return renderPass; }

  VkSwapchainKHR getSwapChain() {return swapchain; }

  VkRenderPassBeginInfo createRenderBeginInfo(const uint32_t imageIndex) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;

    return renderPassInfo;
  }

  const VkExtent2D& getExtent() const { return swapchainExtent; }

  uint32_t getImageCount() const { return scImageCount; }

  SwapChainHandler(const SwapChainHandler&) = delete;
 private:
  void createSwapChain(const VkPhysicalDevice physicalDevice, const WindowAndSurfaceManager& window);
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  VkExtent2D chooseSwapExtent(const WindowAndSurfaceManager& window, const VkSurfaceCapabilitiesKHR& capabilities);

  VkSwapchainKHR swapchain;
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;
  std::vector<VkImageView> swapchainImageViews;
  VkRenderPass renderPass;

  const VkDevice device;

  std::vector<VkFramebuffer> swapchainFramebuffers;
  uint32_t scImageCount = 0;
};

}
