#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

class VulkanContext;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

namespace VulkanHelper {

    VkCommandBuffer beginSingleTimeCommands(const std::shared_ptr<VulkanContext>& ctx);
    void endSingleTimeCommands(const std::shared_ptr<VulkanContext>& ctx, VkCommandBuffer commandBuffer);

    uint32_t findMemoryType(const std::shared_ptr<VulkanContext>& ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(const std::shared_ptr<VulkanContext>& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(const std::shared_ptr<VulkanContext>& ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(const std::shared_ptr<VulkanContext>& ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void copyBufferToCubemap(const std::shared_ptr<VulkanContext>& ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void copyImageToBuffer(const std::shared_ptr<VulkanContext>& ctx, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height);

    void createImage(const std::shared_ptr<VulkanContext>& ctx, uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags = 0);
    VkImageView createImageView(const std::shared_ptr<VulkanContext>& ctx, VkImage image, VkFormat format, uint32_t mipLevels, uint32_t arrayLayers, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    void transitionImageLayout(const std::shared_ptr<VulkanContext>& ctx, VkImage image, VkFormat format, uint32_t mipLevels, uint32_t arrayLayers, VkImageLayout oldLayout, VkImageLayout newLayout);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    VkFormat findSupportedFormat(const std::shared_ptr<VulkanContext>& ctx, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat(const std::shared_ptr<VulkanContext>& ctx);
    VkSampleCountFlagBits getMaxMsaaSampleCount(const std::shared_ptr<VulkanContext>& ctx);
    bool hasStencilComponent(VkFormat format);

    std::string formatToString(VkFormat format);

}