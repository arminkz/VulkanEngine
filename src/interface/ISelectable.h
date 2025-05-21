#pragma once
#include "stdafx.h"
#include "Scene.h"
#include "Pipeline.h"


class ISelectable
{
public:
    int getId() const { return _id; }

    // Draw in selection mode
    virtual void drawSelection(VkCommandBuffer commandBuffer, const Scene& scene) = 0;

    void setSelectionPipeline(std::shared_ptr<Pipeline> pipeline) {_selectionPipeline = std::move(pipeline);}
    
protected:
    static int _nextId;
    int _id;

    // Constructor automatically assigns ID
    ISelectable() : _id(_nextId++) {}
    virtual ~ISelectable() {}

    std::shared_ptr<Pipeline> _selectionPipeline = nullptr;
};

// Initialize static member
int ISelectable::_nextId = 1;