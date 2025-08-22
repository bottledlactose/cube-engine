#include "Camera.hpp"

Camera::Camera(float inFov, float inPitch, float inYaw, float inDistance) {
    mIsProjectionDirty = true;
    mIsViewDirty = true;

    mFov = inFov;
    mPitch = inPitch;
    mYaw = inYaw;
    mDistance = inDistance;
}

glm::vec3 Camera::GetThrowDirection(float inX, float inY, float inWidth, float inHeight) {
    // Get basis vectors
    glm::vec3 forward = GetForward();
    glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));
    glm::vec3 up = glm::normalize(glm::cross(forward, right));

    // Map screen coordinates to normalized device coordinates (NDC)
    float normalizedX = (2.0f * inX) / inWidth - 1.0f; // Maps [0, width] to [-1, 1]
    float normalizedY = 1.0f - (2.0f * inY) / inHeight; // Maps [0, height] to [1, -1]

    // Scale based on FOV and aspect ratio
    float tanFovY = tan(glm::radians(mFov) / 2.0f);
    float aspectRatio = inWidth / inHeight;
    float tanFovX = tanFovY * aspectRatio;

    // Adjust signs for right and up vectors to match screen-space behavior
    glm::vec3 scaledRight = right * -normalizedX * tanFovX; // Flip horizontal
    glm::vec3 scaledUp = up * -normalizedY * tanFovY;       // Flip vertical

    // Combine into final direction
    glm::vec3 direction = glm::normalize(forward + scaledRight + scaledUp);

    return direction;
}

void Camera::SetFov(float inFov) {
    mFov = inFov;
    mIsProjectionDirty = true;
}

void Camera::SetAspectRatio(float inAspectRatio) {
    mAspectRatio = inAspectRatio;
    mIsProjectionDirty = true;
}

void Camera::SetAspectRatio(float inWidth, float inHeight) {
    SetAspectRatio(inWidth / inHeight);
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

const glm::vec3 Camera::GetForward() const {
    return glm::vec3(
        cos(glm::radians(mPitch)) * cos(glm::radians(mYaw)),
        sin(glm::radians(mPitch)),
        cos(glm::radians(mPitch)) * sin(glm::radians(mYaw))
    );
}

const glm::vec3 Camera::GetPosition() const {
    return glm::vec3(mDistance) * GetForward();
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
