#include <stdio.h>
#include <string>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

// Used for cross-platform shader loading
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#include <SDL_gpu_shadercross.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/vertices/PositionNormalColorVertex.hpp"
#include "Context.hpp"
#include "graphics/Renderer.hpp"

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

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...)
{
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	//cout << buffer << endl;
    printf("%s\n", buffer);
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine)
{
	// Print to the TTY
	//cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;
    printf("%s:%d: (%s) %s\n", inFile, inLine, inExpression, inMessage != nullptr ? inMessage : "");

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
		switch (inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl() {
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual JPH::uint GetNumBroadPhaseLayers() const override {
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
		switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:return "MOVING";
		default:
            JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// An example contact listener
class MyContactListener : public JPH::ContactListener {
public:
	// See: ContactListener
	virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
		//cout << "Contact validate callback" << endl;
        printf("Contact validate callback\n");

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
		//cout << "A contact was added" << endl;
        printf("A contact was added\n");
	}

	virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
	{
		//cout << "A contact was persisted" << endl;
        printf("A contact was persisted\n");
	}

	virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
	{
		//cout << "A contact was removed" << endl;
        printf("A contact was removed\n");
	}
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener{
public:
	virtual void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override {
		//cout << "A body got activated" << endl;
        printf("A body got activated\n");
	}

	virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
	{
		//cout << "A body went to sleep" << endl;
        printf("A body went to sleep\n");
	}
};

// TODO: Clean up these weird definitions for EASTL

// testing
void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

// testing even more
void* __cdecl operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* file, int line, unsigned int debugFlags, const char* name, int flags) {
    // Implement a simple aligned memory allocation if needed
    // For simplicity, assuming default alignment
    return ::operator new(size);
}

#include <EASTL/vector.h>

static MeshHandle *mesh_handle = nullptr;
static SDL_GPUTexture *depth_texture = nullptr;

static glm::mat4 projection_matrix = glm::mat4(1.0f);
static glm::mat4 view_matrix = glm::mat4(1.0f);

static eastl::vector<glm::mat4> model_matrices = {
    glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
    glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
};

static PositionNormalColorVertex vertices[36] = {
    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},

    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},

    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},

    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},

    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},

    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},

    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},

    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},

    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},

    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},

    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},
};

// Physics testing objects
static JPH::PhysicsSystem physics_system;

JPH::JobSystemThreadPool *p_job_system;
JPH::TempAllocator *p_temp_allocator;

BPLayerInterfaceImpl broad_phase_layer_interface;
ObjectVsBroadPhaseLayerFilterImpl object_vs_broad_phase_layer_filter;
ObjectLayerPairFilterImpl object_vs_layer_filter;

MyBodyActivationListener body_activation_listener;    
MyContactListener contact_listener;

JPH::BodyID floor_id;
JPH::BodyID sphere_id;

int step = 0;
// End physics testing objects

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!Context::Get().Initialize()) {
        return SDL_APP_FAILURE;
    }

    // testing only
    // TODO: Handle window resizing
    projection_matrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // TODO: Handle window resizing
    depth_texture = Renderer::Get().CreateDepthStencil(800, 600);
    if (depth_texture == nullptr) {
        return SDL_APP_FAILURE;
    }

    mesh_handle = Renderer::Get().CreateMesh(vertices, sizeof(PositionNormalColorVertex) * 36, nullptr, 0);
    if (mesh_handle == nullptr) {
        return SDL_APP_FAILURE;
    }

    // Initialize Jolt
    JPH::RegisterDefaultAllocator();
    
    // Set the trace function
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl);

    // Create the physics system
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all types
    JPH::RegisterTypes();

    p_temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    p_job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    const JPH::uint cMaxBodies = 1024;
    const JPH::uint cNumBodyMutexes = 0;
    const JPH::uint cMaxBodyPairs = 1024;
    const JPH::uint cMaxContactConstraints = 1024;

    physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broad_phase_layer_filter, object_vs_layer_filter);

    physics_system.SetBodyActivationListener(&body_activation_listener);
    physics_system.SetContactListener(&contact_listener);

    JPH::BodyInterface &body_interface = physics_system.GetBodyInterface();

    // Create rigid body to serve as the ground plane
    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
    floor_shape_settings.SetEmbedded();

    // Create the shape
    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();

    JPH::BodyCreationSettings floor_settings(floor_shape, JPH::Vec3(0.0f, -2.5f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
    JPH::Body *floor = body_interface.CreateBody(floor_settings);

    //body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);
    floor_id = body_interface.CreateAndAddBody(floor_settings, JPH::EActivation::DontActivate);

    // Create a dynamic body to bounce on the floor
    JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

    body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -1.0f, 0.0f));

    physics_system.OptimizeBroadPhase();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    // Physics testing
    JPH::BodyInterface &body_interface = physics_system.GetBodyInterface();

    JPH::RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
	JPH::Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);

    printf("Step %d: Position = (%f, %f, %f), Velocity = (%f, %f, %f)\n", step, position.GetX(), position.GetY(), position.GetZ(), velocity.GetX(), velocity.GetY(), velocity.GetZ());
    ++step;

    // Update physics
    const float cDeltaTime = 1.0f / 60.0f;
    const int cCollisionSteps = 1;

    physics_system.Update(cDeltaTime, cCollisionSteps, p_temp_allocator, p_job_system);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(Renderer::Get().GetDevice());
    if (command_buffer == nullptr) {
        fprintf(stderr, "Unable to acquire GPU command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain_texture;
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, Context::Get().GetWindow(), &swapchain_texture, nullptr, nullptr)) {
        fprintf(stderr, "Unable to acquire GPU swapchain texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchain_texture != nullptr) {
        SDL_GPURenderPass *render_pass;
        SDL_GPUColorTargetInfo color_target_info;
        SDL_zero(color_target_info);
        color_target_info.texture = swapchain_texture;
        color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info;
        SDL_zero(depth_stencil_target_info);
        depth_stencil_target_info.clear_depth = 1.0f;
        depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.texture = depth_texture;
        depth_stencil_target_info.cycle = true;

        render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);

        Renderer::Get().UseDefaultPipeline(render_pass);

        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));

        glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;
        Renderer::Get().DrawCube(command_buffer, render_pass, mesh_handle, mvp);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
    // TODO: Properly handle frame timing
    SDL_Delay(1000 / 60);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    JPH::BodyInterface &body_interface = physics_system.GetBodyInterface();

    body_interface.RemoveBody(sphere_id);
    body_interface.DestroyBody(sphere_id);

    body_interface.RemoveBody(floor_id);
    body_interface.DestroyBody(floor_id);

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete p_temp_allocator;
    p_temp_allocator = nullptr;

    delete p_job_system;
    p_job_system = nullptr;

    Renderer::Get().DestroyDepthStencil(depth_texture);
    Renderer::Get().DestroyMesh(mesh_handle);
    Renderer::Get().Shutdown();
    SDL_DestroyWindow(Context::Get().GetWindow());
    
    SDL_Quit();
}
