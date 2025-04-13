#pragma once
#include "../stdafx.h"
#include "../geometry/Mesh.h"

class ObjLoader {
public:
    static Mesh load(const std::string& modelPath, const std::string& texturePath);
};