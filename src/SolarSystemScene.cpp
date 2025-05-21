#include "SolarSystemScene.h"
#include "geometry/MeshFactory.h"


SolarSystemScene::SolarSystemScene(std::shared_ptr<VulkanContext> ctx, Renderer& renderer)
    : Scene(std::move(ctx), renderer)
{
}


void SolarSystemScene::createPipelines()
{
    VkDescriptorSetLayout sceneDSL = _sceneDescriptorSets[0]->getDescriptorSetLayout();

    // Planet pipeline
    PipelineParams planetPipelineParams;
    planetPipelineParams.name = "PlanetPipeline";
    planetPipelineParams.descriptorSetLayouts = {sceneDSL, _planets[0]->getDescriptorSet()->getDescriptorSetLayout()};
    planetPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    planetPipelineParams.renderPass = _offscreenRenderPassMSAA;
    planetPipelineParams.msaaSamples = _renderer.getMSAASamples();
    _planetPipeline = std::make_unique<Pipeline>(_ctx, "spv/planet/planet_vert.spv", "spv/planet/planet_frag.spv", planetPipelineParams);

    // Orbit pipeline
    PipelineParams orbitPipelineParams;
    orbitPipelineParams.name = "OrbitPipeline";
    orbitPipelineParams.descriptorSetLayouts = {sceneDSL};
    orbitPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)}};
    orbitPipelineParams.renderPass = _offscreenRenderPassMSAA;
    orbitPipelineParams.msaaSamples = _renderer.getMSAASamples();
    orbitPipelineParams.depthTest = true;
    orbitPipelineParams.depthWrite = false;
    _orbitPipeline = std::make_unique<Pipeline>(_ctx, "spv/orbit/orbit_vert.spv", "spv/orbit/orbit_frag.spv", orbitPipelineParams);

    // GlowSphere pipeline
    PipelineParams glowSpherePipelineParams;
    glowSpherePipelineParams.name = "GlowSpherePipeline";
    glowSpherePipelineParams.descriptorSetLayouts = {sceneDSL, _glowSpheres[0]->getDescriptorSet()->getDescriptorSetLayout()};
    glowSpherePipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    glowSpherePipelineParams.renderPass = _offscreenRenderPassMSAA;
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
    skyBoxPipelineParams.renderPass = _offscreenRenderPassMSAA;
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
    earthPipelineParams.renderPass = _offscreenRenderPassMSAA;
    earthPipelineParams.msaaSamples = _renderer.getMSAASamples();
    _earthPipeline = std::make_unique<Pipeline>(_ctx, "spv/earth/earth_vert.spv", "spv/earth/earth_frag.spv", earthPipelineParams);

    // Sun pipeline
    PipelineParams sunPipelineParams;
    sunPipelineParams.name = "SunPipeline";
    sunPipelineParams.descriptorSetLayouts = {sceneDSL};
    sunPipelineParams.pushConstantRanges = {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4)}};
    sunPipelineParams.renderPass = _offscreenRenderPassMSAA;
    sunPipelineParams.msaaSamples = _renderer.getMSAASamples();
    _sunPipeline = std::make_unique<Pipeline>(_ctx, "spv/sun/sun_vert.spv", "spv/sun/sun_frag.spv", sunPipelineParams);

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
    _planets.push_back(_sun);

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
