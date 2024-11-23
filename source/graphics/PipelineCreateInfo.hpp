#pragma once

#include "ShaderCreateInfo.hpp"

struct PipelineCreateInfo {
    eastl::string mName;
    ShaderCreateInfo mVertexShader;
    ShaderCreateInfo mFragmentShader;
};
