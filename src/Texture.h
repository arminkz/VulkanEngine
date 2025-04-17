#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

class Texture
{
public:
    Texture(const VulkanContext& ctx, const std::string& path);
    ~Texture();

    void cleanup();

    uint32_t getMipLevels() const { return _mipLevels; }
    VkImage getImage() const { return _textureImage; }
    VkImageView getImageView() const { return _textureImageView; }

private:
    void generateMipmaps();

    uint32_t _width;
    uint32_t _height;
    VkFormat _format;
    uint32_t _mipLevels;
    
    const VulkanContext& _ctx;
    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkImageView _textureImageView;
};