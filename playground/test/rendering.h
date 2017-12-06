#ifndef DEKOI_PLAYGROUND_TEST_RENDERING_H
#define DEKOI_PLAYGROUND_TEST_RENDERING_H

#include "test.h"

typedef enum ShaderStage {
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_TESSELLATION_CONTROL = 1,
    SHADER_STAGE_TESSELLATION_EVALUATION = 2,
    SHADER_STAGE_GEOMETRY = 3,
    SHADER_STAGE_FRAGMENT = 4,
    SHADER_STAGE_COMPUTE = 5
} ShaderStage;

struct ShaderCreateInfo {
    ShaderStage stage;
    const char *pFilePath;
    const char *pEntryPointName;
};

struct RendererCreateInfo {
    const char *pApplicationName;
    unsigned int applicationMajorVersion;
    unsigned int applicationMinorVersion;
    unsigned int applicationPatchVersion;
    unsigned int surfaceWidth;
    unsigned int surfaceHeight;
    unsigned int shaderCount;
    const ShaderCreateInfo *pShaderInfos;
    float clearColor[4];
};

int
createRenderer(Window *pWindow,
               const RendererCreateInfo *pCreateInfo,
               Renderer **ppRenderer);

void
destroyRenderer(Window *pWindow, Renderer *pRenderer);

int
resizeRendererSurface(Renderer *pRenderer,
                      unsigned int width,
                      unsigned int height);

int
drawRendererImage(Renderer *pRenderer);

#endif /* DEKOI_PLAYGROUND_TEST_RENDERING_H */
