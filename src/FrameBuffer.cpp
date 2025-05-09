#include "FrameBuffer.h"


FrameBuffer::FrameBuffer(std::shared_ptr<VulkanContext> ctx, const FrameBufferParams& params)
    : _ctx(std::move(ctx))
{
    if (params.hasColor) {
        if (params.colorImageView != VK_NULL_HANDLE) {
            _colorImageView = params.colorImageView;
        } else {
            createColorResources(params.extent, params.colorFormat, params.msaaSamples, params.colorUsage);
        }
    }
    
    if (params.hasDepth) {
        if (params.depthImageView != VK_NULL_HANDLE) {
            _depthImageView = params.depthImageView;
        } else {
            createDepthResources(params.extent, params.depthFormat, params.msaaSamples, params.depthUsage);
        }
    }

    if (params.hasResolve) {
        _resolveImageView = params.resolveImageView;
    }

    std::vector<VkImageView> attachments;
    if (params.hasColor) {
        attachments.push_back(_colorImageView);
    }
    if (params.hasDepth) {
        attachments.push_back(_depthImageView);
    }
    if (params.hasResolve) {
        attachments.push_back(_resolveImageView);
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = params.renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = params.extent.width;
    framebufferInfo.height = params.extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(_ctx->device, &framebufferInfo, nullptr, &_frameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer!");
    }
}

FrameBuffer::~FrameBuffer() {
    vkDestroyFramebuffer(_ctx->device, _frameBuffer, nullptr);

    vkDestroyImageView(_ctx->device, _colorImageView, nullptr);
    vkDestroyImage(_ctx->device, _colorImage, nullptr);
    vkFreeMemory(_ctx->device, _colorImageMemory, nullptr);

    vkDestroyImageView(_ctx->device, _depthImageView, nullptr);
    vkDestroyImage(_ctx->device, _depthImage, nullptr);
    vkFreeMemory(_ctx->device, _depthImageMemory, nullptr);
}

void FrameBuffer::createColorResources(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits msaaSamples, VkImageUsageFlags usage) {

    VulkanHelper::createImage(_ctx, extent.width, extent.height, format, 1, msaaSamples,
        VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _colorImage, _colorImageMemory);

    _colorImageView = VulkanHelper::createImageView(_ctx, _colorImage, format, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}

void FrameBuffer::createDepthResources(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits msaaSamples, VkImageUsageFlags usage) {

    VulkanHelper::createImage(_ctx, extent.width, extent.height, format, 1, msaaSamples,
        VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage, _depthImageMemory);

    _depthImageView = VulkanHelper::createImageView(_ctx, _depthImage, format, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
}