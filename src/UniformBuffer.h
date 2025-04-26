#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

template<typename T>
class UniformBuffer
{
public:

    UniformBuffer(std::shared_ptr<VulkanContext> ctx, uint32_t binding, VkShaderStageFlags stageFlags);
    ~UniformBuffer();

    void update(const T& data);

    VkBuffer getBuffer() const { return _uniformBuffer; }
    uint32_t getBinding() const { return _binding; }
    VkShaderStageFlags getStageFlags() const { return _stageFlags; }
    VkDescriptorBufferInfo getDescriptorInfo() const;

private:

    std::shared_ptr<VulkanContext> _ctx;

    uint32_t _binding;
    VkShaderStageFlags _stageFlags;

    VkBuffer _uniformBuffer;
    VkDeviceMemory _uniformBufferMemory;
    void* _uniformBufferMapped;
};

template<typename T>
UniformBuffer<T>::UniformBuffer(std::shared_ptr<VulkanContext> ctx, uint32_t binding, VkShaderStageFlags stageFlags)
    : _ctx(std::move(ctx)), _binding(binding), _stageFlags(stageFlags)
{
    // Create uniform buffer
    VkDeviceSize bufferSize = sizeof(T); // Size of the uniform buffer

    VulkanHelper::createBuffer(_ctx,
        bufferSize, 
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        _uniformBuffer, _uniformBufferMemory);

    // Map the buffer memory into CPU addressable space
    vkMapMemory(_ctx->device, _uniformBufferMemory, 0, bufferSize, 0, &_uniformBufferMapped);
}

template<typename T>
UniformBuffer<T>::~UniformBuffer()
{
    vkUnmapMemory(_ctx->device, _uniformBufferMemory);
    vkDestroyBuffer(_ctx->device, _uniformBuffer, nullptr);
    vkFreeMemory(_ctx->device, _uniformBufferMemory, nullptr);
}

template <typename T>
void UniformBuffer<T>::update(const T& data) {
    memcpy(_uniformBufferMapped, &data, sizeof(T));
}

template <typename T>
VkDescriptorBufferInfo UniformBuffer<T>::getDescriptorInfo() const {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(T);
    return bufferInfo;
}