#include "SolarSystemScene.h"
#include "geometry/MeshFactory.h"
#include "TextureSampler.h"


SolarSystemScene::SolarSystemScene(std::shared_ptr<VulkanContext> ctx, Renderer& renderer)
    : Scene(std::move(ctx), renderer)
{
}


SolarSystemScene::~SolarSystemScene()
{
    // Wait for any unfinished GPU tasks
    vkDeviceWaitIdle(_ctx->device);

    _planetPipeline = nullptr;
    _orbitPipeline = nullptr;
    _glowSpherePipeline = nullptr;
    _skyBoxPipeline = nullptr;
    _sunPipeline = nullptr;
    _earthPipeline = nullptr;

    _renderPass = nullptr;
    _offscreenRenderPass = nullptr;
    _offscreenRenderPassMSAA = nullptr;
    _objectSelectionRenderPass = nullptr;
}


void SolarSystemScene::createPipelines()
{
    VkDescriptorSetLayout sceneDSL = _sceneDescriptorSets[0]->getDescriptorSetLayout();

    // Texture sampler for post-processing
    _ppTextureSampler = std::make_unique<TextureSampler>(_ctx, 1, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);


    // Glow pass pipeline
    PipelineParams glowPassPipelineParams;
    glowPassPipelineParams.name = "GlowPassPipeline";
    glowPassPipelineParams.descriptorSetLayouts = {sceneDSL};
    glowPassPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GlowPassPushConstants)}};
    glowPassPipelineParams.renderPass = _offscreenRenderPass->getRenderPass();
    _glowPipeline = std::make_unique<Pipeline>(_ctx, "spv/glow/glow_vert.spv", "spv/glow/glow_frag.spv", glowPassPipelineParams);

    VkDescriptorImageInfo glowPassOutputTexture{};
    glowPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    glowPassOutputTexture.imageView = _offscreenFrameBuffers[0]->getColorImageView();
    glowPassOutputTexture.sampler = _ppTextureSampler->getSampler();


    // Blur pass pipeline (vertical)
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
    blurVertPassOutputTexture.sampler = _ppTextureSampler->getSampler();

    PipelineParams blurPassPipelineParams {};
    blurPassPipelineParams.name = "BlurPassPipeline - Vertical";
    blurPassPipelineParams.vertexBindingDescription = std::nullopt;
    blurPassPipelineParams.vertexAttributeDescriptions = {};
    blurPassPipelineParams.cullMode = VK_CULL_MODE_NONE;
    blurPassPipelineParams.descriptorSetLayouts = { _blurVertDescriptorSet->getDescriptorSetLayout() }; 
    blurPassPipelineParams.pushConstantRanges = {};
    blurPassPipelineParams.renderPass = _offscreenRenderPass->getRenderPass();
    int blurDirection = 0; // 0 for vertical
    VkSpecializationMapEntry blurDirectionMapEntry = {0, 0, sizeof(int)};
    blurPassPipelineParams.fragmentShaderSpecializationInfo = VkSpecializationInfo {1, &blurDirectionMapEntry, sizeof(int), &blurDirection};
    _blurVertPipeline = std::make_unique<Pipeline>(_ctx, "spv/blur/blur_vert.spv", "spv/blur/blur_frag.spv", blurPassPipelineParams);


    // Blur pass pipeline (horizontal)
    std::vector<Descriptor> blurHorizDescriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _blurSettingsUBO->getDescriptorInfo()),
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, blurVertPassOutputTexture), // Blur vert texture
    };
    _blurHorizDescriptorSet = std::make_unique<DescriptorSet>(_ctx, blurHorizDescriptors);

    VkDescriptorImageInfo blurHorizPassOutputTexture{};
    blurHorizPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    blurHorizPassOutputTexture.imageView = _offscreenFrameBuffers[2]->getColorImageView();
    blurHorizPassOutputTexture.sampler = _ppTextureSampler->getSampler();

    blurPassPipelineParams.name = "BlurPassPipeline - Horizontal";
    blurDirection = 1;     // 1 for horizontal
    blurPassPipelineParams.renderPass = _offscreenRenderPass->getRenderPass();
    _blurHorizPipeline = std::make_unique<Pipeline>(_ctx, "spv/blur/blur_vert.spv", "spv/blur/blur_frag.spv", blurPassPipelineParams);


    // Composite pass pipeline
    VkDescriptorImageInfo normalPassOutputTexture{};
    normalPassOutputTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalPassOutputTexture.imageView = _offscreenFrameBuffers[3]->getResolveImageView();
    normalPassOutputTexture.sampler = _ppTextureSampler->getSampler();

    std::vector<Descriptor> compositeDescriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, blurHorizPassOutputTexture),   // Blur texture
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, normalPassOutputTexture),      // Normal scene texture
    };
    _compositeDescriptorSet = std::make_unique<DescriptorSet>(_ctx, compositeDescriptors);

    PipelineParams compositePipelineParams {};
    compositePipelineParams.name = "CompositePipeline";
    compositePipelineParams.vertexBindingDescription = std::nullopt; // This is a fullscreen triangle, so we don't need vertex binding description
    compositePipelineParams.vertexAttributeDescriptions = {};
    compositePipelineParams.cullMode = VK_CULL_MODE_NONE;
    compositePipelineParams.descriptorSetLayouts = { _compositeDescriptorSet->getDescriptorSetLayout() };
    compositePipelineParams.pushConstantRanges = {};
    compositePipelineParams.renderPass = _renderPass->getRenderPass();
    compositePipelineParams.msaaSamples = _renderer.getMSAASamples();
    _compositePipeline = std::make_unique<Pipeline>(_ctx, "spv/composite/composite_vert.spv", "spv/composite/composite_frag.spv", compositePipelineParams);

    
    // Planet pipeline
    PipelineParams planetPipelineParams;
    planetPipelineParams.name = "PlanetPipeline";
    planetPipelineParams.descriptorSetLayouts = {sceneDSL, _planets[0]->getDescriptorSet()->getDescriptorSetLayout()};
    planetPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    planetPipelineParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    planetPipelineParams.msaaSamples = _renderer.getMSAASamples();
    _planetPipeline = std::make_unique<Pipeline>(_ctx, "spv/planet/planet_vert.spv", "spv/planet/planet_frag.spv", planetPipelineParams);

    // Orbit pipeline
    PipelineParams orbitPipelineParams;
    orbitPipelineParams.name = "OrbitPipeline";
    orbitPipelineParams.descriptorSetLayouts = {sceneDSL};
    orbitPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)}};
    orbitPipelineParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    orbitPipelineParams.msaaSamples = _renderer.getMSAASamples();
    orbitPipelineParams.depthTest = true;
    orbitPipelineParams.depthWrite = false;
    _orbitPipeline = std::make_unique<Pipeline>(_ctx, "spv/orbit/orbit_vert.spv", "spv/orbit/orbit_frag.spv", orbitPipelineParams);

    // GlowSphere pipeline
    PipelineParams glowSpherePipelineParams;
    glowSpherePipelineParams.name = "GlowSpherePipeline";
    glowSpherePipelineParams.descriptorSetLayouts = {sceneDSL, _glowSpheres[0]->getDescriptorSet()->getDescriptorSetLayout()};
    glowSpherePipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    glowSpherePipelineParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    glowSpherePipelineParams.msaaSamples = _renderer.getMSAASamples();
    glowSpherePipelineParams.depthTest = true;
    glowSpherePipelineParams.depthWrite = false;
    glowSpherePipelineParams.frontFace = VK_FRONT_FACE_CLOCKWISE;
    _glowSpherePipeline = std::make_unique<Pipeline>(_ctx, "spv/glowsphere/glowsphere_vert.spv", "spv/glowsphere/glowsphere_frag.spv", glowSpherePipelineParams);

    // SkyBox pipeline
    PipelineParams skyBoxPipelineParams;
    skyBoxPipelineParams.name = "SkyBoxPipeline";
    skyBoxPipelineParams.descriptorSetLayouts = {sceneDSL, _skyBox->getDescriptorSet()->getDescriptorSetLayout()};
    skyBoxPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT , 0, sizeof(glm::mat4)}};
    skyBoxPipelineParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    skyBoxPipelineParams.msaaSamples = _renderer.getMSAASamples();
    skyBoxPipelineParams.depthTest = true;
    skyBoxPipelineParams.depthWrite = false;
    skyBoxPipelineParams.frontFace = VK_FRONT_FACE_CLOCKWISE;
    _skyBoxPipeline = std::make_unique<Pipeline>(_ctx, "spv/skybox/skybox_vert.spv", "spv/skybox/skybox_frag.spv", skyBoxPipelineParams);

    // Earth pipeline
    PipelineParams earthPipelineParams;
    earthPipelineParams.name = "EarthPipeline";
    earthPipelineParams.descriptorSetLayouts = {sceneDSL, _earth->getDescriptorSet()->getDescriptorSetLayout()};
    earthPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    earthPipelineParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    earthPipelineParams.msaaSamples = _renderer.getMSAASamples();
    _earthPipeline = std::make_unique<Pipeline>(_ctx, "spv/earth/earth_vert.spv", "spv/earth/earth_frag.spv", earthPipelineParams);

    // Sun pipeline
    PipelineParams sunPipelineParams;
    sunPipelineParams.name = "SunPipeline";
    sunPipelineParams.descriptorSetLayouts = {sceneDSL};
    sunPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    sunPipelineParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    sunPipelineParams.msaaSamples = _renderer.getMSAASamples();
    _sunPipeline = std::make_unique<Pipeline>(_ctx, "spv/sun/sun_vert.spv", "spv/sun/sun_frag.spv", sunPipelineParams);
}


void SolarSystemScene::connectPipelines()
{
    // Set the pipelines for all planets
    for (const auto& planet : _planets) {
        planet->setPipeline(_planetPipeline);
    }

    // Set the pipeline for orbits
    for (const auto& orbit : _orbits) {
        orbit->setPipeline(_orbitPipeline);
    }

    // Set the pipeline for glow spheres
    for (const auto& glowSphere : _glowSpheres) {
        glowSphere->setPipeline(_glowSpherePipeline);
    }

    // Earth and sun have their own pipelines
    _sun->setPipeline(_sunPipeline);
    _earth->setPipeline(_earthPipeline);

    // Set the pipeline for the skybox
    _skyBox->setPipeline(_skyBoxPipeline);
}


void SolarSystemScene::createModels()
{
    // Create host meshes
    HostMesh sphere = MeshFactory::createSphereMesh(1.f, 64, 64);
    HostMesh ring = MeshFactory::createAnnulusMesh(1.3f, 2.2f, 64);
    HostMesh quad =  MeshFactory::createQuadMesh(1.f, 1.f, true);
    HostMesh cube = MeshFactory::createCubeMesh(1.f, 1.f, 1.f);

    // Create device meshes (GPU resources)
    std::shared_ptr<DeviceMesh> sphereDMesh = std::make_shared<DeviceMesh>(_ctx, sphere);
    std::shared_ptr<DeviceMesh> ringDMesh = std::make_shared<DeviceMesh>(_ctx, ring);
    std::shared_ptr<DeviceMesh> quadDMesh = std::make_shared<DeviceMesh>(_ctx, quad);
    std::shared_ptr<DeviceMesh> cubeDMesh = std::make_shared<DeviceMesh>(_ctx, cube);

    // SkyBox
    //_skyBox = std::make_unique<SkyBox>(_ctx, "Skybox", cubeDMesh, cubemapTexture);

    // Sun
    _sun = std::make_shared<Sun>(_ctx, "Sun", sphereDMesh, sizeSun);

    // Mercury
    std::shared_ptr<Texture2D> mercuryColorTexture = std::make_shared<Texture2D>(_ctx, "textures/mercury/8k_mercury.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> mercury = std::make_shared<Planet>(_ctx, "Mercury", sphereDMesh, mercuryColorTexture, _sun, sizeMercury, orbitRadMercury);
    _planets.push_back(std::move(mercury));

    // Venus
    std::shared_ptr<Texture2D> venusColorTexture = std::make_shared<Texture2D>(_ctx, "textures/venus/4k_venus_atmosphere.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> venus = std::make_shared<Planet>(_ctx, "Venus", sphereDMesh, venusColorTexture, _sun, sizeVenus, orbitRadVenus);
    _planets.push_back(std::move(venus));

    // Earth
    std::shared_ptr<Texture2D> colorTexture = std::make_shared<Texture2D>(_ctx, "textures/earth/10k_earth_day.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Texture2D> unlitTexture = std::make_shared<Texture2D>(_ctx, "textures/earth/10k_earth_night.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Texture2D> normalTexture = std::make_shared<Texture2D>(_ctx, "textures/earth/2k_earth_normal.png", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<Texture2D> specularTexture = std::make_shared<Texture2D>(_ctx, "textures/earth/2k_earth_specular.jpeg", VK_FORMAT_R8G8B8A8_UNORM);
    std::shared_ptr<Texture2D> overlayTexture = std::make_shared<Texture2D>(_ctx, "textures/earth/8k_earth_clouds.png", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Earth> earth = std::make_shared<Earth>(_ctx, "Earth", sphereDMesh, colorTexture, unlitTexture, normalTexture, specularTexture, overlayTexture, _sun, sizeEarth, orbitRadEarth);
    _earth = earth;
    _planets.push_back(std::move(earth));

    // Mars
    std::shared_ptr<Texture2D> marsColorTexture = std::make_shared<Texture2D>(_ctx, "textures/mars/8k_mars.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> mars = std::make_shared<Planet>(_ctx, "Mars", sphereDMesh, marsColorTexture, _sun, sizeMars, orbitRadMars);
    _planets.push_back(std::move(mars));

    // Jupiter
    std::shared_ptr<Texture2D> jupiterColorTexture = std::make_shared<Texture2D>(_ctx, "textures/jupiter/4k_jupiter.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> jupiter = std::make_shared<Planet>(_ctx, "Jupiter", sphereDMesh, jupiterColorTexture, _sun, sizeJupiter, orbitRadJupiter);
    _planets.push_back(std::move(jupiter));

    // Saturn
    std::shared_ptr<Texture2D> saturnColorTexture = std::make_shared<Texture2D>(_ctx, "textures/saturn/8k_saturn.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> saturn = std::make_shared<Planet>(_ctx, "Saturn", sphereDMesh, saturnColorTexture, _sun, sizeSaturn, orbitRadSaturn);
    _planets.push_back(std::move(saturn));

    // Saturn Ring
    std::shared_ptr<Texture2D> ringTexture = std::make_shared<Texture2D>(_ctx, "textures/saturn/8k_saturn_ring_alpha.png", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> saturn_ring = std::make_shared<Planet>(_ctx, "SaturnRing", ringDMesh, ringTexture, _sun, sizeSaturnRing, orbitRadSaturn);
    _planets.push_back(std::move(saturn_ring));

    // Uranus
    std::shared_ptr<Texture2D> uranusColorTexture = std::make_shared<Texture2D>(_ctx, "textures/uranus/1k_uranus.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> uranus = std::make_shared<Planet>(_ctx, "Uranus", sphereDMesh, uranusColorTexture, _sun, sizeUranus, orbitRadUranus);
    _planets.push_back(std::move(uranus));

    // Neptune
    std::shared_ptr<Texture2D> neptuneColorTexture = std::make_shared<Texture2D>(_ctx, "textures/neptune/2k_neptune.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> neptune = std::make_shared<Planet>(_ctx, "Neptune", sphereDMesh, neptuneColorTexture, _sun, sizeNeptune, orbitRadNeptune);
    _planets.push_back(std::move(neptune));

    // Pluto
    std::shared_ptr<Texture2D> plutoColorTexture = std::make_shared<Texture2D>(_ctx, "textures/pluto/2k_pluto.jpg", VK_FORMAT_R8G8B8A8_SRGB);
    std::shared_ptr<Planet> pluto = std::make_shared<Planet>(_ctx, "Pluto", sphereDMesh, plutoColorTexture, _sun, sizePluto, orbitRadPluto);
    _planets.push_back(std::move(pluto));


    // Mercury Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "MercuryOrbit", quadDMesh, _sun, orbitRadMercury));

    // Venus Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "VenusOrbit", quadDMesh, _sun, orbitRadVenus));

    // Earth Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "EarthOrbit", quadDMesh, _sun, orbitRadEarth));

    // Mars Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "MarsOrbit", quadDMesh, _sun, orbitRadMars));

    // Jupiter Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "JupiterOrbit", quadDMesh, _sun, orbitRadJupiter));

    // Saturn Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "SaturnOrbit", quadDMesh, _sun, orbitRadSaturn));

    // Uranus Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "UranusOrbit", quadDMesh, _sun, orbitRadUranus));

    // Neptune Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "NeptuneOrbit", quadDMesh, _sun, orbitRadNeptune));

    // Pluto Orbit
    _orbits.push_back(std::make_unique<Orbit>(_ctx, "PlutoOrbit", quadDMesh, _sun, orbitRadPluto));
}


void SolarSystemScene::createRenderPasses() {
    
    RenderPassParams offscreenRenderPassParams;
    offscreenRenderPassParams.name = "Offscreen Renderpass - No MSAA";
    offscreenRenderPassParams.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    offscreenRenderPassParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    offscreenRenderPassParams.colorLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    offscreenRenderPassParams.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    offscreenRenderPassParams.useColor = true;
    offscreenRenderPassParams.useDepth = true;
    offscreenRenderPassParams.useResolve = false;
    offscreenRenderPassParams.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    _offscreenRenderPass = std::make_unique<RenderPass>(_ctx, offscreenRenderPassParams);

    offscreenRenderPassParams.name = "Offscreen Renderpass - MSAA";
    offscreenRenderPassParams.colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    offscreenRenderPassParams.resolveLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    offscreenRenderPassParams.useResolve = true;
    offscreenRenderPassParams.msaaSamples = _renderer.getMSAASamples();
    _offscreenRenderPassMSAA = std::make_unique<RenderPass>(_ctx, offscreenRenderPassParams);

    RenderPassParams screenRenderPassParams;
    screenRenderPassParams.name = "Onscreen Renderpass";
    screenRenderPassParams.colorFormat = _renderer.getSwapChain()->getSwapChainImageFormat();
    screenRenderPassParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    screenRenderPassParams.resolveFormat = _renderer.getSwapChain()->getSwapChainImageFormat();
    screenRenderPassParams.colorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    screenRenderPassParams.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    screenRenderPassParams.resolveLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    screenRenderPassParams.useColor = true;
    screenRenderPassParams.useDepth = true;
    screenRenderPassParams.useResolve = true;
    screenRenderPassParams.msaaSamples = _renderer.getMSAASamples();
    screenRenderPassParams.isMultiPass = false;
    _renderPass = std::make_unique<RenderPass>(_ctx, screenRenderPassParams);

    RenderPassParams objectSelectionRenderPassParams;
    objectSelectionRenderPassParams.name = "Object Selection Renderpass";
    objectSelectionRenderPassParams.colorFormat = VK_FORMAT_R32_UINT;
    objectSelectionRenderPassParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    objectSelectionRenderPassParams.colorLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    objectSelectionRenderPassParams.depthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    objectSelectionRenderPassParams.useColor = true;
    objectSelectionRenderPassParams.useDepth = true;
    objectSelectionRenderPassParams.useResolve = false;
    objectSelectionRenderPassParams.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    objectSelectionRenderPassParams.isMultiPass = false;
    _objectSelectionRenderPass = std::make_unique<RenderPass>(_ctx, objectSelectionRenderPassParams);

}


void SolarSystemScene::createFrameBuffers() {
    
    _offscreenFrameBuffers.resize(4); // 4 offscreen framebuffers (1 for glow, 1 for vertical blur, 1 for horizontal blur, 1 for normal rendering)

    // Offscreen Framebuffers (No MSAA)
    FrameBufferParams offscreenFrameBufferParams{};
    offscreenFrameBufferParams.extent.width = static_cast<int>(_renderer.getSwapChain()->getSwapChainExtent().width);
    offscreenFrameBufferParams.extent.height = static_cast<int>(_renderer.getSwapChain()->getSwapChainExtent().height);
    offscreenFrameBufferParams.renderPass = _offscreenRenderPass->getRenderPass();
    offscreenFrameBufferParams.hasColor = true;
    offscreenFrameBufferParams.hasDepth = true;
    offscreenFrameBufferParams.hasResolve = false;
    offscreenFrameBufferParams.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    offscreenFrameBufferParams.colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    offscreenFrameBufferParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    offscreenFrameBufferParams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    offscreenFrameBufferParams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    _offscreenFrameBuffers[0] = std::make_unique<FrameBuffer>(_ctx, offscreenFrameBufferParams); // Glow pass framebuffer
    _offscreenFrameBuffers[1] = std::make_unique<FrameBuffer>(_ctx, offscreenFrameBufferParams); // Vertical blur framebuffer
    _offscreenFrameBuffers[2] = std::make_unique<FrameBuffer>(_ctx, offscreenFrameBufferParams); // Horizontal blur framebuffer

    // Offscreen Framebuffer (MSAA)
    offscreenFrameBufferParams.renderPass = _offscreenRenderPassMSAA->getRenderPass();
    offscreenFrameBufferParams.hasResolve = true;
    offscreenFrameBufferParams.msaaSamples = _renderer.getMSAASamples();
    offscreenFrameBufferParams.resolveFormat = VK_FORMAT_R8G8B8A8_UNORM;
    offscreenFrameBufferParams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    offscreenFrameBufferParams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    offscreenFrameBufferParams.resolveUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    _offscreenFrameBuffers[3] = std::make_unique<FrameBuffer>(_ctx, offscreenFrameBufferParams); // Normal rendering framebuffer

    // Main Framebuffers
    FrameBufferParams frameBufferParams{};
    frameBufferParams.extent = _renderer.getSwapChain()->getSwapChainExtent();
    frameBufferParams.renderPass = _renderPass->getRenderPass();
    frameBufferParams.msaaSamples = _renderer.getMSAASamples();
    frameBufferParams.hasColor = true;
    frameBufferParams.hasDepth = true;
    frameBufferParams.hasResolve = true;
    frameBufferParams.colorFormat = _renderer.getSwapChain()->getSwapChainImageFormat();
    frameBufferParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    frameBufferParams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    frameBufferParams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    _mainFrameBuffers.resize(_renderer.getSwapChain()->getSwapChainImageViews().size());
    for (size_t i = 0; i < _renderer.getSwapChain()->getSwapChainImageViews().size(); i++) {
        frameBufferParams.resolveImageView = _renderer.getSwapChain()->getSwapChainImageViews()[i];
        _mainFrameBuffers[i] = std::make_unique<FrameBuffer>(_ctx, frameBufferParams);
    }

    // Object Selection Framebuffer
    FrameBufferParams objectSelectionFrameBufferParams{};
    objectSelectionFrameBufferParams.extent = _renderer.getSwapChain()->getSwapChainExtent();
    objectSelectionFrameBufferParams.renderPass = _objectSelectionRenderPass->getRenderPass();
    objectSelectionFrameBufferParams.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    objectSelectionFrameBufferParams.hasColor = true;
    objectSelectionFrameBufferParams.hasDepth = true;
    objectSelectionFrameBufferParams.hasResolve = false;
    objectSelectionFrameBufferParams.colorFormat = VK_FORMAT_R32_UINT;
    objectSelectionFrameBufferParams.depthFormat = VulkanHelper::findDepthFormat(_ctx);
    objectSelectionFrameBufferParams.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    objectSelectionFrameBufferParams.depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    _objectSelectionFrameBuffer = std::make_unique<FrameBuffer>(_ctx, objectSelectionFrameBufferParams);
}


void SolarSystemScene::update(uint32_t currentImage)
{
    Scene::update(currentImage);

    //planet displacment logic here
}


void SolarSystemScene::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // Clear color
    clearValues[1].depthStencil = { 1.0f, 0 };             // Clear depth value

    // Solar System Scene uses bloom effect

    /*
        First pass (Glow): Render glowing objects to offscreen framebuffer
    */
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _offscreenRenderPass->getRenderPass();
    renderPassBeginInfo.framebuffer = _offscreenFrameBuffers[0]->getFrameBuffer();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = _offscreenFrameBuffers[0]->getExtent();
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

    
}