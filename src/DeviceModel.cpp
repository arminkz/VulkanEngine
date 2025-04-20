#include "DeviceModel.h"
#include "geometry/DeviceMesh.h"
#include "VulkanHelper.h"

DeviceModel::DeviceModel(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<DeviceMesh> mesh, glm::vec3 color)
    : _ctx(std::move(ctx))
    , _mesh(std::move(mesh))
{
    _color = color;
    _modelMatrix = glm::mat4(1.f);
}

DeviceModel::DeviceModel(
    std::shared_ptr<VulkanContext> ctx, 
    std::shared_ptr<DeviceMesh> mesh,
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
    _modelMatrix = glm::mat4(1.f);

    _material.hasBaseColorTexture = _baseColorTexture ? 1 : 0;
    _material.hasUnlitColorTexture = _unlitColorTexture ? 1 : 0;
    _material.hasNormalMapTexture = _normalMapTexture ? 1 : 0;
    _material.hasSpecularTexture = _specularTexture ? 1 : 0;
    _material.hasOverlayColorTexture = _overlayColorTexture ? 1 : 0;
    _material.ambientStrength = 0.1f;
    _material.specularStrength = 5.0f;

    _materialUBO = std::make_unique<UniformBuffer<Material>>(_ctx);
    _materialUBO->update(_material);
}

DeviceModel::~DeviceModel()
{
}