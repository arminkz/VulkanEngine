#include "Renderer.h"
#include "stdafx.h"
#include "VulkanHelper.h"

#include "Pipeline.h"
#include "GUI.h"
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

    // Clean up framebuffers
    for (size_t i = 0; i < _mainFrameBuffers.size(); i++) {
        _mainFrameBuffers[i] = nullptr;
    }

    _objectSelectionFrameBuffer = nullptr;

    // Clean up Vulkan resources
    for(size_t i=0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(_ctx->device, _imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(_ctx->device, _renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(_ctx->device, _inFlightFences[i], nullptr);
    }

    _gui = nullptr;
    _pipeline = nullptr;
    _orbitPipeline = nullptr;
    _sunPipeline = nullptr;
    _atmospherePipeline = nullptr;
    _objectSelectionPipeline = nullptr;

    vkDestroyRenderPass(_ctx->device, _renderPass, nullptr);
    vkDestroyRenderPass(_ctx->device, _offscreenRenderPass, nullptr);
    vkDestroyRenderPass(_ctx->device, _offscreenRenderPassMSAA, nullptr);
    vkDestroyRenderPass(_ctx->device, _objectSelectionRenderPass, nullptr);

    //Destroy dummy texture
    DeviceTexture::cleanupDummy();

    _ctx = nullptr;
    
}

bool Renderer::initialize()
{
    // Create swap chain
    _swapChain = std::make_unique<SwapChain>(_ctx);

    // MSAA
    _msaaSamples = VulkanHelper::getMaxMsaaSampleCount(_ctx);
    spdlog::info("Max MSAA samples: {}", static_cast<int>(_msaaSamples));

    // Create Texture Sampler
    _textureSampler = std::make_unique<TextureSampler>(_ctx, 10);
    _postprocessingTextureSampler = std::make_unique<TextureSampler>(_ctx, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

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
    float orbitRadVenus = 13.f;
    float orbitRadEarth = 16.f;
    //float orbitRadMoon = 163.f;
    float orbitRadMars = 22.f;
    float orbitRadJupiter = 47.f;
    float orbitRadSaturn = 77.f;
    float orbitRadUranus = 119.f;
    float orbitRadNeptune = 139.f;
    float orbitRadPluto = 155.f;

    float sizeSun = 3.f;
    float sizeMercury = 0.21f;
    float sizeVenus = 0.48f;
    float sizeEarth = 0.54f;
    //float sizeMoon = 0.0048f;
    float sizeMars = 0.36f;
    float sizeJupiter = 0.8f;
    float sizeSaturn = 0.7f;
    float sizeSaturnRing = 1.2f * sizeSaturn;
    float sizeUranus = 0.4f;
    float sizeNeptune = 0.4f;
    float sizePluto = 0.2f;

    // SkySphere
    std::shared_ptr<DeviceTexture> skyTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_stars_milky_way.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 skyModelMat = glm::mat4(1.f);
    skyModelMat = glm::scale(skyModelMat, glm::vec3(500.f));
    std::shared_ptr<DeviceModel> skySphere = std::make_shared<DeviceModel>("skySphere", _ctx, sphereInsideDMesh, skyModelMat, skyTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    skySphere->material.ambientStrength = 0.3f;
    skySphere->material.specularStrength = 0.0f;
    skySphere->updateMaterial();
    _skyBoxModel = skySphere;

    //Sun
    std::shared_ptr<DeviceTexture> sunTexture = std::make_shared<DeviceTexture>(_ctx, "textures/8k_sun.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> sunPerlinTexture = std::make_shared<DeviceTexture>(_ctx, "textures/perlin.png", VK_FORMAT_R8G8B8A8_UNORM);
    glm::mat4 sunModelMat = glm::mat4(1.f);
    sunModelMat = glm::scale(sunModelMat, glm::vec3(sizeSun));
    std::shared_ptr<DeviceModel> sun = std::make_shared<DeviceModel>("Sun", _ctx, sphereDMesh, sunModelMat, sunTexture, nullptr, nullptr, nullptr, sunPerlinTexture, _textureSampler);
    sun->material.sunShadeMode = 1;
    sun->updateMaterial();
    sun->glowColor = glm::vec3(1.f, 0.3f, 0.0f);
    _sunModel = sun;
    // Sun is selectable
    _selectableObjects[sun->getID()] = sun;
    // Sun is a light source
    _allModels.push_back(sun);

    //Sun atmosphere
    glm::mat4 sunAtmosphereModelMat = glm::mat4(1.f);
    sunAtmosphereModelMat = glm::scale(sunAtmosphereModelMat, glm::vec3(sizeSun * 1.3f));
    std::shared_ptr<AtmosphereModel> sunAtmosphere = std::make_shared<AtmosphereModel>(_ctx, sphereDMesh, sunAtmosphereModelMat, glm::vec3(1.f, 0.3f, 0.0f), 0.4f, 2.0f, true);
    _atmosphereModels.push_back(sunAtmosphere);
    //_sunGlowModel = sunAtmosphere;
    //_allModels.push_back(sunAtmosphere);

    // Mercury
    std::shared_ptr<DeviceTexture> mercuryColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/mercury/8k_mercury.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 mercuryModelMat = glm::mat4(1.f);
    mercuryModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadMercury, 0.f, 0.f));
    mercuryModelMat = glm::scale(mercuryModelMat, glm::vec3(sizeMercury));
    std::shared_ptr<DeviceModel> mercury = std::make_shared<DeviceModel>("Mercury", _ctx, sphereDMesh, mercuryModelMat, mercuryColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(mercury);
    // Mercury is selectable
    _selectableObjects[mercury->getID()] = mercury;
    _allModels.push_back(mercury);

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
    std::shared_ptr<DeviceModel> venus = std::make_shared<DeviceModel>("Venus", _ctx, sphereDMesh, venusModelMat, venusColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(venus);
    // Venus is selectable
    _selectableObjects[venus->getID()] = venus;
    _allModels.push_back(venus);

    // Venus Orbit
    glm::mat4 venusOrbitModelMat = glm::mat4(1.f);
    venusOrbitModelMat = glm::scale(venusOrbitModelMat, glm::vec3(orbitRadVenus * 2.f));
    venusOrbitModelMat = glm::rotate(venusOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("venus_orbit", _ctx, quadDMesh, venusOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Earth
    std::shared_ptr<DeviceTexture> colorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/10k_earth_day.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> unlitTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/10k_earth_night.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<DeviceTexture> normalTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/2k_earth_normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<DeviceTexture> specularTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/2k_earth_specular.jpeg", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<DeviceTexture> overlayTexture = std::make_shared<DeviceTexture>(_ctx, "textures/earth/8k_earth_clouds.png", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 earthModelMat = glm::mat4(1.f);
    earthModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadEarth, 0.f, 0.f));
    earthModelMat = glm::scale(earthModelMat, glm::vec3(sizeEarth));
    std::shared_ptr<DeviceModel> earth = std::make_shared<DeviceModel>("Earth", _ctx, sphereDMesh, earthModelMat, colorTexture, unlitTexture, normalTexture, specularTexture, overlayTexture, _textureSampler);
    _planetModels.push_back(earth);
    // Earth is selectable
    _selectableObjects[earth->getID()] = earth;
    _allModels.push_back(earth);

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

    // // Moon
    // std::shared_ptr<DeviceTexture> moonColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/moon/8k_moon.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    // glm::mat4 moonModelMat = glm::mat4(1.f);
    // moonModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadEarth + orbitRadMoon, 0.f, 0.f));
    // moonModelMat = glm::scale(moonModelMat, glm::vec3(sizeMoon));
    // std::shared_ptr<DeviceModel> moon = std::make_shared<DeviceModel>("Moon", _ctx, sphereDMesh, moonModelMat, moonColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    // _planetModels.push_back(moon);
    // // Moon is selectable
    // _selectableObjects[moon->getID()] = moon;
    // _allModels.push_back(moon);

    // // Moon Orbit
    // glm::mat4 moonOrbitModelMat = glm::mat4(1.f);
    // moonOrbitModelMat = glm::translate(moonOrbitModelMat, glm::vec3(sizeEarth, 0.f, 0.f));
    // moonOrbitModelMat = glm::scale(moonOrbitModelMat, glm::vec3(orbitRadMoon * 2.f));
    // moonOrbitModelMat = glm::rotate(moonOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    // _orbitModels.push_back(std::make_shared<DeviceModel>("moon_orbit", _ctx, quadDMesh, moonOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Mars
    std::shared_ptr<DeviceTexture> marsColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/mars/8k_mars.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 marsModelMat = glm::mat4(1.f);
    marsModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadMars, 0.f, 0.f));
    marsModelMat = glm::scale(marsModelMat, glm::vec3(sizeMars));
    std::shared_ptr<DeviceModel> mars = std::make_shared<DeviceModel>("Mars", _ctx, sphereDMesh, marsModelMat, marsColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(mars);
    // Mars is selectable
    _selectableObjects[mars->getID()] = mars;
    _allModels.push_back(mars);

    // Mars Orbit
    glm::mat4 marsOrbitModelMat = glm::mat4(1.f);
    marsOrbitModelMat = glm::scale(marsOrbitModelMat, glm::vec3(orbitRadMars * 2.f));
    marsOrbitModelMat = glm::rotate(marsOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("mars_orbit", _ctx, quadDMesh, marsOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Jupiter
    std::shared_ptr<DeviceTexture> jupiterColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/jupiter/4k_jupiter.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 jupiterModelMat = glm::mat4(1.f);
    jupiterModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadJupiter, 0.f, 0.f));
    jupiterModelMat = glm::scale(jupiterModelMat, glm::vec3(sizeJupiter));
    std::shared_ptr<DeviceModel> jupiter = std::make_shared<DeviceModel>("Jupiter", _ctx, sphereDMesh, jupiterModelMat, jupiterColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(jupiter);
    // Jupiter is selectable
    _selectableObjects[jupiter->getID()] = jupiter;
    _allModels.push_back(jupiter);

    // Jupiter Orbit
    glm::mat4 jupiterOrbitModelMat = glm::mat4(1.f);
    jupiterOrbitModelMat = glm::scale(jupiterOrbitModelMat, glm::vec3(orbitRadJupiter * 2.f));
    jupiterOrbitModelMat = glm::rotate(jupiterOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("jupiter_orbit", _ctx, quadDMesh, jupiterOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));

    // Saturn
    std::shared_ptr<DeviceTexture> saturnColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/saturn/8k_saturn.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 saturnModelMat = glm::mat4(1.f);
    saturnModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadSaturn, 0.f, 0.f));
    saturnModelMat = glm::scale(saturnModelMat, glm::vec3(sizeSaturn));
    std::shared_ptr<DeviceModel> saturn = std::make_shared<DeviceModel>("Saturn", _ctx, sphereDMesh, saturnModelMat, saturnColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(saturn);
    // Saturn is selectable
    _selectableObjects[saturn->getID()] = saturn;
    _allModels.push_back(saturn);

    // Saturn Ring
    std::shared_ptr<DeviceTexture> ringTexture = std::make_shared<DeviceTexture>(_ctx, "textures/saturn/8k_saturn_ring_alpha.png", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 ringModelMat = glm::mat4(1.f);
    ringModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadSaturn, 0.f, 0.f));
    ringModelMat = glm::scale(ringModelMat, glm::vec3(sizeSaturnRing));
    ringModelMat = glm::rotate(ringModelMat, glm::radians(107.f), glm::vec3(1.f, 0.f, 0.f));
    std::shared_ptr<DeviceModel> saturn_ring = std::make_shared<DeviceModel>("SaturnRing", _ctx, ringDMesh, ringModelMat, ringTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(saturn_ring);
    saturn_ring->material.ambientStrength = 0.3f;
    saturn_ring->updateMaterial();
    _allModels.push_back(saturn_ring);

    // Saturn Orbit
    glm::mat4 saturnOrbitModelMat = glm::mat4(1.f);
    saturnOrbitModelMat = glm::scale(saturnOrbitModelMat, glm::vec3(orbitRadSaturn * 2.f));
    saturnOrbitModelMat = glm::rotate(saturnOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("saturn_orbit", _ctx, quadDMesh, saturnOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));


    // Uranus
    std::shared_ptr<DeviceTexture> uranusColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/uranus/1k_uranus.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 uranusModelMat = glm::mat4(1.f);
    uranusModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadUranus, 0.f, 0.f));
    uranusModelMat = glm::scale(uranusModelMat, glm::vec3(sizeUranus));
    std::shared_ptr<DeviceModel> uranus = std::make_shared<DeviceModel>("Uranus", _ctx, sphereDMesh, uranusModelMat, uranusColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(uranus);
    // Uranus is selectable
    _selectableObjects[uranus->getID()] = uranus;
    _allModels.push_back(uranus);

    // Uranus Orbit
    glm::mat4 uranusOrbitModelMat = glm::mat4(1.f);
    uranusOrbitModelMat = glm::scale(uranusOrbitModelMat, glm::vec3(orbitRadUranus * 2.f));
    uranusOrbitModelMat = glm::rotate(uranusOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("uranus_orbit", _ctx, quadDMesh, uranusOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));


    // Neptune
    std::shared_ptr<DeviceTexture> neptuneColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/neptune/2k_neptune.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 neptuneModelMat = glm::mat4(1.f);
    neptuneModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadNeptune, 0.f, 0.f));
    neptuneModelMat = glm::scale(neptuneModelMat, glm::vec3(sizeNeptune));
    std::shared_ptr<DeviceModel> neptune = std::make_shared<DeviceModel>("Neptune", _ctx, sphereDMesh, neptuneModelMat, neptuneColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(neptune);
    // Neptune is selectable
    _selectableObjects[neptune->getID()] = neptune;
    _allModels.push_back(neptune);

    // Neptune Orbit
    glm::mat4 neptuneOrbitModelMat = glm::mat4(1.f);
    neptuneOrbitModelMat = glm::scale(neptuneOrbitModelMat, glm::vec3(orbitRadNeptune * 2.f));
    neptuneOrbitModelMat = glm::rotate(neptuneOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("neptune_orbit", _ctx, quadDMesh, neptuneOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));


    // Pluto
    std::shared_ptr<DeviceTexture> plutoColorTexture = std::make_shared<DeviceTexture>(_ctx, "textures/pluto/2k_pluto.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    glm::mat4 plutoModelMat = glm::mat4(1.f);
    plutoModelMat = glm::translate(glm::mat4(1.f), glm::vec3(orbitRadPluto, 0.f, 0.f));
    plutoModelMat = glm::scale(plutoModelMat, glm::vec3(sizePluto));
    std::shared_ptr<DeviceModel> pluto = std::make_shared<DeviceModel>("Pluto", _ctx, sphereDMesh, plutoModelMat, plutoColorTexture, nullptr, nullptr, nullptr, nullptr, _textureSampler);
    _planetModels.push_back(pluto);
    // Pluto is selectable
    _selectableObjects[pluto->getID()] = pluto;
    _allModels.push_back(pluto);

    // Pluto Orbit
    glm::mat4 plutoOrbitModelMat = glm::mat4(1.f);
    plutoOrbitModelMat = glm::scale(plutoOrbitModelMat, glm::vec3(orbitRadPluto * 2.f));
    plutoOrbitModelMat = glm::rotate(plutoOrbitModelMat, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    _orbitModels.push_back(std::make_shared<DeviceModel>("pluto_orbit", _ctx, quadDMesh, plutoOrbitModelMat, nullptr, nullptr, nullptr, nullptr, nullptr, _textureSampler));



    // Create Camera
    _currentTargetObjectID = 2; // Sun
    Camera::CameraParams cp{};
    cp.target = glm::vec3(0.f); //_selectableObjects[_currentTargetObjectID]->getPosition();
    _camera = std::make_unique<Camera>(cp);

    // Scene UBO (Per Frame)
    _sceneInfo.view = _camera->getViewMatrix();
    _sceneInfo.projection = glm::perspective(glm::radians(45.f), (float)_swapChain->getSwapChainExtent().width / (float)_swapChain->getSwapChainExtent().height, 0.1f, 1000.f);
    _sceneInfo.projection[1][1] *= -1; // Invert Y axis for Vulkan

    _sceneInfo.time = 0.f;
    _sceneInfo.cameraPosition = _camera->getPosition();
    _sceneInfo.lightColor = glm::vec3(1.f);
    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        _sceneInfoUBOs[i] = std::make_unique<UniformBuffer<SceneInfo>>(_ctx);
        _sceneInfoUBOs[i]->update(_sceneInfo);
    }
    
    // Create render pass
    createOffscreenRenderPasses();
    createMainRenderPass();
    createObjectSelectionRenderPass();

    // Create framebuffers
    createOffscreenFrameBuffers();
    createMainFrameBuffers();
    createObjectSelectionFrameBuffer();
    
    // Create ImGUI
    _gui = std::make_unique<GUI>(_ctx);
    _gui->init(_swapChain->getSwapChainExtent().width, _swapChain->getSwapChainExtent().height);
    _gui->initResources(_renderPass, _msaaSamples);



    // Create per-frame descriptor sets
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<Descriptor> perSceneDescriptors = {
            Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, _sceneInfoUBOs[i]->getDescriptorInfo()), // Scene UBO
        };
        _sceneDescriptorSets[i] = std::make_unique<DescriptorSet>(_ctx, perSceneDescriptors);
    }
    VkDescriptorSetLayout perFrameDSL = _sceneDescriptorSets[0]->getDescriptorSetLayout();


    // Pipeline for offscreen Glow Pass
    PipelineParams glowPassPipelineParams {};
    glowPassPipelineParams.name = "glowPassPipeline";
    glowPassPipelineParams.descriptorSetLayouts = { perFrameDSL };
    glowPassPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GlowPassPushConstants)}};
    glowPassPipelineParams.renderPass = _offscreenRenderPass;
    _glowPipeline = std::make_unique<Pipeline>(_ctx, "spv/glow/glow_vert.spv", "spv/glow/glow_frag.spv", glowPassPipelineParams);

    VkDescriptorImageInfo glowPassOutputTexture{};
    glowPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    glowPassOutputTexture.imageView = _offscreenFrameBuffers[0]->getColorImageView();
    glowPassOutputTexture.sampler = _postprocessingTextureSampler->getSampler();

    // Pipeline for offscreen Blur Pass (vertical)
    _blurSettingsUBO = std::make_unique<UniformBuffer<BlurSettings>>(_ctx);
    _blurSettingsUBO->update(blurSettings);

    std::vector<Descriptor> blurVertDescriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _blurSettingsUBO->getDescriptorInfo()),
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, glowPassOutputTexture), // Glow texture
    };
    _blurVertDescriptorSet = std::make_unique<DescriptorSet>(_ctx, blurVertDescriptors);

    VkDescriptorImageInfo blurVertPassOutputTexture{};
    blurVertPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    blurVertPassOutputTexture.imageView = _offscreenFrameBuffers[1]->getColorImageView();
    blurVertPassOutputTexture.sampler = _postprocessingTextureSampler->getSampler();

    PipelineParams blurPassPipelineParams {};
    blurPassPipelineParams.name = "blurPassPipeline - vertical";
    blurPassPipelineParams.vertexBindingDescription = std::nullopt; // This is a fullscreen triangle, so we don't need vertex binding description
    blurPassPipelineParams.vertexAttributeDescriptions = {};
    blurPassPipelineParams.cullMode = VK_CULL_MODE_NONE;
    blurPassPipelineParams.descriptorSetLayouts = { _blurVertDescriptorSet->getDescriptorSetLayout() }; 
    blurPassPipelineParams.pushConstantRanges = {};
    blurPassPipelineParams.renderPass = _offscreenRenderPass;
    int blurDirection = 0; // 0 for vertical
    VkSpecializationMapEntry blurDirectionMapEntry = {0, 0, sizeof(int)};
    blurPassPipelineParams.fragmentShaderSpecializationInfo = VkSpecializationInfo {1, &blurDirectionMapEntry, sizeof(int), &blurDirection};
    _blurVertPipeline = std::make_unique<Pipeline>(_ctx, "spv/blur/blur_vert.spv", "spv/blur/blur_frag.spv", blurPassPipelineParams);


    // Pipeline for offscreen Blur Pass (horizontal)
    std::vector<Descriptor> blurHorizDescriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _blurSettingsUBO->getDescriptorInfo()),
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, blurVertPassOutputTexture), // Blur vert texture
    };
    _blurHorizDescriptorSet = std::make_unique<DescriptorSet>(_ctx, blurHorizDescriptors);

    VkDescriptorImageInfo blurHorizPassOutputTexture{};
    blurHorizPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    blurHorizPassOutputTexture.imageView = _offscreenFrameBuffers[2]->getColorImageView();
    blurHorizPassOutputTexture.sampler = _postprocessingTextureSampler->getSampler();

    blurPassPipelineParams.name = "blurPassPipeline - horizontal";
    blurDirection = 1;     // 1 for horizontal
    blurPassPipelineParams.renderPass = _offscreenRenderPass;
    _blurHorizPipeline = std::make_unique<Pipeline>(_ctx, "spv/blur/blur_vert.spv", "spv/blur/blur_frag.spv", blurPassPipelineParams);


    // Pipeline for composite pass (Combines the blurred texture with the main scene)
    VkDescriptorImageInfo normalPassOutputTexture{};
    normalPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalPassOutputTexture.imageView = _offscreenFrameBuffers[3]->getResolveImageView();
    normalPassOutputTexture.sampler = _postprocessingTextureSampler->getSampler();

    std::vector<Descriptor> compositeDescriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, blurHorizPassOutputTexture),   // Blur texture
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, normalPassOutputTexture),      // Normal scene texture
    };
    _compositeDescriptorSet = std::make_unique<DescriptorSet>(_ctx, compositeDescriptors);

    PipelineParams compositePipelineParams {};
    compositePipelineParams.name = "compositePipeline";
    compositePipelineParams.vertexBindingDescription = std::nullopt; // This is a fullscreen triangle, so we don't need vertex binding description
    compositePipelineParams.vertexAttributeDescriptions = {};
    compositePipelineParams.cullMode = VK_CULL_MODE_NONE;
    compositePipelineParams.descriptorSetLayouts = { _compositeDescriptorSet->getDescriptorSetLayout() };
    compositePipelineParams.pushConstantRanges = {};
    compositePipelineParams.renderPass = _renderPass;
    compositePipelineParams.msaaSamples = _msaaSamples;
    _compositePipeline = std::make_unique<Pipeline>(_ctx, "spv/composite/composite_vert.spv", "spv/composite/composite_frag.spv", compositePipelineParams);


    VkDescriptorSetLayout perModelDSL = _planetModels[0]->getDescriptorSet()->getDescriptorSetLayout();
    VkDescriptorSetLayout atmosphereDSL = _atmosphereModels[0]->getDescriptorSet()->getDescriptorSetLayout();

    // Pipeline for general planets
    PipelineParams pipelineParams {};
    pipelineParams.name = "planetPipeline";
    pipelineParams.descriptorSetLayouts = { perFrameDSL, perModelDSL };
    pipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}};
    pipelineParams.renderPass = _offscreenRenderPassMSAA;
    pipelineParams.msaaSamples = _msaaSamples;
    _pipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/shader_frag.spv", pipelineParams);

    // Pipeline for sun
    PipelineParams sunPipelineParams {};
    sunPipelineParams.name = "sunPipeline";
    sunPipelineParams.descriptorSetLayouts = { perFrameDSL, perModelDSL };
    sunPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}};
    sunPipelineParams.renderPass = _offscreenRenderPassMSAA;
    sunPipelineParams.msaaSamples = _msaaSamples;
    _sunPipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/sun_frag.spv", sunPipelineParams);

    // Pipeline for orbits
    PipelineParams orbitPipelineParams {};
    orbitPipelineParams.name = "orbitPipeline";
    orbitPipelineParams.descriptorSetLayouts = { perFrameDSL, perModelDSL };
    orbitPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants)}};
    orbitPipelineParams.renderPass = _offscreenRenderPassMSAA;
    orbitPipelineParams.msaaSamples = _msaaSamples;
    orbitPipelineParams.depthTest = true;
    orbitPipelineParams.depthWrite = false;
    _orbitPipeline = std::make_unique<Pipeline>(_ctx, "spv/orbit_vert.spv", "spv/orbit_frag.spv", orbitPipelineParams);

    // Pipeline for Glow/Atmosphere
    PipelineParams atmospherePipelineParams {};
    atmospherePipelineParams.name = "atmospherePipeline";
    atmospherePipelineParams.descriptorSetLayouts = { perFrameDSL, atmosphereDSL };
    atmospherePipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}};
    atmospherePipelineParams.renderPass = _offscreenRenderPassMSAA;
    atmospherePipelineParams.msaaSamples = _msaaSamples;
    atmospherePipelineParams.depthTest = true;
    atmospherePipelineParams.depthWrite = false;
    atmospherePipelineParams.frontFace = VK_FRONT_FACE_CLOCKWISE; // For atmosphere, we need to reverse the front face
    _atmospherePipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/atmosphere_frag.spv", atmospherePipelineParams);

    // Pipeline for object selection
    PipelineParams objectSelectionPipelineParams {};
    objectSelectionPipelineParams.name = "objectSelectionPipeline";
    objectSelectionPipelineParams.descriptorSetLayouts = { perFrameDSL, perModelDSL };
    objectSelectionPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}};
    objectSelectionPipelineParams.renderPass = _objectSelectionRenderPass;
    objectSelectionPipelineParams.blendEnable = false;
    _objectSelectionPipeline = std::make_unique<Pipeline>(_ctx, "spv/shader_vert.spv", "spv/objectselect_frag.spv", objectSelectionPipelineParams);


    // Create command buffers
    if (!createCommandBuffers()) return false;

    // Create sync objects
    if (!createSyncObjects()) return false;
}


void Renderer::invalidate()
{
    vkDeviceWaitIdle(_ctx->device);

    recreateMainRenderResources();
    recreateObjectSelectionResources();
    _gui->init(_swapChain->getSwapChainExtent().width, _swapChain->getSwapChainExtent().height);
}


void Renderer::createOffscreenRenderPasses()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VulkanHelper::findDepthFormat(_ctx);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment};

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pDepthStencilAttachment = &depthReference; // Attach depth attachment

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(_ctx->device, &renderPassInfo, nullptr, &_offscreenRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create (Offscreen Non-MSAA) render pass!");
    }
    spdlog::info("Render pass created successfully. (Offscreen Non-MSAA)");


    // MSAA Render Pass
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    colorAttachment.samples = _msaaSamples;
    depthAttachment.samples = _msaaSamples;
    std::array<VkAttachmentDescription, 3> msaaAttachments = { colorAttachment, depthAttachment, resolveAttachment};

    VkAttachmentReference resolveReference = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription msaaSubpass{};
    msaaSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    msaaSubpass.colorAttachmentCount = 1;
    msaaSubpass.pColorAttachments = &colorReference;
    msaaSubpass.pDepthStencilAttachment = &depthReference; // Attach depth attachment
    msaaSubpass.pResolveAttachments = &resolveReference;   // Attach resolve attachment

    VkRenderPassCreateInfo msaaRenderPassInfo{};
    msaaRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    msaaRenderPassInfo.attachmentCount = static_cast<uint32_t>(msaaAttachments.size());
    msaaRenderPassInfo.pAttachments = msaaAttachments.data();
    msaaRenderPassInfo.subpassCount = 1;
    msaaRenderPassInfo.pSubpasses = &msaaSubpass;
    msaaRenderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    msaaRenderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(_ctx->device, &msaaRenderPassInfo, nullptr, &_offscreenRenderPassMSAA) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create (Offscreen MSAA) render pass!");
    }
    spdlog::info("Render pass created successfully. (Offscreen MSAA)");

}

void Renderer::createOffscreenFrameBuffers() 
{
    _offscreenFrameBuffers.resize(4);

    // 1 offscreen framebuffer for glow - 1 for vertical blur - 1 for horizontal blur (with 1/2 resolution)
    FrameBufferParams ofbparams{};
    ofbparams.extent.width = static_cast<int>(_swapChain->getSwapChainExtent().width);
    ofbparams.extent.height = static_cast<int>(_swapChain->getSwapChainExtent().height);
    ofbparams.renderPass = _offscreenRenderPass;
    ofbparams.hasColor = true;
    ofbparams.hasDepth = true;
    ofbparams.hasResolve = false;
    ofbparams.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    ofbparams.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    ofbparams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    ofbparams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ofbparams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    _offscreenFrameBuffers[0] = std::make_unique<FrameBuffer>(_ctx, ofbparams); // Glow pass framebuffer
    _offscreenFrameBuffers[1] = std::make_unique<FrameBuffer>(_ctx, ofbparams); // Vertical blur framebuffer
    _offscreenFrameBuffers[2] = std::make_unique<FrameBuffer>(_ctx, ofbparams); // Horizontal blur framebuffer

    // 1 offscreen framebuffer for normal rendering (with full resolution)
    ofbparams.extent.width = _swapChain->getSwapChainExtent().width;
    ofbparams.extent.height = _swapChain->getSwapChainExtent().height;
    ofbparams.renderPass = _offscreenRenderPassMSAA;
    ofbparams.hasResolve = true;
    ofbparams.msaaSamples = _msaaSamples;
    ofbparams.resolveFormat = VK_FORMAT_R8G8B8A8_UNORM;
    ofbparams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    ofbparams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ofbparams.resolveUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    _offscreenFrameBuffers[3] = std::make_unique<FrameBuffer>(_ctx, ofbparams); // Normal rendering framebuffer
}

void Renderer::createMainRenderPass() {
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChain->getSwapChainImageFormat();
    colorAttachment.samples = _msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = _swapChain->getSwapChainImageFormat();
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VulkanHelper::findDepthFormat(_ctx);
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
    spdlog::info("Render pass created successfully. (Main Render Pass)");
}

void Renderer::createMainFrameBuffers(){
    FrameBufferParams framebufferParams{};
    framebufferParams.extent = _swapChain->getSwapChainExtent();
    framebufferParams.renderPass = _renderPass;
    framebufferParams.msaaSamples = _msaaSamples;
    framebufferParams.hasColor = true;
    framebufferParams.hasDepth = true;
    framebufferParams.hasResolve = true;
    framebufferParams.colorFormat = _swapChain->getSwapChainImageFormat();
    framebufferParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    framebufferParams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    framebufferParams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    _mainFrameBuffers.resize(_swapChain->getSwapChainImageViews().size());
    for (size_t i = 0; i < _swapChain->getSwapChainImageViews().size(); i++) {
        framebufferParams.resolveImageView = _swapChain->getSwapChainImageViews()[i];
        _mainFrameBuffers[i] = std::make_unique<FrameBuffer>(_ctx, framebufferParams);
    }
}

void Renderer::recreateMainRenderResources()
{
    // Clean up framebuffers
    for (size_t i = 0; i < _mainFrameBuffers.size(); i++) {
        _mainFrameBuffers[i] = nullptr;
    }

    // Clean up Swapchain
    _swapChain->cleanupSwapChain();

    // Create new swapchain
    _swapChain->createSwapChain();

    // Create new framebuffers
    createMainFrameBuffers();
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
    depthAttachment.format = VulkanHelper::findDepthFormat(_ctx); // same as usual
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

void Renderer::createObjectSelectionFrameBuffer()
{
    FrameBufferParams framebufferParams{};
    framebufferParams.extent = _swapChain->getSwapChainExtent();
    framebufferParams.renderPass = _objectSelectionRenderPass;
    framebufferParams.msaaSamples = VK_SAMPLE_COUNT_1_BIT; // No multisampling for ID attachment
    framebufferParams.hasColor = true;
    framebufferParams.hasDepth = true;
    framebufferParams.hasResolve = false;               // No resolve attachment for ID attachment
    framebufferParams.colorFormat = VK_FORMAT_R32_UINT; // Use a format that can store uint IDs
    framebufferParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    framebufferParams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Color attachment and transfer source
    framebufferParams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    _objectSelectionFrameBuffer = std::make_unique<FrameBuffer>(_ctx, framebufferParams);
}

void Renderer::recreateObjectSelectionResources()
{
    _objectSelectionFrameBuffer = nullptr;
    createObjectSelectionFrameBuffer();
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
    _sceneInfo.projection = glm::perspective(glm::radians(45.f), (float)_swapChain->getSwapChainExtent().width / (float)_swapChain->getSwapChainExtent().height, 0.1f, 1000.f);
    _sceneInfo.projection[1][1] *= -1; // Invert Y axis for Vulkan
    _sceneInfo.time = time;
    _sceneInfo.cameraPosition = _camera->getPosition();
    _sceneInfoUBOs[currentImage]->update(_sceneInfo);
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


void Renderer::recordBloomCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    // Begin Command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        spdlog::error("Failed to begin recording command buffer!");
        return;
    }

    /*
        First pass (Glow): Render glowing objects to offscreen framebuffer
    */
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _offscreenRenderPass;
    renderPassBeginInfo.framebuffer = _offscreenFrameBuffers[0]->getFrameBuffer();
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = _offscreenFrameBuffers[0]->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // Clear color
    clearValues[1].depthStencil = { 1.0f, 0 };             // Clear depth value
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport offscreenViewport{};
    offscreenViewport.x = 0.0f;
    offscreenViewport.y = 0.0f;
    offscreenViewport.width = _offscreenFrameBuffers[0]->getExtent().width;
    offscreenViewport.height = _offscreenFrameBuffers[0]->getExtent().height;
    offscreenViewport.minDepth = 0.0f;
    offscreenViewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &offscreenViewport);

    VkRect2D offscreenScissor{};
    offscreenScissor.offset = {0, 0};
    offscreenScissor.extent = _offscreenFrameBuffers[0]->getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &offscreenScissor);

    _glowPipeline->bind(commandBuffer);
    for(int m=0; m<static_cast<int>(_allModels.size()); m++) {
        VkBuffer vertexBuffers[] = {_allModels[m]->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _allModels[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 1> descriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet()  // Per-frame descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _glowPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

        // Push constants for model
        GlowPassPushConstants pushConstants{};
        pushConstants.model = _allModels[m]->getModelMatrix();
        pushConstants.glowColor = _allModels[m]->glowColor;
        vkCmdPushConstants(commandBuffer, _glowPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GlowPassPushConstants), &pushConstants);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_allModels[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    /*
        Second pass (Vertical Blur): Apply vertical blur to the glow pass output
    */
    renderPassBeginInfo.framebuffer = _offscreenFrameBuffers[1]->getFrameBuffer();
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    _blurVertPipeline->bind(commandBuffer);
    std::array<VkDescriptorSet, 1> blurVertDescriptorSets = {
        _blurVertDescriptorSet->getDescriptorSet()                 // Blur descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _blurVertPipeline->getPipelineLayout(), 0, 1, blurVertDescriptorSets.data(), 0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Draw a full-screen triangle for the blur pass
    vkCmdEndRenderPass(commandBuffer);


    /*
        Third pass (Horizontal Blur): Apply horizontal blur to the vertical blur output
    */
    renderPassBeginInfo.framebuffer = _offscreenFrameBuffers[2]->getFrameBuffer();
    renderPassBeginInfo.renderArea.extent = _offscreenFrameBuffers[2]->getExtent();
    offscreenViewport.width = _offscreenFrameBuffers[2]->getExtent().width;
    offscreenViewport.height = _offscreenFrameBuffers[2]->getExtent().height;
    vkCmdSetViewport(commandBuffer, 0, 1, &offscreenViewport);
    offscreenScissor.extent = _offscreenFrameBuffers[2]->getExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &offscreenScissor);
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    _blurHorizPipeline->bind(commandBuffer);
    std::array<VkDescriptorSet, 1> blurHorizDescriptorSets = {
        _blurHorizDescriptorSet->getDescriptorSet()                 // Blur descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _blurHorizPipeline->getPipelineLayout(), 0, 1, blurHorizDescriptorSets.data(), 0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Draw a full-screen triangle for the blur pass
    vkCmdEndRenderPass(commandBuffer);


    /*
        Fourth pass (Normal Shading)
    */
    VkRenderPassBeginInfo mainRenderPassBeginInfo{};
    mainRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    mainRenderPassBeginInfo.renderPass = _offscreenRenderPassMSAA;
    mainRenderPassBeginInfo.framebuffer = _offscreenFrameBuffers[3]->getFrameBuffer();
    mainRenderPassBeginInfo.renderArea.offset = { 0, 0 };
    mainRenderPassBeginInfo.renderArea.extent = _swapChain->getSwapChainExtent();
    mainRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    mainRenderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &mainRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swapChain->getSwapChainExtent().width;
    viewport.height = (float) _swapChain->getSwapChainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChain->getSwapChainExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    // Draw skybox
    {
        _pipeline->bind(commandBuffer);

        VkBuffer vertexBuffers[] = {_skyBoxModel->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _skyBoxModel->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 2> skyboxDescriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet(),  // Per-frame descriptor set
            _skyBoxModel->getDescriptorSet()->getDescriptorSet()       // Per-model descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 2, skyboxDescriptorSets.data(), 0, nullptr);

        // Push constants for model
        PushConstants pushConstants{};
        pushConstants.model = _skyBoxModel->getModelMatrix();
        pushConstants.objectID = _skyBoxModel->getID();
        vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

        // Draw
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_skyBoxModel->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

    // Draw sun model (TODO: this can be indside the Model class .draw() method)
    {
        // Bind pipeline
        _sunPipeline->bind(commandBuffer);

        // Bind vertex and index buffers
        VkBuffer vertexBuffers[] = {_sunModel->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _sunModel->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        // Bind descriptor sets
        std::array<VkDescriptorSet, 2> sunDescriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet(),  // Per-frame descriptor set
            _sunModel->getDescriptorSet()->getDescriptorSet()          // Per-model descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _sunPipeline->getPipelineLayout(), 0, 2, sunDescriptorSets.data(), 0, nullptr);

        // Push constants for model
        PushConstants pushConstants{};
        pushConstants.model = _sunModel->getModelMatrix();
        pushConstants.objectID = _sunModel->getID();
        vkCmdPushConstants(commandBuffer, _sunPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

        // Draw
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_sunModel->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

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


    // Draw orbit models
    _orbitPipeline->bind(commandBuffer);

    for(int m=0; m<static_cast<int>(_orbitModels.size()); m++) {
        VkBuffer vertexBuffers[] = {_orbitModels[m]->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _orbitModels[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 1> descriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet() // Per-frame descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _orbitPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
        vkCmdPushConstants(commandBuffer, _orbitPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &_orbitModels[m]->getModelMatrix());
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_orbitModels[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }
    

    vkCmdEndRenderPass(commandBuffer);

    /*
        Fifth pass (Composite): Combine the normal rendering with the glow pass
    */
    VkRenderPassBeginInfo compositeRenderPassBeginInfo{};
    compositeRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    compositeRenderPassBeginInfo.renderPass = _renderPass;
    compositeRenderPassBeginInfo.framebuffer = _mainFrameBuffers[imageIndex]->getFrameBuffer();
    compositeRenderPassBeginInfo.renderArea.offset = { 0, 0 };
    compositeRenderPassBeginInfo.renderArea.extent = _swapChain->getSwapChainExtent();
    compositeRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    compositeRenderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &compositeRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Bind the composite pipeline
    _compositePipeline->bind(commandBuffer);

    // Bind the descriptor set for the composite pass
    std::array<VkDescriptorSet, 1> compositeDescriptorSets = {
        _compositeDescriptorSet->getDescriptorSet() // Composite descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compositePipeline->getPipelineLayout(), 0, 1, compositeDescriptorSets.data(), 0, nullptr);

    // Draw a full-screen quad for the composite pass
    vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Draw a full-screen triangle for the composite pass
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        spdlog::error("Failed to record command buffer!");
        return;
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
    renderPassBeginInfo.framebuffer = _mainFrameBuffers[imageIndex]->getFrameBuffer();
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChain->getSwapChainExtent();

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
    viewport.width = (float) _swapChain->getSwapChainExtent().width;
    viewport.height = (float) _swapChain->getSwapChainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChain->getSwapChainExtent();
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
    _orbitPipeline->bind(commandBuffer);

    for(int m=0; m<static_cast<int>(_orbitModels.size()); m++) {
        VkBuffer vertexBuffers[] = {_orbitModels[m]->getDeviceMesh()->getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // We can have multiple vertex buffers
        vkCmdBindIndexBuffer(commandBuffer, _orbitModels[m]->getDeviceMesh()->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // We can only use one index buffer at a time

        std::array<VkDescriptorSet, 1> descriptorSets = {
            _sceneDescriptorSets[_currentFrame]->getDescriptorSet() // Per-frame descriptor set
        };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _orbitPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);
        vkCmdPushConstants(commandBuffer, _orbitPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &_orbitModels[m]->getModelMatrix());
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_orbitModels[m]->getDeviceMesh()->getIndicesCount()), 1, 0, 0, 0);
    }

    // Draw ImGui frame
    _gui->drawFrame(commandBuffer, _currentFrame);
    
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
    VkResult result = vkAcquireNextImageKHR(_ctx->device, _swapChain->getSwapChain(), UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        invalidate();
        return;
    } else if (result != VK_SUCCESS) {
        spdlog::error("Failed to acquire swap chain image!");
        return;
    }

    // Reset the fence before drawing
    vkResetFences(_ctx->device, 1, &_inFlightFences[_currentFrame]);
    vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

    // Update ImGui frame
    _gui->newFrame();

    ImGui::SetNextWindowPos(ImVec2(30, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_Always);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
    ImGui::Begin("General", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // FPS
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImGui::End();

    // GUI elements can be added here
    //_selectableObjects[_currentTargetObjectID]->drawGUI();

    _gui->endFrame();
    _gui->updateBuffers(_currentFrame);

    // Update uniform buffer
    updateUniformBuffer(_currentFrame);

    recordBloomCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

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

    VkSwapchainKHR swapChains[] = {_swapChain->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(_ctx->presentQueue, &presentInfo);
    
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        invalidate();
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
    renderPassBeginInfo.framebuffer = _objectSelectionFrameBuffer->getFrameBuffer();
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = _swapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color.uint32[0] = 0;                    // Clear color (ID attachment)
    clearValues[1].depthStencil = { 1.0f, 0 };             // Clear depth value
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) _swapChain->getSwapChainExtent().width;
    viewport.height = (float) _swapChain->getSwapChainExtent().height;
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
    VkDeviceSize imageSize = _swapChain->getSwapChainExtent().width * _swapChain->getSwapChainExtent().height * sizeof(uint32_t); // Assuming 1 bytes per pixel (uint32_t)
    VulkanHelper::createBuffer(_ctx, imageSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

    // Copy the object selection image to the staging buffer for reading pixel data
    VulkanHelper::copyImageToBuffer(_ctx, _objectSelectionFrameBuffer->getColorImage(), stagingBuffer, _swapChain->getSwapChainExtent().width, _swapChain->getSwapChainExtent().height);

    // Map the memory and read the pixel data
    uint32_t* pixelData = new uint32_t[_swapChain->getSwapChainExtent().width * _swapChain->getSwapChainExtent().height];
    void* data;
    vkMapMemory(_ctx->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(pixelData, data, (size_t)imageSize);

    // Unmap and free the staging buffer
    vkUnmapMemory(_ctx->device, stagingBufferMemory);
    vkDestroyBuffer(_ctx->device, stagingBuffer, nullptr);
    vkFreeMemory(_ctx->device, stagingBufferMemory, nullptr);

    // Read mouseX and mouseY pixel data
    int mousePixel = static_cast<int>(mouseX) + static_cast<int>(mouseY) * _swapChain->getSwapChainExtent().width;
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