#pragma once
#include "stdafx.h"
#include "VulkanContext.h"


struct RenderPassParams
{
    VkFormat colorFormat;
    VkFormat depthFormat;

    bool useDepth = true;
    bool useColor = true;
    bool useResolve = false;

    VkSampleCountFlagBits msaaSamples;
};


class RenderPass
{
public:
    RenderPass(std::shared_ptr<VulkanContext> ctx, RenderPassParams params);
    ~RenderPass();

    VkRenderPass getRenderPass() const { return _renderPass; }

private:
    std::shared_ptr<VulkanContext> _ctx;

    VkRenderPass _renderPass = VK_NULL_HANDLE;
};