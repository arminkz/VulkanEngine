#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "VulkanContext.h"


struct FrameBufferParams
{
    VkExtent2D extent;
    VkRenderPass renderPass;

    bool hasColor = true;
    bool hasDepth = true;
    bool hasResolve = false;

    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkImageUsageFlags colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
    VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    VkImageView colorImageView = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkImageView resolveImageView = VK_NULL_HANDLE;
};


class FrameBuffer
{
public:
    FrameBuffer(std::shared_ptr<VulkanContext> ctx, const FrameBufferParams& params);
    ~FrameBuffer();

    VkFramebuffer getFrameBuffer() const { return _frameBuffer; }
    
    VkImage getColorImage() const { return _colorImage; }
    VkImageView getColorImageView() const { return _colorImageView; }

    VkImage getDepthImage() const { return _depthImage; }
    VkImageView getDepthImageView() const { return _depthImageView; }

private:
    std::shared_ptr<VulkanContext> _ctx;

    VkImage _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView _colorImageView;
    void createColorResources(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits msaaSamples, VkImageUsageFlags usage);

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;
    void createDepthResources(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits msaaSamples, VkImageUsageFlags usage);

    VkImageView _resolveImageView;

    VkFramebuffer _frameBuffer;
};