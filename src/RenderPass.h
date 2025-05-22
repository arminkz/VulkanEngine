#pragma once
#include "stdafx.h"
#include "VulkanContext.h"


struct RenderPassParams
{
    VkFormat colorFormat;
    VkFormat depthFormat;
    VkFormat resolveFormat;

    VkImageLayout colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkImageLayout depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkImageLayout resolveLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    bool useDepth = true;
    bool useColor = true;
    bool useResolve = true;

    bool isMultiPass = true;

    VkSampleCountFlagBits msaaSamples;

    std::string name = "RenderPass";
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