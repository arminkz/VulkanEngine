#pragma once
#include "stdafx.h"

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

struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkCommandPool commandPool;
    //VkSurfaceKHR surface;
    //VkSwapchainKHR swapChain;
    //std::vector<VkImage> swapChainImages;
    //VkFormat swapChainImageFormat;
    //VkExtent2D swapChainExtent;
    //std::vector<VkImageView> swapChainImageViews;
};


namespace VulkanHelper {

    VkCommandBuffer beginSingleTimeCommands(VulkanContext ctx);
    void endSingleTimeCommands(VulkanContext ctx, VkCommandBuffer commandBuffer);

    uint32_t findMemoryType(VulkanContext ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(VulkanContext ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VulkanContext ctx, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VulkanContext ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    void createImage(VulkanContext ctx, uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VulkanContext ctx, VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags);
    void transitionImageLayout(VulkanContext ctx, VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
    
}