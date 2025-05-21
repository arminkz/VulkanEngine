#include "Planet.h"


Planet::Planet(std::shared_ptr<VulkanContext> ctx, 
               std::string name, 
               std::shared_ptr<DeviceMesh> mesh, 
               std::shared_ptr<Texture2D> baseColorTexture,
               std::weak_ptr<Model> parent,
               float planetSize,
               float orbitRadius)

    : Model(ctx, std::move(name), std::move(mesh)),
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
        parentPosition = parent->getModelMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Calculate the new position based on the orbit radius and angle
    glm::vec3 newPosition = parentPosition + glm::vec3(_orbitRadius * cos(_orbitAngle), 0.0f, _orbitRadius * sin(_orbitAngle));

    // Update the model matrix
    _modelMatrix = glm::translate(glm::mat4(1.0f), newPosition) * glm::scale(glm::mat4(1.0f), glm::vec3(_size));
}


void Planet::draw(VkCommandBuffer commandBuffer, const Scene& scene)
{
    // Bind the pipeline and descriptor set
    _pipeline->bind(commandBuffer);
    
    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 2> descriptorSets = {
        scene.getSceneDescriptorSet()->getDescriptorSet(),        // Scene descriptor set
        _descriptorSet->getDescriptorSet()                        // My descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

    // Push constants for model
    vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &_modelMatrix);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}


void Planet::drawSelection(VkCommandBuffer commandBuffer, const Scene& scene)
{
    // Bind the pipeline and descriptor set
    _selectionPipeline->bind(commandBuffer);
    
    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 1> descriptorSets = {
        scene.getSceneDescriptorSet()->getDescriptorSet(),
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _selectionPipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

    // Push constants for selection
    struct PushConstants {
        glm::mat4 model;
        int objectID;
    } pushConstants{};

    pushConstants.model = _modelMatrix;
    pushConstants.objectID = getId();
    vkCmdPushConstants(commandBuffer, _selectionPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}