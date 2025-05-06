#pragma once
#include "stdafx.h"
#include "VulkanContext.h"


class Buffer
{
public:
    Buffer(std::shared_ptr<VulkanContext> ctx);
    ~Buffer();

    void initialize(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void destroy();

    VkBuffer getBuffer() const { return _buffer; }
    VkDeviceMemory getMemory() const { return _bufferMemory; }
    void* getMappedMemory() const { return _mappedMemory; }

    void copyData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    

private:
    std::shared_ptr<VulkanContext> _ctx;

    VkBuffer _buffer = VK_NULL_HANDLE;
    VkDeviceMemory _bufferMemory = VK_NULL_HANDLE;
    void* _mappedMemory = nullptr;
};