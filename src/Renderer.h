#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "Camera.h"
#include "Pipeline.h"
#include "DeviceModel.h"
#include "TextureSampler.h"
#include "DescriptorSet.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {

private:
    std::shared_ptr<VulkanContext> _ctx;

    std::unique_ptr<Camera> _camera = nullptr;

    std::shared_ptr<TextureSampler> _textureSampler;

    // Global information that we need to pass to the shader
    struct SceneInfo {
        alignas(4) float time;
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) glm::vec3 lightColor;
    } _sceneInfo;
    std::array<std::unique_ptr<UniformBuffer<SceneInfo>>, MAX_FRAMES_IN_FLIGHT> _sceneInfoUBOs;

    struct MVP {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    };
    std::array<std::unique_ptr<UniformBuffer<MVP>>, MAX_FRAMES_IN_FLIGHT> _mvpUBOs;
    


    std::vector<std::unique_ptr<DeviceModel>> _models;

    
    VkSwapchainKHR _swapChain = nullptr;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass;

    //VkDescriptorSetLayout _descriptorSetLayout;
    //VkDescriptorPool _descriptorPool;

    //std::vector<VkDescriptorSet> _descriptorSets;
    //std::vector<std::vector<VkDescriptorSet>> _descriptorSets;  //_descriptorSets[ModelIndex][FrameIndex]
    
    std::unique_ptr<Pipeline> _pipeline;
    std::array<std::unique_ptr<DescriptorSet>, MAX_FRAMES_IN_FLIGHT> _descriptorSets;

    std::unique_ptr<Pipeline> _orbitPipeline;

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView _colorImageView;

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;

    std::vector<VkCommandBuffer> _commandBuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    bool createSwapChain();
    bool recreateSwapChain();
    void cleanupSwapChain();

    bool createRenderPass();


    //VkDescriptorSetLayoutBinding descriptorBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage, uint32_t count = 1);
    //void createDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    //bool createDescriptorSets();

    bool createFramebuffers();
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkSampleCountFlagBits getMaxMsaaSampleCount();
    bool hasStencilComponent(VkFormat format);

    void updateUniformBuffer(uint32_t currentImage);

    void createColorResources();
    void createDepthResources();
    
    bool createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool createSyncObjects();

public:
    Renderer(std::shared_ptr<VulkanContext> ctx);
    ~Renderer();

    bool initialize();
    void drawFrame();
    void informFramebufferResized() { _framebufferResized = true; };

    Camera* getCamera() { return _camera.get(); };
};