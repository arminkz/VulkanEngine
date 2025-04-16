#include "Model.h"


Model::Model(const VulkanContext& ctx, Mesh mesh, glm::vec3 color): _ctx(ctx), _mesh(mesh), _color(color)
{ 
}

Model::Model(const VulkanContext& ctx, Mesh mesh, const std::string& textureFilename): _ctx(ctx), _mesh(mesh) 
{
    _texture = std::make_unique<Texture>(ctx, textureFilename);
}

Model::~Model()
{
}