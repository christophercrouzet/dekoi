#ifndef DEKOI_DEMOS_COMMON_RENDERER_H
#define DEKOI_DEMOS_COMMON_RENDERER_H

#include "common.h"

typedef enum DkdShaderStage {
    DKD_SHADER_STAGE_VERTEX = 0,
    DKD_SHADER_STAGE_TESSELLATION_CONTROL = 1,
    DKD_SHADER_STAGE_TESSELLATION_EVALUATION = 2,
    DKD_SHADER_STAGE_GEOMETRY = 3,
    DKD_SHADER_STAGE_FRAGMENT = 4,
    DKD_SHADER_STAGE_COMPUTE = 5
} DkdShaderStage;

struct DkdShaderCreateInfo {
    DkdShaderStage stage;
    const char *pFilePath;
    const char *pEntryPointName;
};

struct DkdRendererCreateInfo {
    const char *pApplicationName;
    unsigned int applicationMajorVersion;
    unsigned int applicationMinorVersion;
    unsigned int applicationPatchVersion;
    unsigned int surfaceWidth;
    unsigned int surfaceHeight;
    unsigned int shaderCount;
    const DkdShaderCreateInfo *pShaderInfos;
    float clearColor[4];
    const DkdLoggingCallbacks *pLogger;
    const DkdAllocationCallbacks *pAllocator;
};

int
dkdCreateRenderer(DkdWindow *pWindow,
                  const DkdRendererCreateInfo *pCreateInfo,
                  DkdRenderer **ppRenderer);

void
dkdDestroyRenderer(DkdWindow *pWindow, DkdRenderer *pRenderer);

int
dkdResizeRendererSurface(DkdRenderer *pRenderer,
                         unsigned int width,
                         unsigned int height);

int
dkdDrawRendererImage(DkdRenderer *pRenderer);

#endif /* DEKOI_DEMOS_COMMON_RENDERER_H */
