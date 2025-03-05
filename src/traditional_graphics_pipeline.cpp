#include "vulkan_utils/traditional_graphics_pipeline.h"

#include "file_loader.h"
#include "graphics_types.h"

namespace VulkanUtils {
namespace {
constexpr const char* vertShaderPath = "build/vert.spv";
constexpr const char* fragShaderPath = "build/frag.spv";

}

VkShaderModule TraditionalGraphicsPipeline::createShaderModule(const std::vector<char>& code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
    throw std::runtime_error("failed to create shader module!");

  return shaderModule;
}

TraditionalGraphicsPipeline::TraditionalGraphicsPipeline(DeviceManager& devManager, const SwapChainHandler& swapchainHandler)
  : device(devManager.getDevice()),
    staticDescriptorSetLayout(createStaticDescriptorSetLayout()),
    dynamicDescriptorSetLayout(createDynamicDescriptorSetLayout())
  {
  const auto vertShaderCode = readFile(vertShaderPath);
  const auto fragShaderCode = readFile(fragShaderPath);

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  auto bindingDescription = GraphicsTypes::getVertexBindingDescription();
  auto attributeDescriptions = GraphicsTypes::getVertexAttributeDescriptions();
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  // Layout positions for the shader
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  auto& extent = swapchainHandler.getExtent();
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  std::vector<VkDynamicState> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f; // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f; // Optional
  multisampling.pSampleMask = nullptr; // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE; // Optional

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT |
      VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f; // Optional
  colorBlending.blendConstants[1] = 0.0f; // Optional
  colorBlending.blendConstants[2] = 0.0f; // Optional
  colorBlending.blendConstants[3] = 0.0f; // Optional

  // To allow pushing a unique model matrix for each
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PerModelPushConstants);

  std::array<VkDescriptorSetLayout, 2> layouts = {
      staticDescriptorSetLayout,
      dynamicDescriptorSetLayout
  };

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size()); 
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create pipeline layout!");

  createDescriptorPool();

  // learn more about passes
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = nullptr; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = swapchainHandler.getRenderPass();
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline!");

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);

  // Create a buffer abstaction class
  devManager.createUniformBuffer(sizeof(SceneUBO), sceneUniformBuffer, sceneUniformMemory);
  vkMapMemory(device, sceneUniformMemory, 0, sizeof(SceneUBO), 0, (void**)&scene);
  *scene = SceneUBO{};
  devManager.createUniformBuffer(sizeof(LightUBO), lightUniformBuffer, lightUniformMemory);
  vkMapMemory(device, lightUniformMemory, 0, sizeof(LightUBO), 0, (void**)&light);
  *light = LightUBO{};

  allocateDescriptorSets();
  updateStaticDescriptorSet();
}

TraditionalGraphicsPipeline::~TraditionalGraphicsPipeline() {
  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

  if (sceneUniformBuffer != VK_NULL_HANDLE)
    vkDestroyBuffer(device, sceneUniformBuffer, nullptr);

  if (sceneUniformMemory != VK_NULL_HANDLE) {
    vkUnmapMemory(device, sceneUniformMemory);
    vkFreeMemory(device, sceneUniformMemory, nullptr);
  }

  if (lightUniformBuffer != VK_NULL_HANDLE)
    vkDestroyBuffer(device, lightUniformBuffer, nullptr);

  if (lightUniformMemory != VK_NULL_HANDLE) {
    vkUnmapMemory(device, lightUniformMemory);
    vkFreeMemory(device, lightUniformMemory, nullptr);

  if (descriptorPool != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  }
}

void TraditionalGraphicsPipeline::createDescriptorPool() {
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = 2; // for scene + light
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = 1; // not fully sure how this works yet, or if this is the right number

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();

  poolInfo.maxSets = 2;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor pool!");
}

void TraditionalGraphicsPipeline::allocateDescriptorSets() {
  std::array<VkDescriptorSetLayout, 2> setLayouts = {
      staticDescriptorSetLayout, // global
      dynamicDescriptorSetLayout // texture
  };

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(setLayouts.size());
  allocInfo.pSetLayouts = setLayouts.data();

  std::array<VkDescriptorSet, 2> sets{};
  if (vkAllocateDescriptorSets(device, &allocInfo, sets.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate descriptor sets!");

  staticDescriptorSet = sets[0];
  dynamicDescriptorSet = sets[1];
}

void TraditionalGraphicsPipeline::updateTextureDescriptorSet(
    VkImageView textureImageView,
    VkSampler textureSampler) {
  std::vector<VkWriteDescriptorSet> descriptorWrites;

  // binding 0, image sampler
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = textureImageView;
  imageInfo.sampler = textureSampler;

  VkWriteDescriptorSet samplerWrite{};
  samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  samplerWrite.dstSet = dynamicDescriptorSet;
  samplerWrite.dstBinding = 0;
  samplerWrite.dstArrayElement = 0;
  samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerWrite.descriptorCount = 1;
  samplerWrite.pImageInfo = &imageInfo;
  descriptorWrites.push_back(samplerWrite);

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void TraditionalGraphicsPipeline::updateStaticDescriptorSet() {
  std::vector<VkWriteDescriptorSet> descriptorWrites;
  // binding 0, scene UBO
  VkDescriptorBufferInfo sceneBufferInfo{};
  sceneBufferInfo.buffer = sceneUniformBuffer;
  sceneBufferInfo.offset = 0;
  sceneBufferInfo.range = sizeof(SceneUBO);

  VkWriteDescriptorSet sceneWrite{};
  sceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  sceneWrite.dstSet = staticDescriptorSet;
  sceneWrite.dstBinding = 0;
  sceneWrite.dstArrayElement = 0;
  sceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sceneWrite.descriptorCount = 1;
  sceneWrite.pBufferInfo = &sceneBufferInfo;
  descriptorWrites.push_back(sceneWrite);

  // binding 1, light UBO
  VkDescriptorBufferInfo lightBufferInfo{};
  lightBufferInfo.buffer = lightUniformBuffer;
  lightBufferInfo.offset = 0;
  lightBufferInfo.range = sizeof(LightUBO);

  VkWriteDescriptorSet lightWrite{};
  lightWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  lightWrite.dstSet = staticDescriptorSet;
  lightWrite.dstBinding = 1;
  lightWrite.dstArrayElement = 0;
  lightWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightWrite.descriptorCount = 1;
  lightWrite.pBufferInfo = &lightBufferInfo;
  descriptorWrites.push_back(lightWrite);

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

VkDescriptorSetLayout TraditionalGraphicsPipeline::createStaticDescriptorSetLayout() {
  // binding 0, scene UBO
  VkDescriptorSetLayoutBinding sceneBinding{};
  sceneBinding.binding = 0;
  sceneBinding.descriptorCount = 1;
  sceneBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  sceneBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  sceneBinding.pImmutableSamplers = nullptr;

  // binding 1, light UBO
  VkDescriptorSetLayoutBinding lightBinding{};
  lightBinding.binding = 1;
  lightBinding.descriptorCount = 1;
  lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; //| VK_SHADER_STAGE_VERTEX_BIT; if used in vertex

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = { 
      sceneBinding, 
      lightBinding
  };

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  VkDescriptorSetLayout descriptorSetLayout;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create descriptor set layout!");

  return descriptorSetLayout;
}

VkDescriptorSetLayout TraditionalGraphicsPipeline::createDynamicDescriptorSetLayout() {
  // binding 0, image sampler
  VkDescriptorSetLayoutBinding textureBinding{};
  textureBinding.binding = 0;
  textureBinding.descriptorCount = 1;
  textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  // samplerBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &textureBinding;

  VkDescriptorSetLayout layout;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
    throw std::runtime_error("failed to create dynamic descriptor set layout!");

  return layout;
}

}