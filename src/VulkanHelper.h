#pragma once
#include "stdafx.h"
#include "VulkanContext.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

static std::vector<char> readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("Failed to open file: {}", filename);
        return {};
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

namespace VulkanHelper {

    VkCommandBuffer beginSingleTimeCommands(std::shared_ptr<VulkanContext> ctx);
    void endSingleTimeCommands(std::shared_ptr<VulkanContext> ctx, VkCommandBuffer commandBuffer);

    uint32_t findMemoryType(std::shared_ptr<VulkanContext> ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(std::shared_ptr<VulkanContext> ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(std::shared_ptr<VulkanContext> ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(std::shared_ptr<VulkanContext> ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    void createImage(std::shared_ptr<VulkanContext> ctx, uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(std::shared_ptr<VulkanContext> ctx, VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags);
    void transitionImageLayout(std::shared_ptr<VulkanContext> ctx, VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
    
}