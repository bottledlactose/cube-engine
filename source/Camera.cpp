#include "Camera.hpp"

Camera::Camera(float inFov, float inPitch, float inYaw, float inDistance) {
    mIsProjectionDirty = true;
    mIsViewDirty = true;

    mFov = inFov;
    mPitch = inPitch;
    mYaw = inYaw;
    mDistance = inDistance;
}

void Camera::SetFov(float inFov) {
    mFov = inFov;
    mIsProjectionDirty = true;
}

void Camera::SetAspectRatio(float inAspectRatio) {
    mAspectRatio = inAspectRatio;
    mIsProjectionDirty = true;
}

void Camera::SetPitch(float inPitch) {

    if (inPitch > 89.0f) {
        inPitch = 89.0f;
    }

    if (inPitch < -89.0f) {
        inPitch = -89.0f;
    }

    mPitch = inPitch;
    mIsViewDirty = true;
}

void Camera::SetYaw(float inYaw) {
    mYaw = inYaw;
    mIsViewDirty = true;
}

void Camera::SetDistance(float inDistance) {
    mDistance = inDistance;
    mIsViewDirty = true;
}

const glm::vec3 &Camera::GetPosition() {
    return glm::vec3(
        mDistance * cos(glm::radians(mPitch)) * cos(glm::radians(mYaw)),
        mDistance * sin(glm::radians(mPitch)),
        mDistance * cos(glm::radians(mPitch)) * sin(glm::radians(mYaw))
    );
}

const glm::mat4 &Camera::GetProjectionMatrix() {
    if (mIsProjectionDirty) {
        mProjectionMatrix = glm::perspective(glm::radians(mFov), mAspectRatio, 0.1f, 100.0f);
        mIsProjectionDirty = false;
    }

    return mProjectionMatrix;
}

const glm::mat4 &Camera::GetViewMatrix() {
    if (mIsViewDirty) {
        glm::vec3 position = GetPosition();
        mViewMatrix = glm::lookAt(position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        mIsViewDirty = false;
    }

    return mViewMatrix;
}
