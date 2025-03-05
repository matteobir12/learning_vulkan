#pragma once

#include <glm/glm.hpp>
#include "vulkan/vulkan.h"

#include <array>
#include <stdexcept>
#include <cstring>

#include "vulkan_utils/device_manager.h"
#include "graphics_types.h"

namespace VulkanUtils {
// I think vecs need to be 16 byte aligned? 
struct alignas(16) SceneUBO {
    glm::mat4 uMat;
    glm::mat4 uWorld;
    glm::mat4 uWorldInverseTranspose;
    glm::vec3 uViewerWorldPosition;
};

struct alignas(16) LightUBO {
    glm::vec3 uLightPosition[3];
    glm::vec3 uLightDirection[3];
    glm::vec3 uLightColor[3];
    glm::bvec3 uLightIsOn;
    glm::bvec3 uLightIsDirectional;
    float uShininess;
    float lightCutoff;
    glm::vec3 ambientLight;
    glm::vec3 specColor;
};

inline void createDummyTexture(
    DeviceManager& devManager,
    VkImage& dummyImage,
    VkDeviceMemory& dummyMemory,
    VkImageView& dummyImageView,
    VkSampler& dummySampler) {
  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageCreateInfo.extent = {1, 1, 1};
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

  vkCreateImage(devManager.getDevice(), &imageCreateInfo, nullptr, &dummyImage);

  const VkResult imgCreateResult = devManager.createImage(
      1,
      1,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      dummyImage,
      dummyMemory);
  if (imgCreateResult != VK_SUCCESS)
      throw std::runtime_error("Failed to create dummy image!");

  const VkResult imgCreateViewResult = devManager.createImageView(
      dummyImage,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_IMAGE_ASPECT_COLOR_BIT,
      dummyImageView);

  if (imgCreateViewResult != VK_SUCCESS)
      throw std::runtime_error("Failed to create dummy image view!");

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = dummyImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.layerCount = 1;

  vkCreateImageView(devManager.getDevice(), &viewInfo, nullptr, &dummyImageView);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_NEAREST;
  samplerInfo.minFilter = VK_FILTER_NEAREST;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  
  if (vkCreateSampler(devManager.getDevice(), &samplerInfo, nullptr, &dummySampler) != VK_SUCCESS)
    throw std::runtime_error("Failed to create dummy VkSampler!");
}

// Probably requires some kind of alignment... we'll see what works
struct alignas(16) PerModelPushConstants {
    glm::mat4 uModel;
    int uUseTexture; // bool
    glm::vec4 color;
};

struct VulkanModel {
    const bool hasTexture = false; // Temp
    const VkDescriptorSet descriptorSet = VK_NULL_HANDLE; // Temp
    glm::mat4 model_matrix{}; // zero init should be identity?
    glm::vec4 color{0, 1, 0, 1}; // If no texture

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    uint32_t indexCount = 0;

    VulkanModel(const VulkanModel&) = delete;
    VulkanModel& operator=(const VulkanModel&) = delete;
    VulkanModel(VulkanModel&& other) noexcept
      : vertexBuffer(other.vertexBuffer),
        vertexBufferMemory(other.vertexBufferMemory),
        indexBuffer(other.indexBuffer),
        indexBufferMemory(other.indexBufferMemory),
        indexCount(other.indexCount),
        device(other.device) {
      other.vertexBuffer = VK_NULL_HANDLE;
      other.vertexBufferMemory = VK_NULL_HANDLE;
      other.indexBuffer = VK_NULL_HANDLE;
      other.indexBufferMemory = VK_NULL_HANDLE;
    }

    VulkanModel(
        DeviceManager& devManager,
        const std::vector<GraphicsTypes::Vertex>& vertices,
        const std::vector<uint32_t>& indices)
          : indexCount(static_cast<uint32_t>(indices.size())),
            device(devManager.getDevice())
      {
      const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(GraphicsTypes::Vertex);
      devManager.createVertexBuffer(vertexBufferSize, vertexBuffer, vertexBufferMemory);
      {
        void* mappedData = nullptr;
        vkMapMemory(devManager.getDevice(), vertexBufferMemory, 0, vertexBufferSize, 0, &mappedData);
        memcpy(mappedData, vertices.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(devManager.getDevice(), vertexBufferMemory);
      }

      const VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
      devManager.createIndexBuffer(indexBufferSize, indexBuffer, indexBufferMemory);
      {
        void* mappedData = nullptr;
        vkMapMemory(devManager.getDevice(), indexBufferMemory, 0, indexBufferSize, 0, &mappedData);
        memcpy(mappedData, indices.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(devManager.getDevice(), indexBufferMemory);
      }
    }

    ~VulkanModel() {
      if (indexBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device, indexBuffer, nullptr);

      if (indexBufferMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, indexBufferMemory, nullptr);

      if (vertexBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device, vertexBuffer, nullptr);

      if (vertexBufferMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }

   private:
    VkDevice device;
};

}
