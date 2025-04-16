#pragma once
#include "stdafx.h"
#include "VukanContext.h"

class TextureSampler
{
public:
    TextureSampler(const VulkanContext& ctx);
    ~TextureSampler();

    VkSampler getSampler() const { return _textureSampler; }

private:
    VkSampler _textureSampler;
}