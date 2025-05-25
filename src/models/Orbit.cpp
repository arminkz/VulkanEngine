#include "Orbit.h"
#include "SolarSystemScene.h"

Orbit::Orbit(std::shared_ptr<VulkanContext> ctx, 
             std::string name, 
             std::shared_ptr<DeviceMesh> mesh,
             std::weak_ptr<Model> parent,
             float orbitSize)
    : Model(ctx, std::move(name), std::move(mesh)), 
      _parent(std::move(parent)),
      _orbitSize(orbitSize)
{
}


Orbit::~Orbit()
{
    // Cleanup resources if needed
}


void Orbit::calculateModelMatrix()
{
    glm::vec3 parentPosition = glm::vec3(0.0f);
    if (auto parent = _parent.lock()) {
        parentPosition = parent->getPosition();
    }

    glm::mat4 rotation90 = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(_orbitSize * 2.0f));
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), parentPosition);

    // Update the model matrix
    _modelMatrix = translation * scale * rotation90;
}


void Orbit::draw(VkCommandBuffer commandBuffer, const Scene& scene)
{
    const SolarSystemScene* ssScene = dynamic_cast<const SolarSystemScene*>(&scene);

    auto pipeline = _pipeline.lock();
    if (!pipeline) {
        spdlog::error("Pipeline is not set for Orbit model.");
        return;
    }

    // Bind the pipeline and descriptor set
    pipeline->bind(commandBuffer);
    
    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 1> descriptorSets = {
        ssScene->getSceneDescriptorSet()->getDescriptorSet() // Scene descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

    // Push constants for model
    vkCmdPushConstants(commandBuffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &_modelMatrix);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}