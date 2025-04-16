#pragma once
#include "stdafx.h"

class Texture
{
public:
    Texture(VulkanContext ctx, const std::string& path);
    ~Texture();

    void cleanup();

    VkImageView getImageView() const { return _textureImageView; }
    VkSampler getSampler() const { return _textureSampler; }

private:
    void generateMipmaps();

    uint32_t _width;
    uint32_t _height;
    VkFormat _format;
    uint32_t _mipLevels;
    
    VulkanContext _ctx;
    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkImageView _textureImageView;
    VkSampler _textureSampler;
};