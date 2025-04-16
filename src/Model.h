#pragma once
#include "stdafx.h"
#include "geometry/Mesh.h"
#include "Texture.h"
#include "VulkanContext.h"

class Model
{
public:
    Model(const VulkanContext& ctx, Mesh mesh, glm::vec3 color);
    Model(const VulkanContext& ctx, Mesh mesh, const std::string& textureFilename);

    ~Model();

private:
    const VulkanContext& _ctx;
    Mesh _mesh;
    glm::vec3 _color;
    std::unique_ptr<Texture> _texture;
    glm::mat4 _modelMatrix;
};