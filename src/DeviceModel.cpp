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
}

DeviceModel::~DeviceModel()
{
}