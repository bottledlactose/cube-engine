cmake_minimum_required(VERSION 3.5)
project(boomblox LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_SHARED OFF CACHE BOOL "")
set(SDL_STATIC ON CACHE BOOL "")

add_subdirectory(thirdparty/sdl)

add_subdirectory(thirdparty/eastl)

# Jolt Physics
add_subdirectory(thirdparty/jolt/Build)

file(GLOB_RECURSE SOURCES
    source/*.cpp
    source/*.hpp
    source/graphics/*.cpp
    source/graphics/*.hpp
    source/macros/*.cpp
    source/macros/*.hpp
)

add_executable(boomblox ${SOURCES})

# Check if we are using MSVC and set warning flags accordingly
if (MSVC)
    # Enable all level 4 warnings
    target_compile_options(boomblox PRIVATE /W4)
endif()

target_link_libraries(boomblox PRIVATE SDL3-static EASTL Jolt)
target_include_directories(boomblox PRIVATE
    source
    thirdparty/sdl/include
    thirdparty/shadercross
    thirdparty/glm
    thirdparty/eabase/include/Common
    thirdparty/eastl/include
    thirdparty/jolt
)
