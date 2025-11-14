#include <vks/Model.hpp>

#include "vks/Application.hpp"
#include "vks/CommandBuffers.hpp"
#include "vks/Basic/BasicCommandBuffers.hpp"
using namespace vks;

void Model::createSphere(
    const vks::Device& device,
    VkCommandPool commandPool,
    float radius,
    uint32_t sectors,
    uint32_t stacks)
{
    // 1. Generate the data on the CPU
    std::vector<vks::geometry::Vertex> vertices;
    std::vector<uint32_t> indices;
    vks::geometry::createSphere(vertices, indices, radius, sectors, stacks);

    m_vertexCount = static_cast<uint32_t>(vertices.size());
    m_indexCount = static_cast<uint32_t>(indices.size());

    // 2. Upload vertex data to the GPU
    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * m_vertexCount;
    createBufferFromData(
        device,
        commandPool,
        vertices.data(),
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        m_vertexBuffer);

    // 3. Upload index data to the GPU
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * m_indexCount;
    createBufferFromData(
        device,
        commandPool,
        indices.data(),
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        m_indexBuffer);
}

void Model::createBufferFromData(
    const vks::Device& device,
    VkCommandPool commandPool,
    void* data,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    std::unique_ptr<vks::Buffer>& outBuffer)
{
    // 1. Create a "staging" buffer on the CPU
    // This is a temporary buffer that's host-visible (mappable)
    vks::Buffer stagingBuffer{
        device,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // It's a "source" for a transfer
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    // 2. Map and copy data to the staging buffer
    stagingBuffer.map();
    stagingBuffer.writeToBuffer(data);
    stagingBuffer.unmap();

    // 3. Create the final "device" buffer
    // This buffer is DEVICE_LOCAL (fast GPU memory) but not host-visible
    outBuffer = std::make_unique<vks::Buffer>(
        device,
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, // It's a "destination" AND its final usage
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    CommandBuffers::SingleTimeCommands(device, Application::getInstance().getCommandPool(), [&](const VkCommandBuffer& commandBuffer)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.getBuffer(), outBuffer->getBuffer(), 1, &copyRegion);
    });
}