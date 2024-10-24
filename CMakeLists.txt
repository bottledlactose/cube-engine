cmake_minimum_required(VERSION 3.5)
project(boomblox LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL_SHARED OFF CACHE BOOL "")
set(SDL_STATIC ON CACHE BOOL "")

add_subdirectory(thirdparty/sdl)

add_executable(boomblox source/main.cpp)

target_link_libraries(boomblox PRIVATE SDL3-static)
target_include_directories(boomblox PRIVATE
    thirdparty/sdl/include
    thirdparty/shadercross
    thirdparty/glm)
