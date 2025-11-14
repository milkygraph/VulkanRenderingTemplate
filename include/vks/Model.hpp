#pragma once

#include <vks/Device.hpp>
#include <vks/Buffer.hpp>
#include <vks/Geometry.hpp>
#include <vulkan/vulkan.h>
#include <memory>


namespace vks
{
    /**
     * @brief Manages a 3D model's geometry on the GPU.
     * This class owns the VkBuffer for vertices and indices.
     * It's designed to be stored in a registry (e.g., std::map<string, Model>)
     */
    class Model
    {
    public:
        /**
         * @brief Default constructor for creating an empty model.
         */
        Model() = default;
        ~Model() = default;

        void Model::createSphere(
            const vks::Device& device,
            VkCommandPool commandPool,
            float radius,
            uint32_t sectors,
            uint32_t stacks);

        // Models are unique assets, so delete copy operations.
        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        // Models can be moved (e.g., when placing in a std::map)
        Model(Model&&) = default;
        Model& operator=(Model&&) = default;

        // --- Getters for the Render Loop ---
        VkBuffer getVertexBuffer() const { return m_vertexBuffer->getBuffer(); }
        VkBuffer getIndexBuffer() const { return m_indexBuffer->getBuffer(); }
        uint32_t getIndexCount() const { return m_indexCount; }

    private:
        /**
         * @brief Helper to create and fill a vertex/index buffer using a staging buffer.
         * This is the correct way to upload data to fast DEVICE_LOCAL memory.
         */
        void createBufferFromData(
            const vks::Device& device,
            VkCommandPool commandPool,
            void* data,
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            std::unique_ptr<vks::Buffer>& outBuffer);

        std::unique_ptr<vks::Buffer> m_vertexBuffer;
        std::unique_ptr<vks::Buffer> m_indexBuffer;

        uint32_t m_vertexCount = 0;
        uint32_t m_indexCount = 0;
    };
} // namespace vks