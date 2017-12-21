#ifndef DEKOI_PLAYGROUND_TEST_RENDERING_H
#define DEKOI_PLAYGROUND_TEST_RENDERING_H

#include "test.h"

typedef enum PlShaderStage {
    PL_SHADER_STAGE_VERTEX = 0,
    PL_SHADER_STAGE_TESSELLATION_CONTROL = 1,
    PL_SHADER_STAGE_TESSELLATION_EVALUATION = 2,
    PL_SHADER_STAGE_GEOMETRY = 3,
    PL_SHADER_STAGE_FRAGMENT = 4,
    PL_SHADER_STAGE_COMPUTE = 5
} PlShaderStage;

struct PlShaderCreateInfo {
    PlShaderStage stage;
    const char *pFilePath;
    const char *pEntryPointName;
};

struct PlRendererCreateInfo {
    const char *pApplicationName;
    unsigned int applicationMajorVersion;
    unsigned int applicationMinorVersion;
    unsigned int applicationPatchVersion;
    unsigned int surfaceWidth;
    unsigned int surfaceHeight;
    unsigned int shaderCount;
    const PlShaderCreateInfo *pShaderInfos;
    float clearColor[4];
    const PlLoggingCallbacks *pLogger;
};

int
plCreateRenderer(PlWindow *pWindow,
                 const PlRendererCreateInfo *pCreateInfo,
                 PlRenderer **ppRenderer);

void
plDestroyRenderer(PlWindow *pWindow, PlRenderer *pRenderer);

int
plResizeRendererSurface(PlRenderer *pRenderer,
                        unsigned int width,
                        unsigned int height);

int
plDrawRendererImage(PlRenderer *pRenderer);

#endif /* DEKOI_PLAYGROUND_TEST_RENDERING_H */
