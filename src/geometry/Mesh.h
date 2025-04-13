#pragma once
#include "../stdafx.h"
#include "Vertex.h"

class Mesh {
public:
    std::vector<Vertex> vertices;  // Vertex data
    std::vector<uint32_t> indices; // Index data
};