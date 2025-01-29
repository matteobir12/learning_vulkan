#pragma once

#include "vulkan/vulkan.h"

#include <vector>
#include <stdexcept>

namespace VulkanUtils {

struct FrameSyncObjects {
  VkSemaphore imageAvailable;
  VkSemaphore renderFinished;
  VkFence inFlight;
};

class SyncObjectsManager {
public:
    SyncObjectsManager(const VkDevice _device, const int _maxFramesInFlight)
      : device(_device), maxFramesInFlight(_maxFramesInFlight) 
    {
      syncObjects.resize(maxFramesInFlight);
      for (int i = 0; i < maxFramesInFlight; ++i)
        createSyncObjects(syncObjects[i]);
    }

    ~SyncObjectsManager() {
      for (auto& s : syncObjects) {
        vkDestroySemaphore(device, s.imageAvailable, nullptr);
        vkDestroySemaphore(device, s.renderFinished, nullptr);
        vkDestroyFence(device, s.inFlight, nullptr);
      }
    }

    FrameSyncObjects& get(int frameIndex) {
      return syncObjects[frameIndex];
    }

 private:
  void createSyncObjects(FrameSyncObjects& frameSync) {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameSync.imageAvailable) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frameSync.renderFinished) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &frameSync.inFlight) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create sync objects!");
    }
  }

 private:
  VkDevice device;
  int maxFramesInFlight;
  std::vector<FrameSyncObjects> syncObjects;
};

}
