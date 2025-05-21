#include "Orbit.h"


Orbit::Orbit(std::shared_ptr<VulkanContext> ctx, 
             std::string name, 
             std::shared_ptr<DeviceMesh> mesh)
    : Model(ctx, std::move(name), std::move(mesh))
{
}


Orbit::~Orbit()
{
    // Cleanup resources if needed
}


void Orbit::draw(VkCommandBuffer commandBuffer, const Scene& scene)
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