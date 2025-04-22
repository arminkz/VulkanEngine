#pragma once
#include "../stdafx.h"
#include "HostMesh.h"

namespace MeshFactory {

    HostMesh createSphereMesh(float radius, int segments, int rings, bool skySphere = false);

} 
