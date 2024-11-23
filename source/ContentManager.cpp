#include "ContentManager.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <EASTL/vector.h>

#include "macros/log.hpp"

#include "Context.hpp"
#include "graphics/RenderService.hpp"
#include "graphics/vertices/PositionNormalTextureVertex.hpp"
#include "MeshData.hpp"

SDL_GPUShader *ContentManager::LoadShader(
    const eastl::string &inPath,
    SDL_GPUShaderStage inStage,
    Uint32 inSamplerCount,
    Uint32 inUniformBufferCount,
    Uint32 inStorageBufferCount,
    Uint32 inStorageTextureCount
) {
    auto it = mShaders.find(inPath);
    if (it != mShaders.end()) {
        return it->second;
    }

    size_t code_size;
    void *code = SDL_LoadFile((Context::Get().GetBasePath() + inPath).c_str(), &code_size);
    if (code == nullptr) {
        LOG_ERROR("Unable to load shader file: %s", SDL_GetError());
        return nullptr;
    }

    SDL_GPUShader *shader = RenderService::Get().CreateShader(
        inStage,
        (const Uint8 *)code,
        code_size,
        inSamplerCount,
        inUniformBufferCount,
        inStorageBufferCount,
        inStorageTextureCount
    );

    SDL_free(code);

    mShaders[inPath] = shader;
    return shader;
}

void ContentManager::UnloadShader(const eastl::string &inPath) {
    auto it = mShaders.find(inPath);
    if (it != mShaders.end()) {
        RenderService::Get().DestroyShader(it->second);
        mShaders.erase(it);
    }
}

static MeshData AssimpProcessMesh(const aiMesh *mesh) {
    eastl::vector<PositionNormalTextureVertex> vertices;
    eastl::vector<Uint16> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        PositionNormalTextureVertex vertex;

        vertex.mPosition.x = mesh->mVertices[i].x;
        vertex.mPosition.y = mesh->mVertices[i].y;
        vertex.mPosition.z = mesh->mVertices[i].z;

        vertex.mNormal.x = mesh->mNormals[i].x;
        vertex.mNormal.y = mesh->mNormals[i].y;
        vertex.mNormal.z = mesh->mNormals[i].z;

        if (mesh->mTextureCoords[0]) {
            vertex.mTexCoords.x = mesh->mTextureCoords[0][i].x;
            vertex.mTexCoords.y = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.mTexCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(static_cast<Uint16>(face.mIndices[j]));
        }
    }

    return {
        .vertices = vertices,
        .indices = indices
    };
}

static MeshData AssimpProcessNode(const aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        // Only process the first mesh in the model file
        return AssimpProcessMesh(mesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        return AssimpProcessNode(node->mChildren[i], scene);
    }
}

MeshHandle *ContentManager::LoadMesh(const eastl::string &inPath) {

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(inPath.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("Failed to import model file:%s\n", importer.GetErrorString());
        return nullptr;
    }

    MeshData mesh_data = AssimpProcessNode(scene->mRootNode, scene);

    // Create a mesh from the data
    // TODO: Clean up input parameters (separate unit sizes from counts to ease things up)
    MeshHandle *mesh_handle = RenderService::Get().CreateMesh(
        mesh_data.vertices.data(),
        static_cast<Uint32>(sizeof(PositionNormalTextureVertex) * mesh_data.vertices.size()),
        static_cast<Uint32>(mesh_data.vertices.size()),
        mesh_data.indices.data(),
        static_cast<Uint32>(sizeof(Uint16) * mesh_data.indices.size()),
        static_cast<Uint32>(mesh_data.indices.size())
    );

    return mesh_handle;
}

void ContentManager::UnloadMesh(const eastl::string &inPath) {
    auto it = mMeshes.find(inPath);
    if (it != mMeshes.end()) {
        RenderService::Get().DestroyMesh(it->second);
        mMeshes.erase(it);
    }
}

void ContentManager::Unload() {
    for (auto &pair : mShaders) {
        RenderService::Get().DestroyShader(pair.second);
    }

    for (auto &pair : mMeshes) {
        RenderService::Get().DestroyMesh(pair.second);
    }
}
