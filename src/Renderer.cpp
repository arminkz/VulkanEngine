#include "Renderer.h"
#include "stdafx.h"
#include "VulkanHelper.h"

#include "Pipeline.h"
#include "loader/ObjLoader.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "geometry/DeviceMesh.h"
#include "geometry/MeshFactory.h"

Renderer::Renderer(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx))
{
}

Renderer::~Renderer()
{
    // Wait for any unfinished GPU tasks
    vkDeviceWaitIdle(_ctx->device);

    // Free all models
    for(size_t i=0; i<_models.size(); i++) {
        _models[i] = nullptr;
    }
    _textureSampler = nullptr;

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT; i++) {
        _mvpUBOs[i] = nullptr;
        _sceneInfoUBOs[i] = nullptr;
    }

    cleanupSwapChain();

    // Clean up Vulkan resources
    for(size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(_ctx->device, _imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(_ctx->device, _renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(_ctx->device, _inFlightFences[i], nullptr);
    }

    // for (auto framebuffer : _swapChainFramebuffers) {
    //     vkDestroyFramebuffer(_ctx->device, framebuffer, nullptr);
    // }

    _pipeline = nullptr;
    _orbitPipeline = nullptr;

    vkDestroyRenderPass(_ctx->device, _renderPass, nullptr);

    // vkDestroyDescriptorPool(_ctx->device, _descriptorPool, nullptr);
    // vkDestroyDescriptorSetLayout(_ctx->device, _descriptorSetLayout, nullptr);
    
}

bool Renderer::initialize()
{
    // Create Camera
    Camera::CameraParams cp{};
    cp.target = glm::vec3(-36.f, 0.f, 0.f);
    _camera = std::make_unique<Camera>(cp);

    // Create Texture Sampler
    _textureSampler = std::make_unique<TextureSampler>(_ctx, 10);

    // MVP UBO (Per Frame)
    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        _mvpUBOs[i] = std::make_unique<UniformBuffer<MVP>>(_ctx, 0, VK_SHADER_STAGE_VERTEX_BIT);
        _mvpUBOs[i]->update({ glm::mat4(1.f), glm::mat4(1.f), glm::mat4(1.f) });
    }

    // Scene UBO (Per Frame)
    _sceneInfo.time = 0.f;
    _sceneInfo.cameraPosition = _camera->getPosition();
    _sceneInfo.lightColor = glm::vec3(1.f);
    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        _sceneInfoUBOs[i] = std::make_unique<UniformBuffer<SceneInfo>>(_ctx, 2, VK_SHADER_STAGE_FRAGMENT_BIT);
        _sceneInfoUBOs[i]->update(_sceneInfo);
    }


    // Create Render objects
    HostMesh sphere = MeshFactory::createSphereMesh(1.f, 64, 64);
    HostMesh sphereInside = MeshFactory::createSphereMesh(1.f, 64, 64, true);
    HostMesh ring = MeshFactory::createAnnulusMesh(1.3f, 2.2f, 64);

    std::shared_ptr<DeviceMesh> sphereDMesh = std::make_shared<DeviceMesh>(_ctx, sphere);
    std::shared_ptr<DeviceMesh> sphereInsideDMesh = std::make_shared<DeviceMesh>(_ctx, sphereInside);
    std::shared_ptr<DeviceMesh> ringDMesh = std::make_shared<DeviceMesh>(_ctx, ring);

    // SkySphere
    std::shared_ptr<DeviceTexture> skyTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_stars_milky_way.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 skyModelMat = glm::mat4(1.f);
    skyModelMat = glm::scale(skyModelMat, glm::vec3(500.f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereInsideDMesh, skyModelMat, skyTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));
    _models[0]->material.ambientStrength = 0.3f;
    _models[0]->material.specularStrength = 0.0f;
    _models[0]->updateMaterial();

    //Sun
    std::shared_ptr<DeviceTexture> sunTexture = std::make_shared<DeviceTexture>(_ctx, "textures/2k_sun.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> sunPerlinTexture = std::make_shared<DeviceTexture>(_ctx, "textures/perlin.png", VK_FORMAT_R8G8B8A8_UNORM);
    glm::mat4 sunModelMat = glm::mat4(1.f);
    sunModelMat = glm::scale(sunModelMat, glm::vec3(3.f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereDMesh, sunModelMat, sunTexture, nullptr, nullptr, nullptr, sunPerlinTexture, _textureSampler));
    _models[1]->material.sunShadeMode = 1;
    _models[1]->updateMaterial();

    // Earth
    std::shared_ptr<DeviceTexture> colorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_earth_daymap.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> unlitTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_earth_nightmap.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> normalTexture = std::make_shared<DeviceTexture>(_ctx, "textures/EarthNormal.png", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<DeviceTexture> specularTexture = std::make_shared<DeviceTexture>(_ctx, "textures/EarthSpec.png", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<DeviceTexture> overlayTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_earth_clouds.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 earthModelMat = glm::mat4(1.f);
    earthModelMat = glm::translate(glm::mat4(1.f), glm::vec3(-19.f, 0.f, 0.f));
    earthModelMat = glm::scale(earthModelMat, glm::vec3(1.f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereDMesh, earthModelMat, colorTexture, unlitTexture, normalTexture, specularTexture, overlayTexture, _textureSampler));

    // Moon
    std::shared_ptr<DeviceTexture> moonColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/2k_moon.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 moonModelMat = glm::mat4(1.f);
    moonModelMat = glm::translate(glm::mat4(1.f), glm::vec3(-19.f, 6.f, 0.f));
    moonModelMat = glm::scale(moonModelMat, glm::vec3(0.5f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereDMesh, moonModelMat, moonColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Mars
    std::shared_ptr<DeviceTexture> marsColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_mars.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 marsModelMat = glm::mat4(1.f);
    marsModelMat = glm::translate(glm::mat4(1.f), glm::vec3(-26.f, 0.f, 0.f));
    marsModelMat = glm::scale(marsModelMat, glm::vec3(0.8f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereDMesh, marsModelMat, marsColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Saturn
    std::shared_ptr<DeviceTexture> saturnColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_saturn.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 saturnModelMat = glm::mat4(1.f);
    saturnModelMat = glm::translate(glm::mat4(1.f), glm::vec3(-36.f, 0.f, 0.f));
    saturnModelMat = glm::scale(saturnModelMat, glm::vec3(1.5f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereDMesh, saturnModelMat, saturnColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Saturn Ring
    std::shared_ptr<DeviceTexture> ringTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_saturn_ring_alpha.png", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 ringModelMat = glm::mat4(1.f);
    ringModelMat = glm::translate(glm::mat4(1.f), glm::vec3(-36.f, 0.f, 0.f));
    ringModelMat = glm::scale(ringModelMat, glm::vec3(1.5f));
    ringModelMat = glm::rotate(ringModelMat, glm::radians(107.f), glm::vec3(1.f, 0.f, 0.f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, ringDMesh, ringModelMat, ringTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));
    _models[6]->material.ambientStrength = 0.3f;
    _models[6]->updateMaterial();


    // Neptune
    std::shared_ptr<DeviceTexture> neptuneColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/2k_neptune.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 neptuneModelMat = glm::mat4(1.f);
    neptuneModelMat = glm::translate(glm::mat4(1.f), glm::vec3(-46.f, 0.f, 0.f));
    neptuneModelMat = glm::scale(neptuneModelMat, glm::vec3(1.f));
    _models.push_back(std::make_unique<DeviceModel>(_ctx, sphereDMesh, neptuneModelMat, neptuneColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));



    _msaaSamples = getMaxMsaaSampleCount();
    spdlog::info("Max MSAA samples: {}", static_cast<int>(_msaaSamples));

    // Create swap chain
    if (!createSwapChain()) return false;

    // Create render pass
    if (!createRenderPass()) return false;

    // Create per-frame descriptor sets
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        std::vector<Descriptor> perSceneDescriptors = {
            Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1, _mvpUBOs[i]->getDescriptorInfo()),         // MVP
            Descriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _sceneInfoUBOs[i]->getDescriptorInfo()), // Scene UBO
        };

        _descriptorSets[i] = std::make_unique<DescriptorSet>(_ctx, perSceneDescriptors);
    }

    VkDescriptorSetLayout perModelDSL = _models[0]->getDescriptorSet()->getDescriptorSetLayout();
    VkDescriptorSetLayout perFrameDSL = _descriptorSets[0]->getDescriptorSetLayout();

    // Create graphics pipeline
    PipelineParams pipelineParams {
        { perFrameDSL, perModelDSL },
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) },
        _renderPass,
        _msaaSamples
    };
    _pipeline = std::make_unique<Pipeline>(_ctx, "shaders/vert.spv", "shaders/frag.spv", pipelineParams);

    // Create color resources
    createColorResources();

    // Create depth resources
    createDepthResources();

    // Create framebuffers
    if (!createFramebuffers()) return false;

    // Create command buffers
    if (!createCommandBuffers()) return false;

    // Create sync objects
    if (!createSyncObjects()) return false;
}

VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Check for mailbox mode first (triple buffering)
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    // Fallback to FIFO mode (double buffering)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL_GetWindowSizeInPixels(_ctx->window, &width, &height);
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

bool Renderer::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_ctx->physicalDevice, _ctx->surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = _ctx->surface;

    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(_ctx->physicalDevice, _ctx->surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapChainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_ctx->device, &swapChainCreateInfo, nullptr, &_swapChain) != VK_SUCCESS) {
        spdlog::error("Failed to create swap chain!");
        return false;
    } else {
        spdlog::info("Swap chain created successfully");
    }

    vkGetSwapchainImagesKHR(_ctx->device, _swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_ctx->device, _swapChain, &imageCount, _swapChainImages.data());

    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    _swapChainImageViews.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++) {

        // Create image views for each swap chain image
        _swapChainImageViews[i] = VulkanHelper::createImageView(_ctx, _swapChainImages[i], _swapChainImageFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    return true;
}

bool Renderer::recreateSwapChain() {
    vkDeviceWaitIdle(_ctx->device);

    // Destroy old swap chain
    cleanupSwapChain();

    // Create new swap chain
    createSwapChain();
    createColorResources();
    createDepthResources();
    createFramebuffers();
    return true;
}

void Renderer::cleanupSwapChain() {
    // Clean up color resources
    vkDestroyImageView(_ctx->device, _colorImageView, nullptr);
    vkDestroyImage(_ctx->device, _colorImage, nullptr);
    vkFreeMemory(_ctx->device, _colorImageMemory, nullptr);

    // Clean up depth resources
    vkDestroyImageView(_ctx->device, _depthImageView, nullptr);
    vkDestroyImage(_ctx->device, _depthImage, nullptr);
    vkFreeMemory(_ctx->device, _depthImageMemory, nullptr);

    // Clean up swap chain resources
    for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(_ctx->device, _swapChainFramebuffers[i], nullptr);
    }
    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        vkDestroyImageView(_ctx->device, _swapChainImageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(_ctx->device, _swapChain, nullptr);
}

bool Renderer::createRenderPass() {
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = _msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = _swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = _msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Attach depth attachment
    subpass.pResolveAttachments = &colorAttachmentResolveRef; // Attach resolve attachment

    // Subpass has to wait for the swapchain to be ready before it can start rendering
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_ctx->device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        spdlog::error("Failed to create render pass!");
        return false;
    }else {
        spdlog::info("Render pass created successfully");
        return true;
    }
}

bool Renderer::createFramebuffers() {
    _swapChainFramebuffers.resize(_swapChainImageViews.size());

    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            _colorImageView,
            _depthImageView,
            _swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_ctx->device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create framebuffer!");
            return false;
        }
    }

    return true;
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Update MVP UBO
    MVP mvp{};
    mvp.model = glm::mat4(1.0f);
    mvp.view = _camera->getViewMatrix(); 
    mvp.projection = glm::perspective(glm::radians(45.0f), (float)_swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 1000.0f);
    mvp.projection[1][1] *= -1; // Invert Y axis for Vulkan
    _mvpUBOs[currentImage]->update(mvp);

    // Update SceneInfo UBO
    _sceneInfo.time = time;
    _sceneInfo.cameraPosition = _camera->getPosition();
    _sceneInfoUBOs[currentImage]->update(_sceneInfo);
}


void Renderer::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat(); // Find a suitable depth format

    VulkanHelper::createImage(_ctx, _swapChainExtent.width, _swapChainExtent.height, 
        depthFormat,
        1, // Number of mip levels
        _msaaSamples, // Number of samples for multisampling
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _depthImage, _depthImageMemory);

    //transitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    _depthImageView = VulkanHelper::createImageView(_ctx, _depthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

}

void Renderer::createColorResources() {

    VkFormat colorFormat = _swapChainImageFormat; // Use the swap chain image format for color resources

    VulkanHelper::createImage(_ctx, 
        _swapChainExtent.width,
        _swapChainExtent.height, 
        colorFormat,
        1,
        _msaaSamples, // Number of samples for multisampling
        VK_IMAGE_TILING_OPTIMAL, // Image tiling
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _colorImage, _colorImageMemory);

    _colorImageView = VulkanHelper::createImageView(_ctx, _colorImage, colorFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}

VkFormat Renderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_ctx->physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    spdlog::error("Failed to find supported format!");
    return VK_FORMAT_UNDEFINED;
}

VkFormat Renderer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkSampleCountFlagBits Renderer::getMaxMsaaSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(_ctx->physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT; // Fallback to 1 sample
}

bool Renderer::hasStencilComponent(VkFormat format) {
    switch (format) {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return true;
        default:
            return false;
    }
}


bool Renderer::createCommandBuffers() {
    // Allocate command buffer
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _ctx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

    if (vkAllocateCommandBuffers(_ctx->device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        spdlog::error("Failed to allocate command buffers!");
        return false;
    } else {
        spdlog::info("Command buffers allocated successfully");
        return true;
    }
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        spdlog::error("Failed to begin recording command buffer!");
        return;
    }

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.framebuffer = _swapChainFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // Clear color
    clearValues[1].depthStencil = { 1.0f, 0 };             // Clear depth value
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    _pipeline->bind(commandBuffer);
    
    // Draw call here (e.g., vkCmdDraw)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swapChainExtent.width;
    viewport.height = (float) _swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    // Iterate over scene models
    for(int m=0; m<static_cast<int>(_models.size()); m++) {
        VkBuffer vertexBuffers[] = {_models[m]->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _models[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 2> descriptorSets = {
            _descriptorSets[_currentFrame]->getDescriptorSet(), // Per-frame descriptor set
            _models[m]->getDescriptorSet()->getDescriptorSet()  // Per-model descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

        vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &_models[m]->getModelMatrix());

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_models[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }
    
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        spdlog::error("Failed to record command buffer!");
        return;
    }
}

bool Renderer::createSyncObjects() {
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; //Initially signaled

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(_ctx->device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_ctx->device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(_ctx->device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            spdlog::error("Failed to create semaphores / fences for frame {}!", i);
            return false;
        }
    }
    return true;
}

void Renderer::drawFrame() {
    vkWaitForFences(_ctx->device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_ctx->device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to acquire swap chain image!");
        return;
    }

    // Reset the fence before drawing
    vkResetFences(_ctx->device, 1, &_inFlightFences[_currentFrame]);

    // Update uniform buffer
    updateUniformBuffer(_currentFrame);

    vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);
    recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_ctx->graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        spdlog::error("Failed to submit draw command buffer!");
        return;
    }

    // Present the image to the swap chain (after the command buffer is done)
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(_ctx->presentQueue, &presentInfo);
    
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}