#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

// Texture on GPU
class DeviceTexture
{
public:
    DeviceTexture(std::shared_ptr<VulkanContext> ctx, const std::string& path, VkFormat format);
    DeviceTexture(std::shared_ptr<VulkanContext> ctx, const void* pixelData, uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels = 0);
    ~DeviceTexture();

    void cleanup();

    uint32_t getMipLevels() const { return _mipLevels; }
    VkImage getImage() const { return _textureImage; }
    VkImageView getImageView() const { return _textureImageView; }

    VkDescriptorImageInfo getDescriptorInfo(VkSampler sampler) const;

    // Singleton pattern for dummy texture
    static DeviceTexture* getDummy(std::shared_ptr<VulkanContext> ctx);
    static void cleanupDummy();

private:
    std::shared_ptr<VulkanContext> _ctx;

    uint32_t _width;
    uint32_t _height;
    VkFormat _format;
    uint32_t _mipLevels;
    
    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkImageView _textureImageView;
    
    void generateMipmaps();

    static DeviceTexture* dummyTexture;
};