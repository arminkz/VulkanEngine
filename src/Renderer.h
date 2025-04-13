#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "GeometryHelper.h"
#include "geometry/Vertex.h"
#include "geometry/Mesh.h"
#include "TurnTableCamera.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {

private:
    SDL_Window* _window = nullptr;

    Mesh _mesh;
    std::unique_ptr<TurnTableCamera> _camera = nullptr;

    VkInstance _instance = nullptr;
    VkPhysicalDevice _physicalDevice = nullptr;
    VkDevice _device = nullptr;
    VkSurfaceKHR _surface = nullptr;

    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

    VkSwapchainKHR _swapChain = nullptr;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass;

    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;
    
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;

    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;
    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;
    std::vector<void*> _uniformBuffersMapped;

    uint32_t _mipLevels;
    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;
    VkImageView _textureImageView;
    VkSampler _textureSampler;

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView _colorImageView;

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;

    bool _validationLayersAvailable = true;

    // Debug messenger for validation layers
    void setupDebugMessenger();
    VkDebugUtilsMessengerEXT debugMessenger;
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    

    bool createVulkanInstance();
    bool createSurface();

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool pickPhysicalDevice();

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool createLogicalDevice();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    bool createSwapChain();
    bool recreateSwapChain();
    void cleanupSwapChain();

    VkShaderModule createShaderModule(const std::vector<char>& code);
    bool createRenderPass();

    bool createDescriptorSetLayout();
    bool createDescriptorPool();
    bool createDescriptorSets();

    bool createGraphicsPipeline();

    bool createFramebuffers();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    bool copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    bool createImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipLevels, VkImageAspectFlags aspectFlags);
    void transitionImageLayout(VkImage image, VkFormat format, uint32_t mipLevels, VkImageLayout oldLayout, VkImageLayout newLayout);
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkSampleCountFlagBits getMaxMsaaSampleCount();
    bool hasStencilComponent(VkFormat format);

    bool createVertexBuffer();
    bool createIndexBuffer();
    bool createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    bool createTextureImage();
    bool createTextureImageView();
    bool createTextureSampler();
    void generateMipmaps(VkImage image, VkFormat imageFormat, uint32_t mipLevels, int32_t texWidth, int32_t texHeight);

    void createColorResources();
    void createDepthResources();
    
    bool createCommandPool();
    bool createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    bool createSyncObjects();

    void printVulkanInfo();
    void printSwapChainSupportDetails(const SwapChainSupportDetails& details);

public:
    Renderer();
    ~Renderer();

    bool initialize(SDL_Window* window);
    void drawFrame();
    void informFramebufferResized() { _framebufferResized = true; };

    TurnTableCamera* getCamera() { return _camera.get(); };
};