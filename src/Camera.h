#pragma once
#include "stdafx.h"

class Camera 
{
public:

    struct CameraParams
    {
        float radius = 16.0f;
        float minRadius = 0.1f;  
        float maxRadius = 300.0f; 

        glm::vec3 target = glm::vec3(0.);

        glm::vec3 initialUp = glm::vec3(0., 1., 0.); // Up vector
        glm::vec3 initialForward = glm::vec3(0., 0., -1.); // Forward vector
    };

    explicit Camera(const CameraParams& params = CameraParams());

    void changeZoom(float delta);
    void rotateHorizontally(float delta);
    void rotateVertically(float delta);

    glm::mat4 getViewMatrix();
    glm::vec3 getPosition();

    void setTarget(const glm::vec3& target);
    void setTargetAnimated(const glm::vec3& target);

    void advanceAnimation(float deltaTime);

private:
    float _radius;
    float _minRadius;
    float _maxRadius;

    glm::vec3 _target;
    glm::vec3 _forward;
    glm::vec3 _left;
    glm::vec3 _up;

    glm::mat4 _viewMatrix;

    // Animation state
    bool _isAnimating = false;
    glm::vec3 _animationStartTarget;
    glm::vec3 _animationEndTarget;
    float _animationDuration = 1.0f; // in seconds
    float _animationElapsed = 0.0f;

    float easeInOutCubic(float t);
};