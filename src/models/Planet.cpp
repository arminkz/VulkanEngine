#include "Planet.h"
#include "SolarSystemScene.h"


Planet::Planet(std::shared_ptr<VulkanContext> ctx, 
               std::string name, 
               std::shared_ptr<DeviceMesh> mesh, 
               std::shared_ptr<Texture2D> baseColorTexture,
               std::weak_ptr<Model> parent,
               float planetSize,
               float orbitRadius)

    : SelectableModel(ctx, std::move(name), std::move(mesh)),
    _baseColorTexture(std::move(baseColorTexture)),
    _parent(std::move(parent)),
    _size(planetSize),
    _orbitRadius(orbitRadius)
{
    std::vector<Descriptor> descriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _baseColorTexture->getDescriptorInfo()), // Base color texture
    };
    _descriptorSet = std::make_unique<DescriptorSet>(_ctx, descriptors);
}


Planet::~Planet()
{
    // Cleanup resources if needed
}


void Planet::calculateModelMatrix()
{
    // Calculate position based on parent's position and orbit radius
    glm::vec3 parentPosition = glm::vec3(0.0f);
    if (auto parent = _parent.lock()) {
        parentPosition = parent->getPosition();
    }

    // Calculate the new position based on the orbit radius and angle
    glm::vec3 newPosition = parentPosition + glm::vec3(_orbitRadius * cos(_orbitAngle), 0.0f, _orbitRadius * sin(_orbitAngle));

    // Update the model matrix
    _modelMatrix = glm::translate(glm::mat4(1.0f), newPosition) * glm::scale(glm::mat4(1.0f), glm::vec3(_size));
}


void Planet::draw(VkCommandBuffer commandBuffer, const Scene& scene)
{
    const SolarSystemScene* ssScene = dynamic_cast<const SolarSystemScene*>(&scene);

    auto pipeline = _pipeline.lock();
    if (!pipeline) {
        spdlog::error("Pipeline is not set for Planet model.");
        return;
    }

    // Bind the pipeline and descriptor set
    pipeline->bind(commandBuffer);

    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 2> descriptorSets = {
        ssScene->getSceneDescriptorSet()->getDescriptorSet(),        // Scene descriptor set
        _descriptorSet->getDescriptorSet()                        // My descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

    // Push constants for model
    vkCmdPushConstants(commandBuffer, pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &_modelMatrix);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}


void Planet::drawSelection(VkCommandBuffer commandBuffer, const Scene& scene)
{
    const SolarSystemScene* ssScene = dynamic_cast<const SolarSystemScene*>(&scene);

    auto selectionPipeline = _selectionPipeline.lock();
    if (!selectionPipeline) {
        spdlog::error("Selection pipeline is not set for Planet model.");
        return;
    }
    
    // Bind the pipeline and descriptor set
    selectionPipeline->bind(commandBuffer);

    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 1> descriptorSets = {
        ssScene->getSceneDescriptorSet()->getDescriptorSet(),
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, selectionPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

    // Push constants for selection
    struct PushConstants {
        glm::mat4 model;
        int objectID;
    } pushConstants{};

    pushConstants.model = _modelMatrix;
    pushConstants.objectID = getID();
    vkCmdPushConstants(commandBuffer, selectionPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}