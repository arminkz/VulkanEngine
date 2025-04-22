#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "GeometryHelper.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "TurnTableCamera.h"
#include "DeviceModel.h"
#include "TextureSampler.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {

private:
    SDL_Window* _window = nullptr;

    std::shared_ptr<VulkanContext> _ctx;

    std::unique_ptr<TurnTableCamera> _camera = nullptr;
    std::unique_ptr<DeviceTexture> _dummyTexture;

    std::unique_ptr<TextureSampler> _textureSampler;

    struct SceneInfo {
        alignas(4) float time;
    };
    std::array<std::unique_ptr<UniformBuffer<SceneInfo>>, MAX_FRAMES_IN_FLIGHT> _sceneInfoUBOs;
    
    std::vector<std::unique_ptr<DeviceModel>> _models;

    
    VkSwapchainKHR _swapChain = nullptr;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass;

    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorPool _descriptorPool;

    //std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<std::vector<VkDescriptorSet>> _descriptorSets;  //_descriptorSets[ModelIndex][FrameIndex]
    
    VkPipelineLayout _pipelineLayout;
    VkPipeline _graphicsPipeline;

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;
    std::vector<void*> _uniformBuffersMapped;

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView _colorImageView;

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    //VkCommandPool _commandPool;
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
    

    void createVulkanContext();
    //-------------------------
    bool createVulkanInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createCommandPool();
    //-------------------------


    bool isDeviceSuitable(VkPhysicalDevice device);
    
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
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
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkSampleCountFlagBits getMaxMsaaSampleCount();
    bool hasStencilComponent(VkFormat format);

    bool createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    void createColorResources();
    void createDepthResources();
    
    bool createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

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