#include "Scene.h"

Scene::Scene(std::shared_ptr<VulkanContext> ctx)
    : _ctx(std::move(ctx))
{
    // Initialize scene information
    _sceneInfo.view = glm::mat4(1.0f);
    _sceneInfo.projection = glm::mat4(1.0f);
    _sceneInfo.time = 0.0f;
    _sceneInfo.cameraPosition = glm::vec3(0.0f);
    _sceneInfo.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    // Create UBO for scene information
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _sceneInfoUBOs[i] = std::make_unique<UniformBuffer<SceneInfo>>(_ctx);
        _sceneDescriptorSets[i] = std::make_unique<DescriptorSet>(_ctx, std::vector<Descriptor>{
            Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, _sceneInfoUBOs[i]->getDescriptorInfo())
        });
    }
}