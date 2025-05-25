#include "MeshFactory.h"
#include "Vertex.h"
#include "HostMesh.h"

namespace MeshFactory {

    // HostMesh createSphereMesh(float radius, int segments, int rings, bool skySphere) {
    //     const float PI = 3.14159265359f;

    //     HostMesh mesh;

    //     for (unsigned int i = 0; i <= rings; ++i) {
    //         float phi = PI * i / rings; // φ from 0 to π

    //         for (unsigned int j = 0; j <= segments; ++j) {
    //             float theta = 2 * PI * j / segments; // θ from 0 to 2π

    //             // Spherical to Cartesian conversion
    //             float x = radius * sinf(phi) * cosf(theta);
    //             float y = radius * cosf(phi);
    //             float z = radius * sinf(phi) * sinf(theta);

    //             Vertex vertex;
    //             vertex.pos = { x, y, z };
    //             vertex.color = glm::vec4(1.f); // Default color (white)
    //             vertex.texCoord = { static_cast<float>(j) / segments, static_cast<float>(i) / rings };
    //             vertex.normal = { x / radius, y / radius, z / radius }; // Normalized position vector
    //             vertex.tangent = { -sinf(theta), 0.0f, cosf(theta) }; // Tangent vector (for normal mapping)

    //             // Add vertex to mesh
    //             mesh.vertices.push_back(vertex);
    //         }
    //     }

    //     // Generate indices for triangle strips
    //     for (unsigned int i = 0; i < rings; ++i) {
    //         for (unsigned int j = 0; j < segments; ++j) {
    //             unsigned int first = i *  (segments + 1) + j;
    //             unsigned int second = first + segments + 1;

    //             if(!skySphere){
    //                 mesh.indices.push_back(first);
    //                 mesh.indices.push_back(second);
    //                 mesh.indices.push_back(first + 1);
    
    //                 mesh.indices.push_back(first + 1);
    //                 mesh.indices.push_back(second);
    //                 mesh.indices.push_back(second + 1);
    //             }
    //             else{
    //                 mesh.indices.push_back(first);
    //                 mesh.indices.push_back(first + 1);
    //                 mesh.indices.push_back(second);
                    
    
    //                 mesh.indices.push_back(first + 1);
    //                 mesh.indices.push_back(second + 1);
    //                 mesh.indices.push_back(second);
    //             }

    //         }
    //     }

    //     return mesh;
    // }

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
                vert.tangent = glm::normalize(glm::vec3(-sinPhi, cosPhi, 0.0)); // Tangent (u direction)

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

    HostMesh createQuadMesh(float width, float height, bool twoSided)
    {
        HostMesh mesh;

        Vertex v0;
        v0.pos = { -width / 2, 0.0f, -height / 2 };
        v0.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        v0.texCoord = { 0.0f, 0.0f };
        v0.normal = { 0.0f, 1.0f, 0.0f };
        v0.tangent = { 1.0f, 0.0f, 0.0f };

        Vertex v1;
        v1.pos = { width / 2, 0.0f, -height / 2 };
        v1.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        v1.texCoord = { 1.0f, 0.0f };
        v1.normal = { 0.0f, 1.0f, 0.0f };
        v1.tangent = { 1.0f, 0.0f, 0.0f };

        Vertex v2;
        v2.pos = { width / 2, 0.0f, height / 2 };
        v2.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        v2.texCoord = { 1.0f, 1.0f };
        v2.normal = { 0.0f, 1.0f, 0.0f };
        v2.tangent = { 1.0f, 0.0f, 0.0f };

        Vertex v3;
        v3.pos = { -width / 2, 0.0f, height / 2 };
        v3.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        v3.texCoord = { 0.0f, 1.0f };
        v3.normal = { 0.0f, 1.0f, 0.0f };
        v3.tangent = { 1.0f, 0.0f, 0.0f };

        mesh.vertices.push_back(v0);
        mesh.vertices.push_back(v1);
        mesh.vertices.push_back(v2);
        mesh.vertices.push_back(v3);

        mesh.indices.push_back(0);
        mesh.indices.push_back(1);
        mesh.indices.push_back(2);

        mesh.indices.push_back(0);
        mesh.indices.push_back(2);
        mesh.indices.push_back(3);

        if(twoSided) {
            mesh.indices.push_back(0);
            mesh.indices.push_back(2);
            mesh.indices.push_back(1);
    
            mesh.indices.push_back(0);
            mesh.indices.push_back(3);
            mesh.indices.push_back(2);
        }

        return mesh;
    }

    
    HostMesh createCubeMesh(float width, float height, float depth)
    {
        HostMesh mesh;

        // Define the 8 vertices of the cube
        std::array<glm::vec3, 8> vertices = {
            glm::vec3(-width / 2, -height / 2, -depth / 2),
            glm::vec3(width / 2, -height / 2, -depth / 2),
            glm::vec3(width / 2, height / 2, -depth / 2),
            glm::vec3(-width / 2, height / 2, -depth / 2),
            glm::vec3(-width / 2, -height / 2, depth / 2),
            glm::vec3(width / 2, -height / 2, depth / 2),
            glm::vec3(width / 2, height / 2, depth / 2),
            glm::vec3(-width / 2, height / 2, depth / 2)
        };

        // Define the indices for the cube's faces
        std::array<uint32_t, 36> indices = {
            // Front face
            0, 2, 1,
            0, 3, 2,
            // Back face
            4, 5, 6,
            4, 6, 7,
            // Left face
            0, 4, 7,
            0, 7, 3,
            // Right face
            1, 6, 5,
            1, 2, 6,
            // Top face
            3, 6, 2,
            3, 7, 6,
            // Bottom face
            0, 1, 5,
            0, 5, 4
        };

        for (const auto& vertex : vertices) {
            Vertex v;
            v.pos = vertex;
            v.color = {1.0f,1.0f,1.0f,1.0f}; // Default color (white)
            v.texCoord = {0.0f, 0.0f};       // Default texture coordinates
            v.normal = {0.0f, 0.0f, 0.0f};   // Default normal (to be calculated later)
            v.tangent = {0.0f, 0.0f, 0.0f};  // Default tangent (to be calculated later)
            mesh.vertices.push_back(v);
        }

        for (const auto& index : indices) {
            mesh.indices.push_back(index);
        }

        return mesh;
    }

}