#pragma once
#include "stdafx.h"
#include "VulkanHelper.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "Camera.h"
#include "Pipeline.h"
#include "DeviceModel.h"
#include "AtmosphereModel.h"
#include "TextureSampler.h"
#include "DescriptorSet.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {

private:
    std::shared_ptr<VulkanContext> _ctx;

    std::unique_ptr<Camera> _camera = nullptr;
    uint32_t _currentTargetObjectID;
    std::unordered_map<int, std::shared_ptr<DeviceModel>> _selectableObjects;

    std::shared_ptr<TextureSampler> _textureSampler;

    // Global information that we need to pass to the shader
    struct SceneInfo {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;

        alignas(4) float time;
        alignas(16) glm::vec3 cameraPosition;
        alignas(16) glm::vec3 lightColor;
    } _sceneInfo;

    std::array<std::unique_ptr<UniformBuffer<SceneInfo>>, MAX_FRAMES_IN_FLIGHT> _sceneInfoUBOs;
    std::array<std::unique_ptr<DescriptorSet>, MAX_FRAMES_IN_FLIGHT> _sceneDescriptorSets;

    struct PushConstants {
        alignas(16) glm::mat4 model;
        alignas(4) uint32_t objectID;
    } _pushConstants;

    std::vector<std::shared_ptr<DeviceModel>> _planetModels;
    std::vector<std::shared_ptr<DeviceModel>> _orbitModels;
    std::vector<std::shared_ptr<AtmosphereModel>> _atmosphereModels;

    // Normal rendering
    std::unique_ptr<Pipeline> _pipeline;
    std::unique_ptr<Pipeline> _orbitPipeline;
    std::unique_ptr<Pipeline> _atmospherePipeline;

    VkSwapchainKHR _swapChain = nullptr;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;
    void createSwapChain();
    void recreateSwapChain();
    void cleanupSwapChain();

    std::vector<VkFramebuffer> _swapChainFramebuffers;
    void createFramebuffers();

    VkRenderPass _renderPass;
    void createRenderPass();

    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage _colorImage;
    VkDeviceMemory _colorImageMemory;
    VkImageView _colorImageView;
    void createColorResources();

    VkImage _depthImage;
    VkDeviceMemory _depthImageMemory;
    VkImageView _depthImageView;
    void createDepthResources();

    // Object selection rendering (has no swapchain, offscreen rendering)
    std::unique_ptr<Pipeline> _objectSelectionPipeline;

    VkFramebuffer _objectSelectionFramebuffer;
    void createObjectSelectionFramebuffer();

    VkRenderPass _objectSelectionRenderPass;
    void createObjectSelectionRenderPass();

    VkImage _objectSelectionImage;
    VkDeviceMemory _objectSelectionImageMemory;
    VkImageView _objectSelectionImageView;
    void createObjectSelectionResources();

    VkImage _objectSelectionDepthImage;
    VkDeviceMemory _objectSelectionDepthImageMemory;
    VkImageView _objectSelectionDepthImageView;
    void createObjectSelectionDepthResources();






    std::vector<VkCommandBuffer> _commandBuffers;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    VkSampleCountFlagBits getMaxMsaaSampleCount();
    bool hasStencilComponent(VkFormat format);

    void updateUniformBuffer(uint32_t currentImage);
    
    bool createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    bool createSyncObjects();

    uint32_t querySelectionImage(float mouseX, float mouseY);

public:
    Renderer(std::shared_ptr<VulkanContext> ctx);
    ~Renderer();

    bool initialize();
    void drawFrame();
    void handleMouseClick(float mouseX, float mouseY);
    void informFramebufferResized() { _framebufferResized = true; };

    Camera* getCamera() { return _camera.get(); };
};