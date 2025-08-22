cmake_minimum_required(VERSION 3.5)
project(cube-engine LANGUAGES CXX VERSION 0.1.0)

# Set the C++ standard to C++20 and make it required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate the Python executable (try to find Python3 first, fall back to Python2 if not found)
find_program(PYTHON_EXECUTABLE NAMES python3 python)

# Check if the Python executable was found
if(NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Python executable not found! Please install Python or specify the path manually.")
else()
    message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")
endif()

# Configure SDL library build options use use the static version of SDL
set(SDL_SHARED OFF CACHE BOOL "")
set(SDL_STATIC ON CACHE BOOL "")

# Add SDL as a dependency
add_subdirectory(thirdparty/sdl)

# Add EASTL (EA Standard Template Library) as a dependency
add_subdirectory(thirdparty/eastl)

# Add Jolt Physics as a dependency
add_subdirectory(thirdparty/jolt/Build)

# Configure Assimp to only build the library and not the tools
# Also disable all the importers and exporters except for certain importers
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")

# Add Assimp as a dependency
add_subdirectory(thirdparty/assimp)

# Collect all .cpp and .hpp files from specific directories
file(GLOB_RECURSE SOURCES
    source/*.cpp
    source/*.hpp
    source/graphics/*.cpp
    source/graphics/*.hpp
    source/macros/*.cpp
    source/macros/*.hpp
)

# Define the executable target using the collected source files
add_executable(cube-engine ${SOURCES})

# Check if we are using MSVC and set warning flags accordingly
if (MSVC)
    # Enable all level 4 warnings
    target_compile_options(cube-engine PRIVATE /W4)
else()
    # Enable all warnings
    target_compile_options(cube-engine PRIVATE -Wall -Wextra -pedantic)
endif()

# Check if we are using MSVC and set exception handling flags accordingly
if (MSVC)
    target_compile_options(cube-engine PRIVATE /EHs-c-)
else()
    target_compile_options(cube-engine PRIVATE -fno-exceptions)
endif()

# Link the executable target with the required libraries
target_link_libraries(cube-engine PRIVATE SDL3-static EASTL Jolt assimp)

# Add include directories to the executable target
target_include_directories(cube-engine PRIVATE
    source
    thirdparty/sdl/include
    thirdparty/shadercross
    thirdparty/glm
    thirdparty/eabase/include/Common
    thirdparty/eastl/include
    thirdparty/jolt
    thirdparty/assimp/include
)

# Automatically run the Python script to build the shaders
execute_process(
    COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-shaders.py
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/source/graphics/shaders
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE SHADERS_RESULT
)

# Check if the Python script was successful
if(NOT SHADERS_RESULT EQUAL 0)
    message(FATAL_ERROR "Python script failed with error code: ${SHADERS_RESULT}")
endif()

# Copy the content folder over to the build directory
file(COPY content DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
