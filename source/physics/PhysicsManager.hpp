#pragma once

#include "macros/singleton.hpp"

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "BroadPhaseLayers.hpp"
#include "Layers.hpp"
#include "ObjectLayerPairFilterImpl.hpp"
#include "BPLayerInterfaceImpl.hpp"
#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

class PhysicsManager {
private:
    JPH::PhysicsSystem mPhysicsSystem;

    JPH::JobSystem *mJobSystem;
    JPH::TempAllocator *mTempAllocator;

    BPLayerInterfaceImpl mBroadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadPhaseLayerFilter;
    ObjectLayerPairFilterImpl mObjectLayerPairFilter;

public:
    bool Initialize();
    void Shutdown();

	void OptimizeBroadPhase();
    
    JPH::BodyID CreateBox(const JPH::Vec3 &inPosition, const JPH::Vec3 &inSize, bool inIsDynamic = false);
	JPH::BodyID CreateBall(const JPH::Vec3 &inPosition, const float inSize);
    void DestroyBody(JPH::BodyID inBodyID);

    void Update();

    inline JPH::BodyInterface &GetBodyInterface() {
        return mPhysicsSystem.GetBodyInterface();
    }
};
