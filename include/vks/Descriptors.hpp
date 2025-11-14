#pragma once

#include <vks/Device.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vks {

template<typename T>
using Ref = std::shared_ptr<T>;

class DescriptorSetLayout;
class DescriptorPool;
class DescriptorWriter;

class DescriptorSetLayout {
public:
    class Builder {
    public:
        // Pass in the device instead of using a singleton
        Builder(const vks::Device& device) : m_device(device) {}

        Builder& addBinding(
            uint32_t binding,
            VkDescriptorType descriptorType,
            VkShaderStageFlags stageFlags,
            uint32_t count = 1);
        Ref<DescriptorSetLayout> build() const;

    private:
        const vks::Device& m_device; // Store a reference
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
    };

    DescriptorSetLayout(
        const vks::Device& device, // Pass in device
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~DescriptorSetLayout();
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    const vks::Device& m_device; // Store a reference
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

    friend class DescriptorWriter;
};

class DescriptorPool {
public:
    const VkDescriptorPool getDescriptorPool() { return m_descriptorPool; }

    class Builder {
    public:
        Builder(const vks::Device& device) : m_device(device) {}

        Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder& setMaxSets(uint32_t count);
        Ref<DescriptorPool> build() const;

    private:
        const vks::Device& m_device;
        std::vector<VkDescriptorPoolSize> m_poolSizes{};
        uint32_t m_maxSets = 1000;
        VkDescriptorPoolCreateFlags m_poolFlags = 0;
    };

    DescriptorPool(
        const vks::Device& device,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;

    bool allocateDescriptor(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

    void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

    void resetPool();

    const vks::Device& getDevice() const { return m_device; }

private:
    const vks::Device& m_device;
    VkDescriptorPool m_descriptorPool;

    friend class DescriptorWriter;
};

class DescriptorWriter {
public:
    DescriptorWriter(Ref<DescriptorSetLayout> setLayout, Ref<DescriptorPool> pool);

    DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
    DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

    bool build(VkDescriptorSet& set);
    void overwrite(VkDescriptorSet& set);

private:
    Ref<DescriptorSetLayout> m_setLayout;
    Ref<DescriptorPool> m_pool; // Changed from reference
    std::vector<VkWriteDescriptorSet> m_writes;
};

} // namespace vks
