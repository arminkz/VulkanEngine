#include "DeviceModel.h"
#include "geometry/DeviceMesh.h"
#include "VulkanHelper.h"

DeviceModel::DeviceModel(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<DeviceMesh> mesh, glm::vec3 color)
    : _ctx(std::move(ctx))
    , _mesh(std::move(mesh))
{
    _texture = nullptr;
    _color = color;
    _modelMatrix = glm::mat4(1.f);
}

DeviceModel::DeviceModel(std::shared_ptr<VulkanContext> ctx, std::shared_ptr<DeviceMesh> mesh, std::shared_ptr<DeviceTexture> texture)
    : _ctx(std::move(ctx))
    , _mesh(std::move(mesh))
    , _texture(std::move(texture))
{
    _color = glm::vec3(0.f);
    _modelMatrix = glm::mat4(1.f);
}

DeviceModel::~DeviceModel()
{
}