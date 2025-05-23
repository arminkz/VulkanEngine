#include "Scene.h"

Scene::Scene(std::shared_ptr<VulkanContext> ctx, Renderer& renderer)
    : _ctx(std::move(ctx)), _renderer(renderer)
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


Scene::~Scene()
{
}


void Scene::update(uint32_t currentImage)
{
    _currentFrame = currentImage;
    
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Update camera position based on time
    _camera->advanceAnimation(time - _sceneInfo.time);

    // Update SceneInfo
    _sceneInfo.view = _camera->getViewMatrix();

    VkExtent2D swapChainExtent = _renderer.getSwapChain()->getSwapChainExtent();
    _sceneInfo.projection = glm::perspective(glm::radians(45.f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 1000.f);
    _sceneInfo.projection[1][1] *= -1; // Invert Y axis for Vulkan

    _sceneInfo.time = time;
    
    _sceneInfo.cameraPosition = _camera->getPosition();

    // Update the scene information UBO
    _sceneInfoUBOs[_currentFrame]->update(_sceneInfo);
}