#include "MeshFactory.h"
#include "Vertex.h"
#include "HostMesh.h"

namespace MeshFactory {

    HostMesh createSphereMesh(float radius, int segments, int rings, bool skySphere)
    {
        HostMesh mesh;

        // Generate vertices and texture coordinates
        for (int y = 0; y <= rings; ++y) {
            float v = (float)y / rings;
            float theta = v * glm::pi<float>(); // from 0 to PI

            for (int x = 0; x <= segments; ++x) {
                float u = (float)x / segments;
                float phi = u * 2.0f * glm::pi<float>(); // from 0 to 2PI

                float sinTheta = std::sin(theta);
                float cosTheta = std::cos(theta);
                float sinPhi = std::sin(phi);
                float cosPhi = std::cos(phi);

                float px = radius * sinTheta * cosPhi;
                float py = radius * sinTheta * sinPhi;
                float pz = radius * cosTheta;

                Vertex vert;
                vert.pos = { px, py, pz };
                vert.color = glm::vec4(1.f);
                vert.texCoord = { u, v };
                vert.normal = glm::normalize(vert.pos);
                vert.tangent = glm::normalize(glm::vec3(-sinPhi * sinTheta, cosPhi * sinTheta, 0.0f));

                // Add vertex
                mesh.vertices.push_back(vert);
            }
        }

        // Generate indices
        for (int y = 0; y < rings; ++y) {
            for (int x = 0; x < segments; ++x) {
                int i0 = y * (segments + 1) + x;
                int i1 = i0 + 1;
                int i2 = i0 + (segments + 1);
                int i3 = i2 + 1;

                if (skySphere) {
                    // Triangle 1
                    mesh.indices.push_back(i0);
                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i2);

                    // Triangle 2
                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i3);
                    mesh.indices.push_back(i2);
                }
                else
                {
                    // Triangle 1
                    mesh.indices.push_back(i0);
                    mesh.indices.push_back(i2);
                    mesh.indices.push_back(i1);

                    // Triangle 2
                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i2);
                    mesh.indices.push_back(i3);
                }
            }
        }

        return mesh;
    }

    // Ring / Annulus Mesh
    HostMesh createAnnulusMesh(float innerRadius, float outerRadius, int segments)
    {
        HostMesh mesh;

        float angleStep = 2.0f * glm::pi<float>() / static_cast<float>(segments);

        for (uint32_t i = 0; i <= segments; ++i) {

            float angle = i * angleStep;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            // Outer vertex
            Vertex outerVertex;
            outerVertex.pos = { cosA * outerRadius, 0.0f, sinA * outerRadius };
            outerVertex.color = glm::vec4(1.f);
            outerVertex.texCoord = {  1.0f, static_cast<float>(i) / segments };
            outerVertex.normal = glm::vec3(0, 1, 0);
            outerVertex.tangent = glm::vec3(-sinA, 0.0f, cosA); // Tangent is perpendicular to the normal
            mesh.vertices.push_back(outerVertex);

            // Inner vertex
            Vertex innerVertex;
            innerVertex.pos = { cosA * innerRadius, 0.0f, sinA * innerRadius };
            innerVertex.color = glm::vec4(1.f);
            innerVertex.texCoord = { 0.0f, static_cast<float>(i) / segments };
            innerVertex.normal = glm::vec3(0, 1, 0);
            innerVertex.tangent = glm::vec3(-sinA, 0.0f, cosA); // Tangent is perpendicular to the normal
            mesh.vertices.push_back(innerVertex);

        }

        // Generate indices for both sides of the annulus
        for (uint32_t i = 0; i < segments; ++i) {
            uint32_t outerIndex = i * 2;
            uint32_t innerIndex = outerIndex + 1;
            uint32_t nextOuterIndex = ((i + 1) % segments) * 2;
            uint32_t nextInnerIndex = nextOuterIndex + 1;

            // Triangle 1
            mesh.indices.push_back(outerIndex);
            mesh.indices.push_back(innerIndex);
            mesh.indices.push_back(nextOuterIndex);

            // Triangle 2
            mesh.indices.push_back(innerIndex);
            mesh.indices.push_back(nextInnerIndex);
            mesh.indices.push_back(nextOuterIndex);

            mesh.indices.push_back(outerIndex);
            mesh.indices.push_back(nextOuterIndex);
            mesh.indices.push_back(innerIndex);

            mesh.indices.push_back(innerIndex);
            mesh.indices.push_back(nextOuterIndex);
            mesh.indices.push_back(nextInnerIndex);

        }

        return mesh;
    }

}