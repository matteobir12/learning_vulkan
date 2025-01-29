#include "vulkan/vulkan.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <utility>
#include <vector>
#include <cstdint>

#include "vulkan_utils/window_and_surface_manager.h"
#include "vulkan_utils/instance_creator.h"
#include "vulkan_utils/device_manager.h"
#include "vulkan_utils/swapchain_handler.h"
#include "vulkan_utils/command_pool_wrapper.h"
#include "vulkan_utils/traditional_graphics_pipeline.h"
#include "vulkan_utils/sync_object_manager.h"

namespace {
constexpr uint32_t frameBufferCount = 2;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
}

class VulkanApplication {
 public:
  VulkanApplication()
    : instanceWrapper(windowManager, validationLayers),
      devManager(instanceWrapper.getInstance(), windowManager.getSurface(), deviceExtensions, validationLayers),
      swapchain(devManager, windowManager),
      commandPool(
          devManager.getDevice(),
          VulkanUtils::findQueueFamilies(
              devManager.getPhysicalDevice(),
              windowManager.getSurface()).graphicsFamily.value(),
          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
      traditionalGP(devManager.getDevice(), swapchain),
      syncObjects(devManager.getDevice(), frameBufferCount),
      commandBuffer(commandPool.allocateCommandBuffers(1).front())
  {}

  void run() {
    mainLoop();
  }

 private:
  #ifdef NDEBUG
      const bool enableValidationLayers = false;
  #else
      const bool enableValidationLayers = true;
  #endif

  VulkanUtils::WindowAndSurfaceManager windowManager;
  VulkanUtils::VulkanInstanceWrapper instanceWrapper;
  VulkanUtils::DeviceManager devManager;
  VulkanUtils::SwapChainHandler swapchain;
  VulkanUtils::CommandPoolWrapper commandPool;
  VulkanUtils::TraditionalGraphicsPipeline traditionalGP;
  VulkanUtils::SyncObjectsManager syncObjects;
  VkCommandBuffer commandBuffer; // Tempish

  uint32_t currentFrame = 0;

  void mainLoop() {
    while (!windowManager.windowShouldClose()) {
      windowManager.pollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(devManager.getDevice());
  }

  void recordCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("failed to begin recording command buffer!");
  
    auto renderPassInfo = swapchain.createRenderBeginInfo(imageIndex);
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, traditionalGP.getPipeline());
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    const auto& extent = swapchain.getExtent();
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
      throw std::runtime_error("failed to record command buffer!");
  }

  void drawFrame() {
    auto& inFlightFence = syncObjects.get(currentFrame).inFlight;
    vkWaitForFences(devManager.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(devManager.getDevice(), 1, &inFlightFence);
    uint32_t imageIndex;
    auto& new_img_objects = syncObjects.get(imageIndex);
    vkAcquireNextImageKHR(devManager.getDevice(), swapchain.getSwapChain(), UINT64_MAX, new_img_objects.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {new_img_objects.imageAvailable};
    VkSemaphore signalSemaphores[] = {new_img_objects.renderFinished};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(devManager.getGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
      throw std::runtime_error("failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapchain.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(devManager.getPresentQueue(), &presentInfo);
  }
};

int main() {
  VulkanApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}