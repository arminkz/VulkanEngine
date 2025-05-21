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

    // Create device meshes (GPU resources)
    std::shared_ptr<DeviceMesh> sphereDMesh = std::make_shared<DeviceMesh>(_ctx, sphere);
    std::shared_ptr<DeviceMesh> ringDMesh = std::make_shared<DeviceMesh>(_ctx, ring);
    std::shared_ptr<DeviceMesh> quadDMesh = std::make_shared<DeviceMesh>(_ctx, quad);

    //
