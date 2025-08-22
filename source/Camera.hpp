#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
    bool mIsProjectionDirty;
    bool mIsViewDirty;

    glm::mat4 mProjectionMatrix;
    glm::mat4 mViewMatrix;

    float mFov;
    float mAspectRatio;
    float mPitch;
    float mYaw;
    float mDistance;

public:
    Camera(float inFov, float inPitch, float inYaw, float inDistance);
    virtual ~Camera() = default;

    glm::vec3 GetThrowDirection(float inX, float inY, float inWidth, float inHeight);

    void SetFov(float inFov);
    void SetAspectRatio(float inAspectRatio);
    void SetAspectRatio(float inWidth, float inHeight);
    void SetPitch(float inPitch);
    void SetYaw(float inYaw);
    void SetDistance(float inDistance);

    inline float GetFov() const { return mFov; }
    inline float GetAspectRatio() const { return mAspectRatio; }
    inline float GetPitch() const { return mPitch; }
    inline float GetYaw() const { return mYaw; }
    inline float GetDistance() const { return mDistance; }

    const glm::vec3 GetForward() const;
    const glm::vec3 GetPosition() const;
    const glm::mat4 &GetProjectionMatrix();
    const glm::mat4 &GetViewMatrix();
};
