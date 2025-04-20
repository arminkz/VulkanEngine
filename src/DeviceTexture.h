#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

// Texture on GPU
class DeviceTexture
{
public:
    DeviceTexture(std::shared_ptr<VulkanContext> ctx, const std::string& path);
    ~DeviceTexture();

    void cleanup();

    uint32_t getMipLevels() const { return _mipLevels; }
    VkImage getImage() const { return _textureImage; }
    VkImageView getImageView() const { return _textureImageView; }

    VkDescriptorImageInfo getDescriptorInfo(VkSampler sampler) const;

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
};