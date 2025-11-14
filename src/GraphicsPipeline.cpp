#include <vks/GraphicsPipeline.hpp>

// Include the shader byte code
#include <base_frag.h>
#include <base_vert.h>
#include <sphere_frag.h>
#include <sphere_vert.h>

#include <map>
#include <string>
#include <array>
#include <iostream>
#include <stdexcept>
#include <glm/glm.hpp>

#include <vks/Device.hpp>
#include <vks/RenderPass.hpp>
#include <vks/SwapChain.hpp>
#include <vks/Descriptors.hpp>

#include "vks/Geometry.hpp"

using namespace vks;

GraphicsPipeline::GraphicsPipeline(const Device& device,
                                   const SwapChain& swapChain,
                                   const RenderPass& renderPass)
    : m_oldLayout(VK_NULL_HANDLE),
      m_device(device), m_swapChain(swapChain), m_renderPass(renderPass)
{
    // Call the main function to create ALL pipelines
    createPipelines();
    std::cout << "Successfully created the pipeline" << std::endl;
}

GraphicsPipeline::~GraphicsPipeline()
{
    // Clean up all pipelines and layouts in the maps
    for (auto& pair : m_pipelines)
    {
        vkDestroyPipeline(m_device.logical(), pair.second, nullptr);
    }
    for (auto& pair : m_pipelineLayouts)
    {
        vkDestroyPipelineLayout(m_device.logical(), pair.second, nullptr);
    }

    // Clean up descriptor set layouts (they are now shared_ptrs, so this is automatic)
    m_descriptorSetLayouts.clear();
}

VkPipeline GraphicsPipeline::getPipeline(const std::string& name) const
{
    // Use .at() for const-correctness and bounds checking
    try
    {
        return m_pipelines.at(name);
    }
    catch (const std::out_of_range& e)
    {
        throw std::runtime_error("Failed to find pipeline: " + name);
    }
}

VkPipelineLayout GraphicsPipeline::getLayout(const std::string& name) const
{
    try
    {
        return m_pipelineLayouts.at(name);
    }
    catch (const std::out_of_range& e)
    {
        throw std::runtime_error("Failed to find pipeline layout: " + name);
    }
}

Ref<DescriptorSetLayout> GraphicsPipeline::getDescriptorSetLayout(const std::string& name) const
{
    try
    {
        return m_descriptorSetLayouts.at(name);
    }
    catch (const std::out_of_range& e)
    {
        throw std::runtime_error("Failed to find descriptor set layout: " + name);
    }
}

void GraphicsPipeline::recreate()
{
    // Clean up all pipelines and layouts in the maps
    for (auto& pair : m_pipelines)
    {
        vkDestroyPipeline(m_device.logical(), pair.second, nullptr);
    }
    m_pipelines.clear();

    for (auto& pair : m_pipelineLayouts)
    {
        vkDestroyPipelineLayout(m_device.logical(), pair.second, nullptr);
    }
    m_pipelineLayouts.clear();

    // Clean up descriptor set layouts (just clear the map)
    m_descriptorSetLayouts.clear();

    // Re-create all
    createPipelines();
}

void GraphicsPipeline::createPipelines()
{
    // --- Create Shared Descriptor Set Layouts ---
    // Use your new vks::DescriptorSetLayout::Builder

    // "global" layout (Set 0) for camera UBO
    // Matches: layout(set = 0, binding = 0) uniform CameraUBO
    m_descriptorSetLayouts["global"] = vks::DescriptorSetLayout::Builder(m_device)
                                       .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                                       .build();

    // "material" layout (Set 1) for material UBO
    // Matches: layout(set = 1, binding = 0) uniform MaterialUBO
    m_descriptorSetLayouts["material"] = vks::DescriptorSetLayout::Builder(m_device)
                                         .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
                                         // .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // For textures later
                                         .build();

    // --- Create Individual Pipelines ---
    createBasePipeline();
    createSpherePipeline();
}

void GraphicsPipeline::createBasePipeline()
{
    VkShaderModule vertShaderModule = createShaderModule(BASE_VERT);
    VkShaderModule fragShaderModule = createShaderModule(BASE_FRAG);

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

    // --- No Vertex Input ---
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    // --- Standard Config (Input Assembly, Viewport, Rasterizer, etc.) ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChain.extent().width);
    viewport.height = static_cast<float>(m_swapChain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChain.extent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Original
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // --- Empty Pipeline Layout ---
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(m_device.logical(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Base Pipeline Layout creation failed");
    }

    // --- No Depth Test ---
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE; // No depth test
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // --- Create Pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil; // No depth
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = m_renderPass.handle();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(m_device.logical(), VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Base Graphics Pipeline creation failed");
    }

    // --- Store in map ---
    m_pipelines["base"] = pipeline;
    m_pipelineLayouts["base"] = pipelineLayout;

    for (auto& shader : shaderStages)
    {
        vkDestroyShaderModule(m_device.logical(), shader.module, nullptr);
    }
}

void GraphicsPipeline::createSpherePipeline()
{
    VkShaderModule vertShaderModule = createShaderModule(SPHERE_VERT);
    VkShaderModule fragShaderModule = createShaderModule(SPHERE_FRAG);

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

    // --- Vertex Input (from vks::geometry::Vertex) ---
    auto bindingDescription = vks::geometry::Vertex::getBindingDescription();
    auto attributeDescriptions = vks::geometry::Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // --- Standard Config (Input Assembly, Viewport, Rasterizer, etc.) ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChain.extent().width);
    viewport.height = static_cast<float>(m_swapChain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChain.extent();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // For 3D models
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // --- Pipeline Layout (with UBOs and Push Constants) ---
    // Push Constant for the model matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4); // Assumes you use GLM for matrices

    // This pipeline uses TWO descriptor sets
    // (Set 0 = "global", Set 1 = "material")
    std::array<VkDescriptorSetLayout, 2> setLayouts = {
        m_descriptorSetLayouts["global"]->getDescriptorSetLayout(),
        m_descriptorSetLayouts["material"]->getDescriptorSetLayout()
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data(); // Use both layouts
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(m_device.logical(), &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Sphere Pipeline Layout creation failed");
    }

    // --- Enable Depth Test ---
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE; // Enable depth testing
    depthStencil.depthWriteEnable = VK_TRUE; // Write to the depth buffer
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // Fragments in front pass
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // --- Create Pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil; // Set depth state
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = m_renderPass.handle();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(m_device.logical(), VK_NULL_HANDLE, 1,
                                  &pipelineInfo, nullptr,
                                  &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Sphere Graphics Pipeline creation failed");
    }

    // --- Store in map ---
    m_pipelines["sphere"] = pipeline;
    m_pipelineLayouts["sphere"] = pipelineLayout;

    for (auto& shader : shaderStages)
    {
        vkDestroyShaderModule(m_device.logical(), shader.module, nullptr);
    }
}

VkShaderModule
GraphicsPipeline::createShaderModule(const std::vector<unsigned char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();

    // Vector class already ensures that the data is correctly aligned,
    // so no need to manually do it
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(m_device.logical(), &createInfo, nullptr, &module) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return module;
}
