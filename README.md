# Cube Engine

This project mostly serves as a massive learning experience for me in terms of 3D game engine development. I've learned various skills, such as writing low-level memory stuff using C++, using a novel graphics engine technology, refactoring code, setting up a physics engine and lots, lots more.

## Directory Structure

- `assets` - Includes unpre-processed `.blend` files that were converted to `.glb` files
- `content` - Asset files that are ready for direct importing into the game engine
- `scripts` - A collection of utility scripts that may or may not be used by the build process
- `shaders` - Raw GLSL shader files that are to be compiled to SPIR-V and then to header files
- `source` - All of the source code, including all `.cpp` and `.hpp` files
- `thirdparty` - All engine dependencies

## Dependencies

- `assimp` - Used for importing 3D model data
- `eabase` - A base utility library by EA, used by `eastl`
- `eastl` - A replacement for the C++ STL by EA
- `glm` - A math library that directly maps to GLSL
- `jolt` - Used for the physics engine
- `sdl` - Used for creating a window and rendering graphics using SDL GPU
- `shadercross` - SDL satellite library for online converting SPIR-V shaders to HLSL and Metal

## Requirements

- CMake
- MSVC
- Python 3

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
