#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

class TextureSampler
{
public:
    TextureSampler(std::shared_ptr<VulkanContext> ctx, uint32_t mipLevels);
    ~TextureSampler();

    VkSampler getSampler() const { return _textureSampler; }

private:
    std::shared_ptr<VulkanContext> _ctx;

    VkSampler _textureSampler;
};