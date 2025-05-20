#pragma once
#include "stdafx.h"
#include "Planet.h"
#include "VulkanContext.h"
#include "geometry/DeviceMesh.h"
#include "Texture2D.h"
#include "Pipeline.h"


class Earth : public Model, public ISelectable
{
public:
    Earth(std::shared_ptr<VulkanContext> ctx, 
          std::string name, 
          std::shared_ptr<DeviceMesh> mesh, 
          std::shared_ptr<Texture2D> baseColorTexture,
          std::shared_ptr<Texture2D> unlitColorTexture,
          std::shared_ptr<Texture2D> normalMapTexture,
          std::shared_ptr<Texture2D> specularTexture,
          std::shared_ptr<Texture2D> overlayColorTexture,
          std::shared_ptr<Pipeline> pipeline,
          std::shared_ptr<Pipeline> selectionPipeline);

    ~Earth();

    void draw(VkCommandBuffer commandBuffer, const Scene& scene) override;
    void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) override;

private:
    std::shared_ptr<Texture2D> _baseColorTexture;
    std::shared_ptr<Texture2D> _unlitColorTexture;
    std::shared_ptr<Texture2D> _normalMapTexture;
    std::shared_ptr<Texture2D> _specularTexture;
    std::shared_ptr<Texture2D> _overlayColorTexture;

    std::shared_ptr<Pipeline> _pipeline;
    std::shared_ptr<Pipeline> _selectionPipeline;
    std::unique_ptr<DescriptorSet> _descriptorSet;
};