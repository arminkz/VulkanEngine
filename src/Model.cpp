#include "Model.h"
#include "geometry/Mesh.h"

Model::Model(const VulkanContext& ctx, Mesh mesh, glm::vec3 color): _ctx(ctx)
{
    _mesh = std::make_unique<Mesh>(mesh);
    _texture = nullptr;
    _color = color;
    _modelMatrix = glm::mat4(1.f);
}

Model::Model(const VulkanContext& ctx, Mesh mesh, const std::string& textureFilename): _ctx(ctx)
{
    _mesh = std::make_unique<Mesh>(mesh);
    _texture = std::make_unique<Texture>(ctx, textureFilename);
    _color = glm::vec3(0.f);
    _modelMatrix = glm::mat4(1.f);
}

Model::~Model()
{
}