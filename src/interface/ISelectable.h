#pragma once
#include "stdafx.h"
#include "Scene.h"

class ISelectable
{
protected:
    static int _nextId;
    int _id;

    // Constructor automatically assigns ID
    ISelectable() : _id(_nextId++) {}
    virtual ~ISelectable() {}

    // Draw in selection mode
    virtual void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) = 0;

public:
    int getId() const { return _id; }
};

// Initialize static member
int ISelectable::_nextId = 1;