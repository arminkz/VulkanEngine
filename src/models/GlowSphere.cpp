#include "GlowSphere.h"


GlowSphere::GlowSphere(std::shared_ptr<VulkanContext> ctx, 
                         std::string name, 
                         std::shared_ptr<DeviceMesh> mesh,
                         std::weak_ptr<Model> parent,
                         float planetSize)
    : Model(ctx, std::move(name), std::move(mesh))
{
}


GlowSphere::~GlowSphere()
{
    // Cleanup resources if needed
}


void GlowSphere::calculateModelMatrix()
{
    // GlowSphere has the same position as the parent planet
    glm::vec3 parentPosition = glm::vec3(0.0f);
    if (auto parent = _parent.lock()) {
        parentPosition = parent->getModelMatrix() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // Update the model matrix
    _modelMatrix = glm::translate(glm::mat4(1.0f), parentPosition) * glm::scale(glm::mat4(1.0f), glm::vec3(_size));
}


void GlowSphere::draw(VkCommandBuffer commandBuffer, const Scene& scene)
{
    // Bind the pipeline and descriptor set
    _pipeline->bind(commandBuffer);
    
    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 1> descriptorSets = {
        scene.getSceneDescriptorSet()->getDescriptorSet() // Scene descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1, descriptorSets.data(), 0, nullptr);

    // Push constants for model
    vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &_modelMatrix);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}