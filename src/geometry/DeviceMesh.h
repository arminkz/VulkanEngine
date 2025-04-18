#pragma once
#include "../stdafx.h"
#include "../VulkanContext.h"
#include "HostMesh.h"

// Mesh representation on GPU
class DeviceMesh
{
public:
    DeviceMesh(std::shared_ptr<VulkanContext> ctx, const HostMesh& mesh);
    ~DeviceMesh();

private:
    std::shared_ptr<VulkanContext> _ctx;

    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

    void createVertexBuffer(const HostMesh& mesh);
    void createIndexBuffer(const HostMesh& mesh);
};