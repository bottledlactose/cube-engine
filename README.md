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



