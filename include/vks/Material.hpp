#pragma once

#include <vks/Device.hpp>
#include <vks/GraphicsPipeline.hpp> // Your manager class
#include <vks/Descriptors.hpp>      // Your descriptor system
#include "Buffer.hpp"               // Your buffer helper class
#include <vulkan/vulkan.h>
#include <string>
#include <stdexcept>
#include <memory>
#include <glm/glm.hpp>

namespace vks {

// UBO struct for material data
// This MUST match the layout in the shader (Set 1, Binding 0)
struct MaterialUBO {
    alignas(16) glm::vec4 color;
};

/**
 * @brief Represents a "Material Instance."
 * This class links a Pipeline's *name* with its unique data (Descriptor Set).
 * It creates and owns its own VkDescriptorSet and the UBO that holds its data.
 */
class Material {
public:
    /**
     * @brief Creates a new Material instance.
     * @param device The logical device.
     * @param pipelineManager The pipeline manager (to get layout info).
     * @param descriptorPool The global pool to allocate this material's set from.
     * @param pipelineName The name of the pipeline this material uses (e.g., "sphere").
     * @param color The unique color for this material.
     */
    Material(
        const vks::Device& device,
        const vks::GraphicsPipeline& pipelineManager,
        Ref<vks::DescriptorPool> descriptorPool,
        const std::string& pipelineName,
        glm::vec4 color
    ) :
        m_pipelineName(pipelineName),
        m_materialDescriptorSet(VK_NULL_HANDLE)
    {
        // Get the Material Descriptor Set Layout (for Set 1)
        // We get this from the manager to ensure we match what the pipeline expects.
        Ref<vks::DescriptorSetLayout> materialLayout =
            pipelineManager.getDescriptorSetLayout("material");

        // Create a unique UBO for this material instance's color
        m_uboBuffer = std::make_unique<vks::Buffer>(
            device,
            sizeof(MaterialUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Map and write the color data
        m_uboBuffer->map();
        uboData = MaterialUBO{color};
        m_uboBuffer->writeToBuffer(&uboData);
        m_uboBuffer->unmap();

        // Use DescriptorWriter to build this material's Descriptor Set
        auto bufferInfo = m_uboBuffer->descriptorInfo();
        DescriptorWriter writer(materialLayout, descriptorPool);
        writer.writeBuffer(0, &bufferInfo); // Binds UBO to binding 0

        if (!writer.build(m_materialDescriptorSet)) {
            throw std::runtime_error("Failed to build material descriptor set!");
        }
    }

    /**
     * @brief Destructor
     * Cleans up resources owned by this material.
     */
    ~Material() = default; // m_uboBuffer is a unique_ptr, so it cleans itself up.

    // Materials are unique: delete copy operations
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    // Materials can be moved (e.g., when placing them into a std::map)
    Material(Material&& other) = default;
    Material& operator=(Material&& other) = default;

    /**
     * @brief Gets the name of the pipeline this material uses.
     * The render loop uses this name to fetch the *current, valid*
     * pipeline handle from the GraphicsPipeline manager.
     */
    const std::string& getPipelineName() const { return m_pipelineName; }

    /**
     * @brief Gets this material's unique VkDescriptorSet (Set 1).
     * This set contains the material's color, textures, etc.
     */
    VkDescriptorSet getDescriptorSet() const { return m_materialDescriptorSet; }

    MaterialUBO uboData;

private:
    // --- Stored Data ---

    // The name of the pipeline (e.g., "sphere").
    std::string m_pipelineName;

    // This material's unique descriptor set (Set 1).
    VkDescriptorSet m_materialDescriptorSet;

    // This material OWNS its Uniform Buffer.
    // std::unique_ptr handles automatic cleanup.
    std::unique_ptr<vks::Buffer> m_uboBuffer;
};

} // namespace vks
