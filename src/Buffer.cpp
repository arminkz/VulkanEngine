#include "Buffer.h"
#include "VulkanHelper.h"

Buffer::Buffer(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx)), _mappedMemory(nullptr)
{
}

Buffer::~Buffer()
{
    spdlog::debug("Buffer destructor called");
    destroy();
}

void Buffer::initialize(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    // Create the buffer and allocate memory for it
    VulkanHelper::createBuffer(_ctx, size, usage, properties, _buffer, _bufferMemory);

    // Map the memory to the buffer
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        vkMapMemory(_ctx->device, _bufferMemory, 0, size, 0, &_mappedMemory);
    }
}

void Buffer::destroy()
{
    // Unmap the memory if it was mapped
    if (_mappedMemory) {
        vkUnmapMemory(_ctx->device, _bufferMemory);
        _mappedMemory = nullptr;
    }

    // Destroy the buffer and free the memory
    if (_buffer) {
        vkDestroyBuffer(_ctx->device, _buffer, nullptr);
        vkFreeMemory(_ctx->device, _bufferMemory, nullptr);
        _buffer = VK_NULL_HANDLE;
        _bufferMemory = VK_NULL_HANDLE;
    }
}

void Buffer::copyData(const void* data, VkDeviceSize size, VkDeviceSize offset)
{
    if (_mappedMemory) {
        memcpy(static_cast<int32_t*>(_mappedMemory) + offset, data, size);
    } else {
        throw std::runtime_error("Buffer is not mapped!");
    }
}