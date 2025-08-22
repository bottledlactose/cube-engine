#include "PhysicsManager.hpp"

#include <cstdarg>

static void TraceImpl(const char *inFMT, ...) {
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

    printf("%s\n", buffer);
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine) {
    printf("%s:%d: (%s) %s\n", inFile, inLine, inExpression, inMessage != nullptr ? inMessage : "");
	return true;
};

#endif // JPH_ENABLE_ASSERTS

bool PhysicsManager::Initialize() {

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

    return true;
}

void PhysicsManager::Shutdown() {
    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete mTempAllocator;
    mTempAllocator = nullptr;

    delete mJobSystem;
    mJobSystem = nullptr;
}

void PhysicsManager::OptimizeBroadPhase() {
    mPhysicsSystem.OptimizeBroadPhase();
}

JPH::BodyID PhysicsManager::CreateBox(const JPH::Vec3 &inPosition, const JPH::Vec3 &inSize, bool inIsDynamic) {
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

    JPH::BodyID body_id = body_interface.CreateAndAddBody(floor_settings, JPH::EActivation::DontActivate);
    return body_id;
}

JPH::BodyID PhysicsManager::CreateBall(const JPH::Vec3 &inPosition, const float inSize) {
    JPH::BodyInterface &body_interface = mPhysicsSystem.GetBodyInterface();

    JPH::SphereShapeSettings ball_shape_settings(inSize);
    ball_shape_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult ball_shape_result = ball_shape_settings.Create();
    JPH::ShapeRefC ball_shape = ball_shape_result.Get();

    JPH::BodyCreationSettings ball_settings(ball_shape, inPosition, JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    JPH::BodyID body_id = body_interface.CreateAndAddBody(ball_settings, JPH::EActivation::DontActivate);

    return body_id;
}

void PhysicsManager::DestroyBody(JPH::BodyID inBodyID) {
    JPH::BodyInterface &body_interface = mPhysicsSystem.GetBodyInterface();
    body_interface.RemoveBody(inBodyID);
    body_interface.DestroyBody(inBodyID);
}

void PhysicsManager::Update() {
    const float cDeltaTime = 1.0f / 60.0f;
    const int cCollisionSteps = 1;

    mPhysicsSystem.Update(cDeltaTime, cCollisionSteps, mTempAllocator, mJobSystem);
}
