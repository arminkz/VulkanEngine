#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

class TextureSampler
{
public:
    TextureSampler(const VulkanContext& ctx, uint32_t mipLevels);
    ~TextureSampler();

    VkSampler getSampler() const { return _textureSampler; }

private:
    const VulkanContext& _ctx;

    VkSampler _textureSampler;
};