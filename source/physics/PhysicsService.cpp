#include "PhysicsService.hpp"

#include <cstdarg>

static void TraceImpl(const char *inFMT, ...) {
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

    printf("%s\n", buffer);
}

bool PhysicsService::Initialize() {

    JPH::RegisterDefaultAllocator();

    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl);

    JPH::Factory::sInstance = new JPH::Factory();

    JPH::RegisterTypes();

    mTempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    mJobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    const JPH::uint cMaxBodies = 1024;
    const JPH::uint cNumBodyMutexes = 0;
    const JPH::uint cMaxBodyPairs = 1024;
    const JPH::uint cMaxContactConstraints = 1024;

    mPhysicsSystem.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, mBroadPhaseLayerInterface, mObjectVsBroadPhaseLayerFilter, mObjectLayerPairFilter);

    // Set listeneres... just testing for now
    //mPhysicsSystem.SetBodyActivationListener(&mBodyActivationListener);
    //mPhysicsSystem.SetContactListener(&mContactListener);

    // TODO: Move this to some function for when a scene is fully loaded
    //mPhysicsSystem.OptimizeBroadPhase();

    return true;
}

void PhysicsService::Shutdown() {
    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete mTempAllocator;
    mTempAllocator = nullptr;

    delete mJobSystem;
    mJobSystem = nullptr;
}

JPH::BodyID PhysicsService::CreateBox(const JPH::Vec3 &inPosition, const JPH::Vec3 &inSize, bool inIsDynamic) {
    JPH::BodyInterface &body_interface = mPhysicsSystem.GetBodyInterface();

    // Create rigid body to serve as the ground plane
    JPH::BoxShapeSettings floor_shape_settings(inSize);
    floor_shape_settings.SetEmbedded();

    // Create the shape
    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();

    JPH::EMotionType motion_type = inIsDynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
    JPH::ObjectLayer layer = inIsDynamic ? Layers::MOVING : Layers::NON_MOVING;

    JPH::BodyCreationSettings floor_settings(floor_shape, inPosition, JPH::Quat::sIdentity(), motion_type, layer);
    JPH::Body *floor = body_interface.CreateBody(floor_settings);

    //body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    JPH::BodyID body_id = body_interface.CreateAndAddBody(floor_settings, JPH::EActivation::DontActivate);
    return body_id;
}

JPH::BodyID PhysicsService::CreateBall(const JPH::Vec3 &inPosition, const float inSize) {
    JPH::BodyInterface &body_interface = mPhysicsSystem.GetBodyInterface();

    JPH::SphereShapeSettings ball_shape_settings(inSize);
    ball_shape_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult ball_shape_result = ball_shape_settings.Create();
    JPH::ShapeRefC ball_shape = ball_shape_result.Get();

    JPH::BodyCreationSettings ball_settings(ball_shape, inPosition, JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    JPH::BodyID body_id = body_interface.CreateAndAddBody(ball_settings, JPH::EActivation::DontActivate);

    return body_id;
}

void PhysicsService::DestroyBody(JPH::BodyID inBodyID) {
    JPH::BodyInterface &body_interface = mPhysicsSystem.GetBodyInterface();
    body_interface.RemoveBody(inBodyID);
    body_interface.DestroyBody(inBodyID);
}

void PhysicsService::Update() {
    const float cDeltaTime = 1.0f / 60.0f;
    const int cCollisionSteps = 1;

    mPhysicsSystem.Update(cDeltaTime, cCollisionSteps, mTempAllocator, mJobSystem);
}
