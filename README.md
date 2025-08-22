**NOTE:** Dependencies are currently missing due to a restore from my local archive.

# Cube Engine

![screenshot](https://raw.githubusercontent.com/bottledlactose/cube-engine/refs/heads/trunk/screenshot.png)

This project mostly serves as a massive learning experience for me in terms of 3D game engine development. I've learned various skills, such as dealing with low-level memory layouts using C++, using a novel graphics engine technology, refactoring code, setting up a physics engine and lots, lots more.

## Directory Structure

- `assets` - Includes unprocessed `.blend` files that are converted to `.glb` files in `content`
- `content` - Asset files that are ready for direct importing into the game engine
- `scripts` - A collection of utility scripts that are used by the build process
- `shaders` - Raw GLSL shader files that are to be compiled to SPIR-V and then to header files
- `source` - All of the source code, including all `.cpp` and `.hpp` files
- `thirdparty` - All engine dependencies

## Dependencies

- `assimp` - Used for importing 3D model data
- `eabase` - A base utility library by EA, used by `eastl`
- `eastl` - Used as a replacement for the C++ STL by EA ([EASTL](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html))
- `glm` - A math library that directly maps to GLSL
- `jolt` - Used for the handling the engine's physics ([Jolt Physics](https://github.com/jrouwe/JoltPhysics))
- `sdl` - Used for creating a window and rendering graphics using SDL GPU
- `shadercross` - SDL satellite library for online converting SPIR-V shaders to HLSL and Metal

## Requirements

- CMake 3.5+
- MSVC 19+ (x64)
- Python 3.11+
- Vulkan SDK (for `glslc` used by `build-shaders.py`)

## Engine Architecture

The engine's runtime begins in the `main.cpp` file, where it initializes the `Context` singleton, which then creates an SDL window and boots up the engine's singleton services, the most important of which being `RenderService` which is responsible for accessing and managing the graphics device (GPU). The idea is to have all singleton services postfixed with `*Service`, whereas services which could be instantiated are postfixed with `*Manager`, such as `ContentManager` which is responsible for loading and unloading assets from disk.

### Graphics

Most of the engine's graphics code is located in `source/graphics`, the most important file being `RenderService`. Shader files are compiled into SPIR-V bytecode which is then converted to a header file inside `source/graphics/shaders` through the script `scripts/build-shaders.py`. For example:
```cpp
// Write the data to a constant value during compile-time
// This saves the trouble of reading from disk during early shader processing
unsigned char BASIC_MESH_VERT_SHADER[] = {
    // SPIR-V bytecode goes here
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, <...>
};
// Store the size of the bytecode for easier access during runtime
unsigned int BASIC_MESH_VERT_SHADER_SIZE = sizeof(BASIC_MESH_VERT_SHADER);
```

The `RenderService` comes with various functions to create and delete resources on the graphics device, but does deliberately not manage them aside from a simple map containing the available graphics pipelines.

Things the `RenderService` handles:

- Set up the common graphics pipelines used during runtime
- Create and delete shaders (`CreateShader`, `DestroyShader`)
- Create and delete meshes with an optional index buffer (`CreateMesh`, `DestroyMesh`)
- Create various specialized buffer textures, such as a depth buffer and a MSAA buffer
- Being and end a render pass by managing state resources

I've made the concious choice to not try to wrap this whole thing in RAII, since that just tends to become a horrible mess when dealing with graphics resources. Doing it without RAII simply allowed me to take more control over how resources are managed. And thankfully Vulkan is kind enough to notify developers of any leftover resources.

The `BeginPass` and `EndPass` functions have a minor flaw though, since the command buffer can technically be shared between multiple render passes, as well as the swapchain buffer. However, since there's only a single render pass, I've decided to not worry about this too much.

The `source/graphics/uniforms` directory contains a set of struct definitions that map directly to GLSL uniforms. One thing that really bothered me here was that uniform buffers in Vulkan always need to allocate a memory in chunks of 16 bytes. This required me to use more bytes than were really needed, such as in the following case:
```cpp
struct Material {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 shininess;
};
```
As can be quickly pointed out, `shininess` should ideally only consist of a single `float`. However, since a single `float` only allocates 32 bits of memory, or 4 bytes, an additional 12 bytes (`sizeof(float) * 3`) need to be padded to the struct to make mapping to GLSL possible:
```c
struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular; 
    float shininess;
    float _padding1;
    float _padding2;
    float _padding3;
};
```
Not doing it this way makes it impossible to directly push a struct to a uniform buffer and not get any funky results. The rule of thumb I've learned is to always just use `vec4`s since these always allocate a total of 16 bytes.

The struct above could have also been written as the following:
```c
struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular; 
    vec4 shininess;
};
```
The main reason I didn't do it like this, is because you'd be forced to access `shininess` through something like `material.shininess.x` in the shader itself rather than simply `material.shininess`.

Lastly, there's also the `source/graphics/vertices` class which was originally intended to contain multiple vertex layouts, although I ended up with only a single `PositionNormalTextureVertex` struct. As the name already makes clear, it contains a vertex position, normal and texture coordinates. It does not contain color data since this is already managed through the `Material` struct in the mesh shader itself.

### Physics

All physics code is contained in the `source/physics/PhysicsManager` file. It gets initialized when then scene is loaded and is largely copy-pasted from the Jolt Physics boilerplate example code, which somehow worked out of the box.

The `PhysicsManager` comes with ways to easily manage the initialization of the physics state as well as physics bodies that are created through `CreateBall` and `CreateBox`. Whereas `CreateBall` always creates a dynamic ball, `CreateBox` comes with an additional parameter to decide if the created box body should be static or dynamic.

Additional abstraction and body management would have been a good idea for this class, especially since body management is currently up to the scene itself, whereas the `PhysicsManager` could simply keep track of all physics bodies and clear then all at once as soon as the scene shuts down.

### Content

There is only one single content type being loaded by the `ContentManager`, namely singular 3D meshes. The original idea was to have the `ContentManager` load the full 3D model rather than just the first mesh it can find. There are still leftovers of a shader loading function from when shaders were being loaded from disk rather than header files.

The `ContentManager` was designed to be used by both the `Context` as a global, long-term content storage for assets that are potentially re-used across different scenes, such as fonts, and in the scope of a scene to load short-term assets that are loaded when a scene is created, and destroyed when a scene shuts down. It's only designed to contain reference to actual resources, such as those created on the graphics device. A singular `Unload` function takes care of properly cleaning and unloading each asset loaded with the current `ContentManager`.

### Input

Since this engine involves only mouse input, the `InputService` is responsible for returning the current state of the user's input. The state is updated through the main event loop in `main.cpp` and can then be fetched globally around the engine.

## What I've Learned

I initially started working on a game idea where the player would be tasked with destroying block towers as quickly or in as little moves as possible. I wanted to use the freshly released SDL3 graphics API, but I quickly found I was more interested in the technology rather than actually making a game. So I decided to open source this project and share my findings on here.

Here are the main things I've learned from working on this project:

- Creating a 3D engine from scratch is ***A LOT*** of work even if it's a simple game idea such as the block tower destruction mechanic
- Singletons aren't so bad for game development, since they save a lot of time and don't have too many side effects if used sparingly and correctly
- Writing shaders can be tough, especially with strange alignment restrictions such as the 16 byte layout requiring padding to work correctly
- There's nothing wrong with not getting your code "right" the first time. Pretty much every single line of code I wrote initially has been changed to something else as the project grew

## What's Next?

I have no intention on keeping up development on this repository, it services primarily as an archive for what I achieved. A personal archive to reflect on my learning journey. However, I might do a few fixes here and there and clean things up a little bit.

Feel free to have a look at this project's source code as a learning resource!
