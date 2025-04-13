#pragma once
#include "stdafx.h"

class TurnTableCamera 
{
public:

    struct CameraParams
    {
        float radius = 4.0f;   // Distance from the center of rotation
        float theta = 0.0f;    // Angle around the Y-axis
        float phi = -glm::half_pi<float>();      // Angle from the Y-axis
        
        float minRadius = 0.1f;   // Minimum distance from the center of rotation
        float maxRadius = 100.0f; // Maximum distance from the center of rotation
    };

    explicit TurnTableCamera(const CameraParams& params = CameraParams());

    void changeRadius(float delta);
    void changeTheta(float delta);
    void changePhi(float delta);

    glm::mat4 getViewMatrix();
    glm::vec3 getPosition();

private:
    float _radius;
    float _theta;
    float _phi;
    float _minRadius;
    float _maxRadius;

    bool _isDirty = true;

    glm::vec3 _position;
    glm::mat4 _viewMatrix;

    void update();
};