#pragma once

#include "stdafx.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "interface/Model.h"
#include "Scene.h"
#include "Pipeline.h"


class GlowSphere : public Model
{
public:
    GlowSphere(std::shared_ptr<VulkanContext> ctx, 
               std::string name, 
               std::shared_ptr<DeviceMesh> mesh,
               std::weak_ptr<Model> parent,
               float planetSize = 1.0f);

    ~GlowSphere();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;

    const DescriptorSet* getDescriptorSet() const { return _descriptorSet.get(); }

    void calculateModelMatrix();

private:

    std::weak_ptr<Model> _parent;
    float _size = 1.0f;

    struct GlowSphereInfo {
        alignas(16) glm::vec3 color;
        alignas(4) float coeffScatter = 3.0f;
        alignas(4) float powScatter = 3.0f;
        alignas(4) int isLightSource = 0;
    };

    std::unique_ptr<UniformBuffer<GlowSphereInfo>> _glowSphereUBO;

    std::unique_ptr<DescriptorSet> _descriptorSet;
};