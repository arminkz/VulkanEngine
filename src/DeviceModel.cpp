#include "DeviceModel.h"
#include "geometry/DeviceMesh.h"
#include "VulkanHelper.h"
#include "DescriptorSet.h"

DeviceModel::DeviceModel(
    std::shared_ptr<VulkanContext> ctx, 
    std::shared_ptr<DeviceMesh> mesh, 
    glm::mat4 modelMatrix,
    glm::vec3 color)
    : _ctx(std::move(ctx))
    , _mesh(std::move(mesh))
{
    _color = color;
    _modelMatrix = modelMatrix;
}

DeviceModel::DeviceModel(
    std::shared_ptr<VulkanContext> ctx, 
    std::shared_ptr<DeviceMesh> mesh,
    glm::mat4 modelMatrix,
    std::shared_ptr<DeviceTexture> baseColorTexture,
    std::shared_ptr<DeviceTexture> unlitColorTexture,
    std::shared_ptr<DeviceTexture> normalMapTexture,
    std::shared_ptr<DeviceTexture> specularTexture,
    std::shared_ptr<DeviceTexture> overlayColorTexture,
    std::shared_ptr<TextureSampler> textureSampler
)
    : _ctx(std::move(ctx))
    , _mesh(std::move(mesh))
    , _baseColorTexture(std::move(baseColorTexture))
    , _unlitColorTexture(std::move(unlitColorTexture))
    , _normalMapTexture(std::move(normalMapTexture))
    , _specularTexture(std::move(specularTexture))
    , _overlayColorTexture(std::move(overlayColorTexture))
    , _textureSampler(std::move(textureSampler))
{
    _color = glm::vec3(0.f);
    _modelMatrix = modelMatrix;

    material.hasBaseColorTexture = _baseColorTexture ? 1 : 0;
    material.hasUnlitColorTexture = _unlitColorTexture ? 1 : 0;
    material.hasNormalMapTexture = _normalMapTexture ? 1 : 0;
    material.hasSpecularTexture = _specularTexture ? 1 : 0;
    material.hasOverlayColorTexture = _overlayColorTexture ? 1 : 0;
    material.ambientStrength = 0.1f;
    material.specularStrength = 3.0f;
    material.sunShadeMode = 0;

    _materialUBO = std::make_unique<UniformBuffer<Material>>(_ctx, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    _materialUBO->update(material);

    // Per-model descriptor set
    DeviceTexture* dummyTexture = DeviceTexture::getDummy(_ctx);
    VkSampler sampler = _textureSampler->getSampler();
    std::vector<Descriptor> descriptors = {
        Descriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, _materialUBO->getDescriptorInfo()), // Material UBO
        Descriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_baseColorTexture ? _baseColorTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),      // Base color texture
        Descriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_unlitColorTexture ? _unlitColorTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),    // Unlit color texture
        Descriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_normalMapTexture ? _normalMapTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),      // Normal map texture
        Descriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_specularTexture ? _specularTexture.get() : dummyTexture)->getDescriptorInfo(sampler)),        // Specular texture
        Descriptor(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, (_overlayColorTexture ? _overlayColorTexture.get() : dummyTexture)->getDescriptorInfo(sampler)) // Overlay color texture
    };
    _descriptorSet = std::make_unique<DescriptorSet>(_ctx, descriptors);
}

DeviceModel::~DeviceModel()
{
}