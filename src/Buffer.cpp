#include <vks/Buffer.hpp>

namespace vks {

Buffer::Buffer(
    const vks::Device& device,
    VkDeviceSize size,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags)
    : m_device(device), m_bufferSize(size)
{
    // 1. Create the VkBuffer handle
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_bufferSize;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Or CONCURRENT if needed

    if (vkCreateBuffer(m_device.logical(), &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VkBuffer!");
    }

    // 2. Get the memory requirements for this buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device.logical(), m_buffer, &memRequirements);

    // 3. Allocate the VkDeviceMemory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags);

    if (vkAllocateMemory(m_device.logical(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    // 4. Bind the memory to the buffer
    if (vkBindBufferMemory(m_device.logical(), m_buffer, m_memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind buffer memory!");
    }
}

Buffer::~Buffer() {
    // Unmap if it's still mapped
    if (m_mapped) {
        unmap();
    }
    // These calls are safe even if the handles are VK_NULL_HANDLE
    vkDestroyBuffer(m_device.logical(), m_buffer, nullptr);
    vkFreeMemory(m_device.logical(), m_memory, nullptr);
}

uint32_t Buffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device.physical(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Check if the memory type's bit is set in the typeFilter
        if ((typeFilter & (1 << i)) &&
            // Check if all required property flags are set for this memory type
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
    assert(!m_mapped && "Buffer is already mapped!");
    return vkMapMemory(m_device.logical(), m_memory, offset, size, 0, &m_mapped);
}

void Buffer::unmap() {
    assert(m_mapped && "Buffer is not mapped!");
    vkUnmapMemory(m_device.logical(), m_memory);
    m_mapped = nullptr;
}

void Buffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
    assert(m_mapped && "Cannot write to an unmapped buffer! Call map() first.");

    if (size == VK_WHOLE_SIZE) {
        size = m_bufferSize; // If VK_WHOLE_SIZE, use the full size of the buffer
    }

    char* mem_offset = (char*)m_mapped;
    mem_offset += offset;
    memcpy(mem_offset, data, (size_t)size);
}

VkDescriptorBufferInfo Buffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
    return VkDescriptorBufferInfo{
        m_buffer,
        offset,
        size // If VK_WHOLE_SIZE, it will be handled by Vulkan
    };
}

} // namespace vks
