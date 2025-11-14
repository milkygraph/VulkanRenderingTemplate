#pragma once

#include <vks/Device.hpp> // Your existing Device class
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cassert>
#include <cstring> // For memcpy

namespace vks {

class Buffer {
public:
    /**
     * @brief Creates a new buffer and allocates its memory.
     * @param device The vks::Device object.
     * @param size The total size of the buffer in bytes.
     * @param usageFlags The VkBufferUsageFlags (e.g., VERTEX_BUFFER, UNIFORM_BUFFER).
     * @param memoryPropertyFlags The memory properties (e.g., HOST_VISIBLE, DEVICE_LOCAL).
     */
    Buffer(
        const vks::Device& device,
        VkDeviceSize size,
        VkBufferUsageFlags usageFlags,
        VkMemoryPropertyFlags memoryPropertyFlags);

    ~Buffer();

    // Buffers are unique resources, so delete copy operations.
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /**
     * @brief Maps the buffer's memory to a CPU-accessible pointer.
     * @param size The size of the memory to map (default: whole buffer).
     * @param offset The offset from the start of the buffer memory.
     * @return VK_SUCCESS on success.
     */
    VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    /**
     * @brief Unmaps the buffer's memory.
     */
    void unmap();

    /**
     * @brief Writes data to the mapped buffer.
     * @warning The buffer must be mapped with map() before calling this.
     * @param data A pointer to the data to write.
     * @param size The size of the data to write (default: whole buffer).
     * @param offset The offset in the mapped memory to write to.
     */
    void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    /**
     * @brief Creates a VkDescriptorBufferInfo struct for this buffer.
     * @param size The size of the buffer region to include in the descriptor.
     * @param offset The offset from the start of the buffer.
     * @return A VkDescriptorBufferInfo struct.
     */
    VkDescriptorBufferInfo descriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

    // --- Getters ---
    VkBuffer getBuffer() const { return m_buffer; }
    VkDeviceMemory getMemory() const { return m_memory; }
    VkDeviceSize getSize() const { return m_bufferSize; }

private:
    /**
     * @brief Finds a suitable memory type index on the physical device.
     */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Store a reference to the device, not a copy
    const vks::Device& m_device;

    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

    VkDeviceSize m_bufferSize;
    void* m_mapped = nullptr; // Pointer to the mapped memory
};

} // namespace vks
