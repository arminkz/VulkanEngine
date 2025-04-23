#include "Camera.h"

Camera::Camera(const CameraParams& params) {
    _radius = params.radius;
    _minRadius = params.minRadius;
    _maxRadius = params.maxRadius;
    _target = params.target;

    _up = glm::normalize(params.initialUp);
    _forward = glm::normalize(params.initialForward);
    _left = glm::normalize(glm::cross(_up, _forward));

    _viewMatrix = glm::lookAt(_target + _radius * -1.f * _forward, _target, _up);
}

void Camera::rotateHorizontally(float delta) {
    // rotate camera frame around the up vector
    glm::quat qYaw = glm::angleAxis(delta, _up);

    _forward = qYaw * _forward;
    _left = glm::cross(_up, _forward);
    _left = glm::normalize(_left);

    _viewMatrix = glm::lookAt(_target + _radius * -1.f * _forward, _target, _up);
}

void Camera::rotateVertically(float delta) {
    // rotate camera frame around the left vector
    glm::quat qPitch = glm::angleAxis(delta, _left);

    _forward = qPitch * _forward;
    _up = glm::cross(_forward, _left);
    _up = glm::normalize(_up);

    _viewMatrix = glm::lookAt(_target + _radius * -1.f * _forward, _target, _up);
}

void Camera::changeZoom(float delta) {
    _radius = glm::clamp(_radius + delta, _minRadius, _maxRadius);

    _viewMatrix = glm::lookAt(_target + _radius * -1.f * _forward, _target, _up);
}

glm::mat4 Camera::getViewMatrix() {
    return _viewMatrix;
}

glm::vec3 Camera::getPosition() {
    return _target + -1.f * _radius * _forward;
}