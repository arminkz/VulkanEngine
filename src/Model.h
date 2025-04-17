#pragma once
#include "stdafx.h"
#include "geometry/Mesh.h"
#include "Texture.h"
#include "VulkanContext.h"

class Mesh;

class Model
{
public:
    Model(const VulkanContext& ctx, Mesh mesh, glm::vec3 color);
    Model(const VulkanContext& ctx, Mesh mesh, const std::string& textureFilename);

    ~Model();

    Mesh* getMesh() const { return _mesh.get(); }
    Texture* getTexture() const { return _texture.get(); }

private:
    const VulkanContext& _ctx;
    
    std::unique_ptr<Mesh> _mesh;
    std::unique_ptr<Texture> _texture;
    glm::vec3 _color;
    glm::mat4 _modelMatrix;
};