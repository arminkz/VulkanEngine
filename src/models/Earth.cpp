#include "Earth.h"

Earth::Earth(std::shared_ptr<VulkanContext> ctx, 
             std::string name, 
             std::shared_ptr<DeviceMesh> mesh, 
             std::shared_ptr<Texture2D> baseColorTexture,
             std::shared_ptr<Texture2D> unlitColorTexture,
             std::shared_ptr<Texture2D> normalMapTexture,
             std::shared_ptr<Texture2D> specularTexture,
             std::shared_ptr<Texture2D> overlayColorTexture)
    : Model(ctx, std::move(name), std::move(mesh)), 
      _baseColorTexture(std::move(baseColorTexture)), 
      _unlitColorTexture(std::move(unlitColorTexture)),
      _normalMapTexture(std::move(normalMapTexture)),
      _specularTexture(std::move(specularTexture)),
      _overlayColorTexture(std::move(overlayColorTexture))
{
    std::vector<Descriptor> descriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _baseColorTexture->getDescriptorInfo()), // Base color texture
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _unlitColorTexture->getDescriptorInfo()), // Unlit color texture
        Descriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _normalMapTexture->getDescriptorInfo()), // Normal map texture
        Descriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _specularTexture->getDescriptorInfo()), // Specular texture
        Descriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _overlayColorTexture->getDescriptorInfo()) // Overlay color texture
    };
    _descriptorSet = std::make_unique<DescriptorSet>(_ctx, descriptors);
}


Earth::~Earth()
{
    // Cleanup resources if needed
}


void Earth::draw(VkCommandBuffer commandBuffer, const Scene& scene)
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


void Earth::drawSelection(VkCommandBuffer commandBuffer, const Scene& scene)
{
    // Bind the selection pipeline and descriptor set
    _selectionPipeline->bind(commandBuffer);
    
    VkBuffer vertexBuffers[] = {_mesh->getVertexBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    std::array<VkDescriptorSet, 2> descriptorSets = {
        scene.getSceneDescriptorSet()->getDescriptorSet(),        // Scene descriptor set
        _descriptorSet->getDescriptorSet()                        // My descriptor set
    };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _selectionPipeline->getPipelineLayout(), 0, 2, descriptorSets.data(), 0, nullptr);

    // Push constants for model
    vkCmdPushConstants(commandBuffer, _selectionPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), &_modelMatrix);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(_mesh->getIndicesCount()), 1, 0, 0, 0);
}