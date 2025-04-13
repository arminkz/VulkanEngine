#include "TurnTableCamera.h"

TurnTableCamera::TurnTableCamera(const CameraParams& params) {
    _radius = params.radius;
    _theta = params.theta;
    _phi = params.phi;
    _minRadius = params.minRadius;
    _maxRadius = params.maxRadius;
}

void TurnTableCamera::update() {
    if (!_isDirty) return;

    auto const yRot = glm::rotate(glm::mat4(1.0f), _theta, glm::vec3(0.0f, 1.0f, 0.0f));
    auto const xRot = glm::rotate(glm::mat4(1.0f), _phi, glm::vec3(1.0f, 0.0f, 0.0f));

    _position = _radius * glm::vec3(yRot * xRot * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
    _viewMatrix = glm::lookAt(_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    _isDirty = false;
}

void TurnTableCamera::changeRadius(float delta) {
    _radius = glm::clamp(_radius + delta, _minRadius, _maxRadius);
    _isDirty = true;
}

void TurnTableCamera::changeTheta(float delta) {
    _theta += delta;
    _isDirty = true;
}

void TurnTableCamera::changePhi(float delta) {
    _phi = glm::clamp(_phi + delta, 0.f, glm::pi<float>());
    _isDirty = true;
}

glm::mat4 TurnTableCamera::getViewMatrix() {
    update();
    return _viewMatrix;
}

glm::vec3 TurnTableCamera::getPosition() {
    update();
    return _position;
}