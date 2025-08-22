#pragma once

#include <EASTL/vector.h>

#include "Camera.hpp"
#include "ContentManager.hpp"
#include "physics/PhysicsManager.hpp"

class Scene {
private:
    Camera mCamera;
    ContentManager mContentManager;
    PhysicsManager mPhysicsManager;

    MeshHandle *mCubeMesh;
    MeshHandle *mBallMesh;

    JPH::BodyID mFloorID;
    JPH::BodyID mBallID;

    eastl::vector<JPH::BodyID> mCubeBodies;

public:
    Scene();

    void Initialize();
    void Shutdown();

    void Update();
    void Draw();
};
