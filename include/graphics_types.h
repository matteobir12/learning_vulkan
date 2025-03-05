#pragma once

#include <glm/glm.hpp>
#include "vulkan/vulkan.h"

#include <array>

namespace GraphicsTypes {
struct Vertex {
  glm::vec3 position;
  glm::vec3 normal{};
  glm::vec2 texCoord{};
};

inline VkVertexInputBindingDescription getVertexBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

// May need to change if ray traced version needs to change
inline std::array<VkVertexInputAttributeDescription, 3> getVertexAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

  // layout(location = 0) -> position
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, Vertex::position);

  // layout(location = 1) -> normal
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, Vertex::normal);

  // layout(location = 2) -> texCoord
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, Vertex::texCoord);

  return attributeDescriptions;
}
}
