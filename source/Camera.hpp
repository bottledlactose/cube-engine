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
    float mPitch;
    float mYaw;
    float mDistance;

public:
    Camera(float inFov, float inPitch, float inYaw, float inDistance);
    virtual ~Camera() = default;

    void SetFov(float inFov);
    void SetPitch(float inPitch);
    void SetYaw(float inYaw);
    void SetDistance(float inDistance);

    inline float GetFov() const { return mFov; }
    inline float GetPitch() const { return mPitch; }
    inline float GetYaw() const { return mYaw; }
    inline float GetDistance() const { return mDistance; }

    const glm::vec3 &GetPosition();
    const glm::mat4 &GetProjectionMatrix();
    const glm::mat4 &GetViewMatrix();
};

/*
    projection_matrix = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

*/