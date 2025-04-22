#include "DeviceModel.h"
#include "geometry/DeviceMesh.h"
#include "VulkanHelper.h"

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
    std::shared_ptr<DeviceTexture> overlayColorTexture
)
    : _ctx(std::move(ctx))
    , _mesh(std::move(mesh))
    , _baseColorTexture(std::move(baseColorTexture))
    , _unlitColorTexture(std::move(unlitColorTexture))
    , _normalMapTexture(std::move(normalMapTexture))
    , _specularTexture(std::move(specularTexture))
    , _overlayColorTexture(std::move(overlayColorTexture))
{
    _color = glm::vec3(0.f);
    _modelMatrix = modelMatrix;

    material.hasBaseColorTexture = _baseColorTexture ? 1 : 0;
    material.hasUnlitColorTexture = _unlitColorTexture ? 1 : 0;
    material.hasNormalMapTexture = _normalMapTexture ? 1 : 0;
    material.hasSpecularTexture = _specularTexture ? 1 : 0;
    material.hasOverlayColorTexture = _overlayColorTexture ? 1 : 0;
    material.ambientStrength = 0.1f;
    material.specularStrength = 5.0f;
    material.sunShadeMode = 0;

    _materialUBO = std::make_unique<UniformBuffer<Material>>(_ctx);
    _materialUBO->update(material);
}

DeviceModel::~DeviceModel()
{
}