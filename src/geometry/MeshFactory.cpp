#include "MeshFactory.h"
#include "Vertex.h"
#include "HostMesh.h"

namespace MeshFactory {

    HostMesh createSphereMesh(float radius, int segments, int rings)
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
                vert.texCoord = { u, v };

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

        return mesh;
    }

}