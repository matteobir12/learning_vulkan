#include "vulkan/vulkan.h"
#include "glm/glm.hpp"

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
#include "vulkan_utils/traditional_graphics_pipeline.h"
#include "vulkan_utils/sync_object_manager.h"
#include "vulkan_utils/vulkan_types.h"
#include "graphics_types.h"

namespace {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Careful changing. Currently Swapchain img count is hardcoded to min + 1
constexpr uint32_t maxInFlightFrameCount = 2;

void createCubeModel(VulkanUtils::DeviceManager& devManager, std::vector<VulkanUtils::VulkanModel>& models) {
  static const std::vector<GraphicsTypes::Vertex> cubeVertices = {
      { {-0.5f, -0.5f, -0.5f} },
      { {-0.5f, -0.5f,  0.5f} },
      { {-0.5f,  0.5f,  0.5f} },
      { {-0.5f,  0.5f, -0.5f} },
      { { 0.5f, -0.5f, -0.5f} },
      { { 0.5f, -0.5f,  0.5f} },
      { { 0.5f,  0.5f,  0.5f} },
      { { 0.5f,  0.5f, -0.5f} }
  };

  static const std::vector<uint32_t> cubeIndices = {
      1, 2, 6,  6, 5, 1,
      0, 3, 7,  7, 4, 0,
      0, 1, 2,  2, 3, 0,
      4, 7, 6,  6, 5, 4,
      3, 2, 6,  6, 7, 3,
      0, 1, 5,  5, 4, 0,
  };

  models.emplace_back(devManager, cubeVertices, cubeIndices);
}

}

class VulkanApplication {
 public:
  VulkanApplication()
    : instanceWrapper(windowManager, validationLayers),
      devManager(instanceWrapper.getInstance(), windowManager.getSurface(), deviceExtensions, validationLayers),
      swapchain(devManager, windowManager),
      traditionalGP(devManager, swapchain),
      syncObjects(devManager, maxInFlightFrameCount)
  {
    createCubeModel(devManager, models);
    VulkanUtils::createDummyTexture(devManager, dummyImage, dummyMemory, dummyImageView, dummySampler);
  }

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
  VulkanUtils::TraditionalGraphicsPipeline traditionalGP;
  VulkanUtils::SyncObjectsManager syncObjects;

  std::vector<VulkanUtils::VulkanModel> models;

  VkImage dummyImage;
  VkDeviceMemory dummyMemory;
  VkImageView dummyImageView;
  VkSampler dummySampler;

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

    traditionalGP.bindDescriptors(commandBuffer);

    for (const auto& model : models) {
      if (model.hasTexture) {
        // Bind texture. Technically slower and can add overhead if done many times a frame (many materials on many models).
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            traditionalGP.getLayout(),
            1, // position for textures
            1,
            &model.descriptorSet,
            0,
            nullptr);
      } else {
        traditionalGP.updateTextureDescriptorSet(dummyImageView, dummySampler);
      }

      VulkanUtils::PerModelPushConstants modelPushes{
          model.model_matrix,
          static_cast<int>(model.hasTexture),
          model.color
      };
      // could calc the full mvp here then push that. Not really sure which would be faster
      vkCmdPushConstants(
          commandBuffer,
          traditionalGP.getLayout(),
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          0,
          sizeof(VulkanUtils::PerModelPushConstants),
          &modelPushes);

      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer, offsets);
      vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(commandBuffer, model.indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
      throw std::runtime_error("failed to record command buffer!");
  }

  void drawFrame() {
    auto& imageSyncObjects = syncObjects.get(currentFrame);

    vkWaitForFences(devManager.getDevice(), 1, &imageSyncObjects.inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(devManager.getDevice(), 1, &imageSyncObjects.inFlight);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(
        devManager.getDevice(),
        swapchain.getSwapChain(),
        UINT64_MAX,
        imageSyncObjects.imageAvailable,
        VK_NULL_HANDLE,
        &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      throw std::runtime_error("Handle swapchain out of date!");
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    // switch to one buff per frame at some point
    vkResetCommandBuffer(imageSyncObjects.commandBuffer, 0);
    recordCommandBuffer(imageSyncObjects.commandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageSyncObjects.imageAvailable};
    VkSemaphore signalSemaphores[] = {imageSyncObjects.renderFinished};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &imageSyncObjects.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(devManager.getGraphicsQueue(), 1, &submitInfo, imageSyncObjects.inFlight) != VK_SUCCESS)
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
    result = vkQueuePresentKHR(devManager.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
      throw std::runtime_error("Issue with present");

    currentFrame = (currentFrame + 1) % maxInFlightFrameCount;
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