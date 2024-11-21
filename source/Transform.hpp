#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>

struct Transform {
    glm::vec3 mPosition;
    glm::quat mRotation;
    glm::vec3 mScale;

    Transform() {
        mPosition = glm::vec3(0.0f);
        mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        mScale = glm::vec3(1.0f);
    }

    static Transform FromBody(const JPH::BodyInterface &inBodyInterface, const JPH::BodyID &inBodyID) {

        JPH::Vec3 position = inBodyInterface.GetCenterOfMassPosition(inBodyID);
        JPH::Quat rotation = inBodyInterface.GetRotation(inBodyID);
        
        Transform transform;
        transform.mPosition = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
        transform.mRotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
        transform.mScale = glm::vec3(1.0f);
        return transform;
    }

    inline glm::mat4 GetModelMatrix() {
        glm::mat4 matrix = glm::mat4(1.0f);
        matrix = glm::translate(matrix, mPosition);
        matrix = matrix * glm::toMat4(mRotation);
        matrix = glm::scale(matrix, mScale);
        return matrix;
    }
};
