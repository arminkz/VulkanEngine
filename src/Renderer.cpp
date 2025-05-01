#include "Renderer.h"
#include "stdafx.h"
#include "VulkanHelper.h"

#include "Pipeline.h"
#include "loader/ObjLoader.h"
#include "geometry/Vertex.h"
#include "geometry/HostMesh.h"
#include "geometry/DeviceMesh.h"
#include "geometry/MeshFactory.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Renderer::Renderer(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx))
{
}

Renderer::~Renderer()
{
    // Wait for any unfinished GPU tasks
    vkDeviceWaitIdle(_ctx->device);

    // Free all models
    for(size_t i=0; i<_planetModels.size(); i++) {
        _planetModels[i] = nullptr;
    }
    for(size_t i=0; i<_orbitModels.size(); i++) {
        _orbitModels[i] = nullptr;
    }
    for(size_t i=0; i<_atmosphereModels.size(); i++) {
        _atmosphereModels[i] = nullptr;
    }
    _sunModel = nullptr;
    for(size_t i=0; i<_selectableObjects.size(); i++) {
        _selectableObjects[i] = nullptr;
    }

    _textureSampler = nullptr;

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT; i++) {
        _sceneInfoUBOs[i] = nullptr;
    }

    cleanupSwapChain();

    // Clean up Vulkan resources
    for(size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(_ctx->device, _imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(_ctx->device, _renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(_ctx->device, _inFlightFences[i], nullptr);
    }

    _pipeline = nullptr;
    _orbitPipeline = nullptr;
    _sunPipeline = nullptr;
    _atmospherePipeline = nullptr;
    _objectSelectionPipeline = nullptr;

    vkDestroyRenderPass(_ctx->device, _renderPass, nullptr);
    vkDestroyRenderPass(_ctx->device, _objectSelectionRenderPass, nullptr);

    //Destroy dummy texture
    DeviceTexture::cleanupDummy();

    _ctx = nullptr;
    
}

bool Renderer::initialize()
{
    // Create Texture Sampler
    _textureSampler = std::make_unique<TextureSampler>(_ctx, 10);

    // Create Render objects
    HostMesh sphere = MeshFactory::createSphereMesh(1.f, 64, 64);
    HostMesh sphereInside = MeshFactory::createSphereMesh(1.f, 64, 64, true);
    HostMesh ring = MeshFactory::createAnnulusMesh(1.3f, 2.2f, 64);
    HostMesh quad =  MeshFactory::createQuadMesh(1.f, 1.f, true);

    std::shared_ptr<DeviceMesh> sphereDMesh = std::make_shared<DeviceMesh>(_ctx, sphere);
    std::shared_ptr<DeviceMesh> sphereInsideDMesh = std::make_shared<DeviceMesh>(_ctx, sphereInside);
    std::shared_ptr<DeviceMesh> ringDMesh = std::make_shared<DeviceMesh>(_ctx, ring);
    std::shared_ptr<DeviceMesh> quadDMesh = std::make_shared<DeviceMesh>(_ctx, quad);

    float orbitRadMercury = 10.f;
    float orbitRadVenus = 15.f;
    float orbitRadEarth = 20.f;
    float orbitRadMoon = 6.f;
    float orbitRadMars = 28.f;
    float orbitRadJupiter = 52.f;
    float orbitRadSaturn = 75.f;
    float orbitRadUranus = 100.f;
    float orbitRadNeptune = 130.f;
    float orbitRadPluto = 160.f;

    float sizeSun = 3.f;
    float sizeMercury = 0.5f;
    float sizeVenus = 0.8f;
    float sizeEarth = 1.f;
    float sizeMoon = 0.3f;
    float sizeMars = 0.6f;
    float sizeJupiter = 1.5f;
    float sizeSaturn = 1.2f;
    float sizeSaturnRing = 1.2f * sizeSaturn;
    float sizeUranus = 1.0f;
    float sizeNeptune = 1.0f;
    float sizePluto = 0.4f;

    // SkySphere
    std::shared_ptr<DeviceTexture> skyTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_stars_milky_way.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 skyModelMat = glm::mat4(1.f);
    skyModelMat = glm::scale(skyModelMat, glm::vec3(500.f));
    std::shared_ptr<DeviceModel> skySphere = std::make_shared<DeviceModel>("skySphere", _ctx, sphereInsideDMesh, skyModelMat, skyTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    skySphere->material.ambientStrength = 0.3f;
    skySphere->material.specularStrength = 0.0f;
    skySphere->updateMaterial();
    _planetModels.push_back(skySphere);

    //Sun
    std::shared_ptr<DeviceTexture> sunTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_sun.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> sunPerlinTexture = std::make_shared<DeviceTexture>(_ctx, "textures/perlin.png", VK_FORMAT_R8G8B8A8_UNORM);
    glm::mat4 sunModelMat = glm::mat4(1.f);
    sunModelMat = glm::scale(sunModelMat, glm::vec3(sizeSun));
    std::shared_ptr<DeviceModel> sun = std::make_shared<DeviceModel>("sun", _ctx, sphereDMesh, sunModelMat, sunTexture, nullptr, nullptr, nullptr, sunPerlinTexture, _textureSampler);
    sun->material.sunShadeMode = 1;
    sun->updateMaterial();
    _sunModel = sun;
    // Sun is selectable
    _selectableObjects[sun->getID()] = sun;

    //Sun atmosphere
    glm::mat4 sunAtmosphereModelMat = glm::mat4(1.f);
    sunAtmosphereModelMat = glm::scale(sunAtmosphereModelMat, glm::vec3(sizeSun * 1.2f));
    std::shared_ptr<AtmosphereModel> sunAtmosphere = std::make_shared<AtmosphereModel>(_ctx, sphereDMesh, sunAtmosphereModelMat, glm::vec3(1.f, 0.8f, 0.5f), 1.f, 4.0f, true);
    _atmosphereModels.push_back(sunAtmosphere);

    // Mercury
    std::shared_ptr<DeviceTexture> mercuryColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/mercury/8k_mercury.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 mercuryModelMat = glm::mat4(1.f);
    mercuryModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadMercury, 0.f, 0.f));
    mercuryModelMat = glm::scale(mercuryModelMat, glm::vec3(sizeMercury));
    std::shared_ptr<DeviceModel> mercury = std::make_shared<DeviceModel>("mercury", _ctx, sphereDMesh, mercuryModelMat, mercuryColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(mercury);
    // Mercury is selectable
    _selectableObjects[mercury->getID()] = mercury;

    // Mercury Orbit
    glm::mat4 mercuryOrbitModelMat = glm::mat4(1.f);
    mercuryOrbitModelMat = glm::scale(mercuryOrbitModelMat, glm::vec3(orbitRadMercury * 2.f));
    mercuryOrbitModelMat = glm::rotate(mercuryOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("mercury_orbit", _ctx, quadDMesh, mercuryOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Venus
    std::shared_ptr<DeviceTexture> venusColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/venus/4k_venus_atmosphere.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 venusModelMat = glm::mat4(1.f);
    venusModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadVenus, 0.f, 0.f));
    venusModelMat = glm::scale(venusModelMat, glm::vec3(sizeVenus));
    std::shared_ptr<DeviceModel> venus = std::make_shared<DeviceModel>("venus", _ctx, sphereDMesh, venusModelMat, venusColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(venus);
    // Venus is selectable
    _selectableObjects[venus->getID()] = venus;

    // Earth
    std::shared_ptr<DeviceTexture> colorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/10k_earth_day.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> unlitTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/10k_earth_night.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> normalTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/2k_earth_normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<DeviceTexture> specularTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/2k_earth_specular.jpeg", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<DeviceTexture> overlayTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/8k_earth_clouds.png", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 earthModelMat = glm::mat4(1.f);
    earthModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadEarth, 0.f, 0.f));
    earthModelMat = glm::scale(earthModelMat, glm::vec3(sizeEarth));
    std::shared_ptr<DeviceModel> earth = std::make_shared<DeviceModel>("earth", _ctx, sphereDMesh, earthModelMat, colorTexture, unlitTexture, normalTexture, specularTexture, overlayTexture, _textureSampler);
    _planetModels.push_back(earth);
    // Earth is selectable
    _selectableObjects[earth->getID()] = earth;

    // Earth Orbit
    glm::mat4 earthOrbitModelMat = glm::mat4(1.f);
    earthOrbitModelMat = glm::scale(earthOrbitModelMat, glm::vec3(orbitRadEarth * 2.f));
    earthOrbitModelMat = glm::rotate(earthOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("earth_orbit", _ctx, quadDMesh, earthOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Earth Atmosphere
    glm::mat4 earthAtmosphereModelMat = glm::mat4(1.f);
    earthAtmosphereModelMat = glm::translate(earthAtmosphereModelMat, glm::vec3(orbitRadEarth, 0.f, 0.f));
    earthAtmosphereModelMat = glm::scale(earthAtmosphereModelMat, glm::vec3(sizeEarth * 1.03f));
    _atmosphereModels.push_back(std::make_shared<AtmosphereModel>(_ctx, sphereDMesh, earthAtmosphereModelMat, glm::vec3(0.45f, 0.55f, 1.f), 3.f, 3.0, false));

    // Moon
    std::shared_ptr<DeviceTexture> moonColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/moon/8k_moon.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 moonModelMat = glm::mat4(1.f);
    moonModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadEarth + orbitRadMoon, 0.f, 0.f));
    moonModelMat = glm::scale(moonModelMat, glm::vec3(sizeMoon));
    std::shared_ptr<DeviceModel> moon = std::make_shared<DeviceModel>("moon", _ctx, sphereDMesh, moonModelMat, moonColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(moon);
    // Moon is selectable
    _selectableObjects[moon->getID()] = moon;

    // Moon Orbit
    glm::mat4 moonOrbitModelMat = glm::mat4(1.f);
    moonOrbitModelMat = glm::translate(moonOrbitModelMat, glm::vec3(sizeEarth, 0.f, 0.f));
    moonOrbitModelMat = glm::scale(moonOrbitModelMat, glm::vec3(orbitRadMoon * 2.f));
    moonOrbitModelMat = glm::rotate(moonOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("moon_orbit", _ctx, quadDMesh, moonOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Mars
    std::shared_ptr<DeviceTexture> marsColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_mars.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 marsModelMat = glm::mat4(1.f);
    marsModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadMars, 0.f, 0.f));
    marsModelMat = glm::scale(marsModelMat, glm::vec3(sizeMars));
    std::shared_ptr<DeviceModel> mars = std::make_shared<DeviceModel>("mars", _ctx, sphereDMesh, marsModelMat, marsColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(mars);
    // Mars is selectable
    _selectableObjects[mars->getID()] = mars;

    // Mars Orbit
    glm::mat4 marsOrbitModelMat = glm::mat4(1.f);
    marsOrbitModelMat = glm::scale(marsOrbitModelMat, glm::vec3(orbitRadMars * 2.f));
    marsOrbitModelMat = glm::rotate(marsOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("mars_orbit", _ctx, quadDMesh, marsOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Saturn
    std::shared_ptr<DeviceTexture> saturnColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/saturn/8k_saturn.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 saturnModelMat = glm::mat4(1.f);
    saturnModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadSaturn, 0.f, 0.f));
    saturnModelMat = glm::scale(saturnModelMat, glm::vec3(1.5f));
    std::shared_ptr<DeviceModel> saturn = std::make_shared<DeviceModel>("saturn", _ctx, sphereDMesh, saturnModelMat, saturnColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(saturn);
    // Saturn is selectable
    _selectableObjects[saturn->getID()] = saturn;

    // Saturn Ring
    std::shared_ptr<DeviceTexture> ringTexture = std::make_shared<DeviceTexture>(_ctx, "textures/saturn/8k_saturn_ring_alpha.png", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 ringModelMat = glm::mat4(1.f);
    ringModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadSaturn, 0.f, 0.f));
    ringModelMat = glm::scale(ringModelMat, glm::vec3(sizeSaturn));
    ringModelMat = glm::rotate(ringModelMat, glm::radians(107.f), glm::vec3(1.f, 0.f, 0.f));
    _planetModels.push_back(std::make_shared<DeviceModel>("saturn_ring", _ctx, ringDMesh, ringModelMat, ringTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler));
    _planetModels[6]->material.ambientStrength = 0.3f;
    _planetModels[6]->updateMaterial();

    // Saturn Orbit
    glm::mat4 saturnOrbitModelMat = glm::mat4(1.f);
    saturnOrbitModelMat = glm::scale(saturnOrbitModelMat, glm::vec3(orbitRadSaturn * 2.f));
    saturnOrbitModelMat = glm::rotate(saturnOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("saturn_orbit", _ctx, quadDMesh, saturnOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Neptune
    std::shared_ptr<DeviceTexture> neptuneColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/2k_neptune.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 neptuneModelMat = glm::mat4(1.f);
    neptuneModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadNeptune, 0.f, 0.f));
    neptuneModelMat = glm::scale(neptuneModelMat, glm::vec3(sizeNeptune));
    std::shared_ptr<DeviceModel> neptune = std::make_shared<DeviceModel>("neptune", _ctx, sphereDMesh, neptuneModelMat, neptuneColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(neptune);
    // Neptune is selectable
    _selectableObjects[neptune->getID()] = neptune;

    // Neptune Orbit
    glm::mat4 neptuneOrbitModelMat = glm::mat4(1.f);
    neptuneOrbitModelMat = glm::scale(neptuneOrbitModelMat, glm::vec3(orbitRadNeptune * 2.f));
    neptuneOrbitModelMat = glm::rotate(neptuneOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("neptune_orbit", _ctx, quadDMesh, neptuneOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Create Camera
    _currentTargetObjectID = 5; // Earth
    Camera::CameraParams cp{};
    cp.target = _selectableObjects[_currentTargetObjectID]->getPosition();
    _camera = std::make_unique<Camera>(cp);

    // Scene UBO (Per Frame)
    _sceneInfo.view = _camera->getViewMatrix();
    _sceneInfo.projection = glm::perspective(glm::radians(45.f), (float)_swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 1000.f);
    _sceneInfo.projection[1][1] *= -1; // Invert Y axis for Vulkan

    _sceneInfo.time = 0.f;
    _sceneInfo.cameraPosition = _camera->getPosition();
    _sceneInfo.lightColor = glm::vec3(1.f);
    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        _sceneInfoUBOs[i] = std::make_unique<UniformBuffer<SceneInfo>>(_ctx);
        _sceneInfoUBOs[i]->update(_sceneInfo);
    }
    
    // MSAA
    _msaaSamples = getMaxMsaaSampleCount();
    spdlog::info("Max MSAA samples: {}", static_cast<int>(_msaaSamples));

    // Create swap chain
    createSwapChain();

    // Create render pass
    createRenderPass();
    createObjectSelectionRenderPass();

    // Create per-frame descriptor sets
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<Descriptor> perSceneDescriptors = {
            Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, _sceneInfoUBOs[i]->getDescriptorInfo()), // Scene UBO
        };
        _sceneDescriptorSets[i] = std::make_unique<DescriptorSet>(_ctx, perSceneDescriptors);
    }

    VkDescriptorSetLayout perModelDSL = _planetModels[0]->getDescriptorSet()->getDescriptorSetLayout();
    VkDescriptorSetLayout perFrameDSL = _sceneDescriptorSets[0]->getDescriptorSetLayout();
    VkDescriptorSetLayout atmosphereDSL = _atmosphereModels[0]->getDescriptorSet()->getDescriptorSetLayout();

    // Create graphics pipeline
    PipelineParams pipelineParams {
        { perFrameDSL, perModelDSL },
        { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) },
        _renderPass,
        _msaaSamples,
        true,
        true,

    };
    _pipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/shader_frag.spv", pipelineParams);

    // Create sun pipeline
    PipelineParams sunPipelineParams {};
    sunPipelineParams.descriptorSetLayouts = { perFrameDSL, perModelDSL };
    sunPipelineParams.pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };
    sunPipelineParams.renderPass = _renderPass;
    sunPipelineParams.msaaSamples = _msaaSamples;
    _sunPipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/sun_frag.spv", sunPipelineParams);


    // Create orbit pipeline
    PipelineParams orbitPipelineParams {
        { perFrameDSL },                                          //Descriptor set layout
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants) }, //Push constants
        _renderPass,                                              //Render pass
        _msaaSamples,
        true,
        false
    };
    _orbitPipeline = std::make_unique<Pipeline>(_ctx, "spv/orbit_vert.spv", "spv/orbit_frag.spv", orbitPipelineParams);


    // Create atmosphere pipeline
    PipelineParams atmospherePipelineParams {};
    atmospherePipelineParams.descriptorSetLayouts = { perFrameDSL, atmosphereDSL };
    atmospherePipelineParams.pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };
    atmospherePipelineParams.renderPass = _renderPass;
    atmospherePipelineParams.msaaSamples = _msaaSamples;
    atmospherePipelineParams.depthTest = true;
    atmospherePipelineParams.depthWrite = false;
    atmospherePipelineParams.transparency = true;
    atmospherePipelineParams.backSide = true;
    _atmospherePipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/atmosphere_frag.spv", atmospherePipelineParams);

    PipelineParams objectSelectionPipelineParams {};
    objectSelectionPipelineParams.descriptorSetLayouts = { perFrameDSL, perModelDSL };
    objectSelectionPipelineParams.pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants) };
    objectSelectionPipelineParams.renderPass = _objectSelectionRenderPass;
    objectSelectionPipelineParams.transparency = false; // No transparency for object selection
    _objectSelectionPipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/objectselect_frag.spv", objectSelectionPipelineParams);

    createColorResources();
    createDepthResources();
    createFramebuffers();

    createObjectSelectionResources();
    createObjectSelectionDepthResources();
    createObjectSelectionFramebuffer();

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

void Renderer::createSwapChain() {
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
        throw std::runtime_error("Failed to create swap chain!");
    }
    spdlog::info("Swap chain created successfully");

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

}

void Renderer::recreateSwapChain() {
    vkDeviceWaitIdle(_ctx->device);

    // Destroy old swap chain
    cleanupSwapChain();

    // Create new swap chain
    createSwapChain();
    createColorResources();
    createDepthResources();
    createFramebuffers();

    createObjectSelectionResources();
    createObjectSelectionDepthResources();
    createObjectSelectionFramebuffer();
}

void Renderer::cleanupSwapChain() {
    // Clean up object selection color resources
    vkDestroyImageView(_ctx->device, _objectSelectionImageView, nullptr);
    vkDestroyImage(_ctx->device, _objectSelectionImage, nullptr);
    vkFreeMemory(_ctx->device, _objectSelectionImageMemory, nullptr);

    // clean up object selection depth resources
    vkDestroyImageView(_ctx->device, _objectSelectionDepthImageView, nullptr);
    vkDestroyImage(_ctx->device, _objectSelectionDepthImage, nullptr);
    vkFreeMemory(_ctx->device, _objectSelectionDepthImageMemory, nullptr);

    // Clean up object selection framebuffers
    vkDestroyFramebuffer(_ctx->device, _objectSelectionFramebuffer, nullptr);

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

void Renderer::createRenderPass() {
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = _msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = _swapChainImageFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

    VkAttachmentReference resolveAttachmentRef{};
    resolveAttachmentRef.attachment = 2;
    resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Attach depth attachment
    subpass.pResolveAttachments = &resolveAttachmentRef;   // Attach resolve attachment

    // Subpass has to wait for the swapchain to be ready before it can start rendering
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, resolveAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_ctx->device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
    spdlog::info("Render pass created successfully");
}

void Renderer::createObjectSelectionRenderPass() {

    VkAttachmentDescription idAttachment{};
    idAttachment.format = VK_FORMAT_R32_UINT;  // Stores uint object IDs
    idAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    idAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    idAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    idAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    idAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    idAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    idAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; // for reading pixel data

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat(); // same as usual
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference idAttachmentRef{};
    idAttachmentRef.attachment = 0;
    idAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &idAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { idAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_ctx->device, &renderPassInfo, nullptr, &_objectSelectionRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create object selection render pass!");
    } else {
        spdlog::info("Object selection render pass created successfully");
    }
}

void Renderer::createFramebuffers() {
    _swapChainFramebuffers.resize(_swapChainImageViews.size());

    for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            _colorImageView,
            _depthImageView,
            _swapChainImageViews[i] // Write resolved color to swap chain image view
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
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void Renderer::createObjectSelectionFramebuffer() {
    std:: array<VkImageView, 2> attachments = {
        _objectSelectionImageView,
        _objectSelectionDepthImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _objectSelectionRenderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = _swapChainExtent.width;
    framebufferInfo.height = _swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(_ctx->device, &framebufferInfo, nullptr, &_objectSelectionFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create framebuffer!");
    }
}

void Renderer::createObjectSelectionDepthResources()
{
    VkFormat depthFormat = findDepthFormat(); // Find a suitable depth format

    VulkanHelper::createImage(_ctx, _swapChainExtent.width, _swapChainExtent.height, 
        depthFormat,
        1,                     // Number of mip levels
        VK_SAMPLE_COUNT_1_BIT, // Number of samples for multisampling
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _objectSelectionDepthImage, _objectSelectionDepthImageMemory);

    //transitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    _objectSelectionDepthImageView = VulkanHelper::createImageView(_ctx, _objectSelectionDepthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Update camera position based on time
    _camera->advanceAnimation(time - _sceneInfo.time);

    // Rotate earth
    float earthRotationSpeed = 0.002f; // Adjust this value to control the rotation speed
    //float earthRotationAngle = time * earthRotationSpeed;
    _planetModels[4]->setModelMatrix(glm::rotate(_planetModels[4]->getModelMatrix(), earthRotationSpeed, glm::vec3(0.f, 0.f, 1.f)));

    // Update SceneInfo UBO
    _sceneInfo.view = _camera->getViewMatrix();
    _sceneInfo.projection = glm::perspective(glm::radians(45.f), (float)_swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 1000.f);
    _sceneInfo.projection[1][1] *= -1; // Invert Y axis for Vulkan
    _sceneInfo.time = time;
    _sceneInfo.cameraPosition = _camera->getPosition();
    _sceneInfoUBOs[currentImage]->update(_sceneInfo);
}


void Renderer::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat(); // Find a suitable depth format

    VulkanHelper::createImage(_ctx, _swapChainExtent.width, _swapChainExtent.height, 
        depthFormat,
        1,            // Number of mip levels
        _msaaSamples, // Number of samples for multisampling
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _depthImage, _depthImageMemory);

    //transitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    _depthImageView = VulkanHelper::createImageView(_ctx, _depthImage, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

}

void Renderer::createObjectSelectionResources() {

    VkFormat idFormat = VK_FORMAT_R32_UINT; // Use a format that can store uint IDs

    VulkanHelper::createImage(_ctx,
        _swapChainExtent.width, 
        _swapChainExtent.height, 
        idFormat,
        1,                     // Number of mip levels (not used here)
        VK_SAMPLE_COUNT_1_BIT, // No multisampling for ID attachment
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // Color attachment and transfer source
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        _objectSelectionImage, _objectSelectionImageMemory);

    _objectSelectionImageView = VulkanHelper::createImageView(_ctx, _objectSelectionImage, idFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
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

    // Draw sun model
    _sunPipeline->bind(commandBuffer);
    VkBuffer vertexBuffers[] = {_sunModel->getDeviceMesh()->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
    vkCmdBindIndexBuffer(commandBuffer, _sunModel->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

    std::array<VkDescriptorSet, 2> descriptorSets = {
        _sceneDescriptorSets[_currentFrame]->getDescriptorSet(),  // Per-frame descriptor set
        _sunModel->getDescriptorSet()->getDescriptorSet()          // Per-model descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _sunPipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

    // Push constants for model
    PushConstants pushConstants{};
    pushConstants.model = _sunModel->getModelMatrix();
    pushConstants.objectID = _sunModel->getID();
    vkCmdPushConstants(commandBuffer, _sunPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_sunModel->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);

    // Iterate over planets
    _pipeline->bind(commandBuffer);
    for(int m=0; m<static_cast<int>(_planetModels.size()); m++) {
        VkBuffer vertexBuffers[] = {_planetModels[m]->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _planetModels[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 2> descriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet(),  // Per-frame descriptor set
            _planetModels[m]->getDescriptorSet()->getDescriptorSet()  // Per-model descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

        // Push constants for model
        PushConstants pushConstants{};
        pushConstants.model = _planetModels[m]->getModelMatrix();
        vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_planetModels[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

    // Draw atmosphere models
    _atmospherePipeline->bind(commandBuffer);

    for(int m=0; m<static_cast<int>(_atmosphereModels.size()); m++) {
        VkBuffer vertexBuffers[] = {_atmosphereModels[m]->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _atmosphereModels[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 2> descriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet(),      // Per-frame descriptor set
            _atmosphereModels[m]->getDescriptorSet()->getDescriptorSet()  // Per-model descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _atmospherePipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

        // Push constants for model
        PushConstants pushConstants{};
        pushConstants.model = _atmosphereModels[m]->getModelMatrix();
        vkCmdPushConstants(commandBuffer, _atmospherePipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_atmosphereModels[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

    // // Draw orbit models
    // _orbitPipeline->bind(commandBuffer);

    // for(int m=0; m<static_cast<int>(_orbitModels.size()); m++) {
    //     VkBuffer vertexBuffers[] = {_orbitModels[m]->getDeviceMesh()->getVertexBuffer()};
    //     VkDeviceSize offsets[] = {0};
    //     vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
    //     vkCmdBindIndexBuffer(commandBuffer, _orbitModels[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

    //     std::array<VkDescriptorSet, 1> descriptorSets = {
    //         _sceneDescriptorSets[_currentFrame]->getDescriptorSet() // Per-frame descriptor set
    //     };
    //     vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _orbitPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
    //     vkCmdPushConstants(commandBuffer, _orbitPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &_orbitModels[m]->getModelMatrix());
    //     vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_orbitModels[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    // }
    
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


uint32_t Renderer::querySelectionImage(float mouseX, float mouseY) {

    VkCommandBuffer cmdBuffer = VulkanHelper::beginSingleTimeCommands(_ctx);

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _objectSelectionRenderPass;
    renderPassBeginInfo.framebuffer = _objectSelectionFramebuffer;
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color.uint32[0] = 0;                    // Clear color (ID attachment)
    clearValues[1].depthStencil = { 1.0f, 0 };             // Clear depth value
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swapChainExtent.width;
    viewport.height = (float) _swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    //In Object Selection we only care about the pixel under the mouse position, set scissor rect to a 1x1 pixel under mouse
    VkRect2D scissor{};
    scissor.offset = {static_cast<int32_t>(mouseX), static_cast<int32_t>(mouseY)}; // Set scissor rect to mouse position
    scissor.extent = {1, 1}; // 1x1 pixel
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    _objectSelectionPipeline->bind(cmdBuffer);

    // Iterate selectable objects
    for (const auto& pair : _selectableObjects) {
        VkBuffer vertexBuffers[] = {pair.second->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(cmdBuffer, pair.second->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 1> descriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet()  // Per-frame descriptor set
            // Per-model descriptor set is not needed here beacuse we dont care about material when drawing ids.
            //_planetModels[m]->getDescriptorSet()->getDescriptorSet()  
        };
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _objectSelectionPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

        // Push constants for model
        PushConstants pushConstants{};
        pushConstants.model = pair.second->getModelMatrix();
        pushConstants.objectID = pair.second->getID();
        vkCmdPushConstants(cmdBuffer, _objectSelectionPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(pair.second->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmdBuffer);

    VulkanHelper::endSingleTimeCommands(_ctx, cmdBuffer);

    // Create a staging buffer to read the pixel data from the object selection image
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize imageSize = _swapChainExtent.width * _swapChainExtent.height * sizeof(uint32_t); // Assuming 1 bytes per pixel (uint32_t)
    VulkanHelper::createBuffer(_ctx, imageSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    // Copy the object selection image to the staging buffer for reading pixel data
    VulkanHelper::copyImageToBuffer(_ctx, _objectSelectionImage, stagingBuffer, _swapChainExtent.width, _swapChainExtent.height);

    // Map the memory and read the pixel data
    uint32_t* pixelData = new uint32_t[_swapChainExtent.width * _swapChainExtent.height];
    void* data;
    vkMapMemory(_ctx->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(pixelData, data, (size_t)imageSize);

    // Unmap and free the staging buffer
    vkUnmapMemory(_ctx->device, stagingBufferMemory);
    vkDestroyBuffer(_ctx->device, stagingBuffer, nullptr);
    vkFreeMemory(_ctx->device, stagingBufferMemory, nullptr);

    // Read mouseX and mouseY pixel data
    int mousePixel = static_cast<int>(mouseX) + static_cast<int>(mouseY) * _swapChainExtent.width;
    uint32_t selectedObjectID = pixelData[mousePixel]; // Assuming the ID is stored in the first channel
    // spdlog::info("Selected object ID: {}", selectedObjectID);

    // uint8_t* pixelData8 = new uint8_t[_swapChainExtent.width * _swapChainExtent.height];
    // for (size_t i = 0; i < _swapChainExtent.width * _swapChainExtent.height; ++i) {
    //     pixelData8[i] = static_cast<uint8_t>(pixelData[i]); // Convert to uint8_t if needed
    // }

    // // Save the pixel data to a file or process it as needed
    // // For example, you can save it to a PNG file using a library like stb_image_write
    // stbi_write_png("object_selection.png", _swapChainExtent.width, _swapChainExtent.height, 1, pixelData8, _swapChainExtent.width);

    // // Clean up
    // delete[] pixelData8;

    delete[] pixelData;
    return selectedObjectID; // Return the selected object ID
}

void Renderer::handleMouseClick(float mouseX, float mouseY) {
    // Call the querySelectionImage function to get the selected object ID
    uint32_t objectID = querySelectionImage(mouseX, mouseY);
    if (_currentTargetObjectID == objectID) return;

    // Check if _selectableObjects contains the objectID
    if (_selectableObjects.find(objectID) != _selectableObjects.end()) {
        // key exists
        _currentTargetObjectID = objectID; // Update the current target object ID
        _camera->setTargetAnimated(_selectableObjects[objectID]->getPosition()); // Set the camera target to the selected object
    }
}