#ifndef DEKOI_DEMOS_COMMON_RENDERER_H
#define DEKOI_DEMOS_COMMON_RENDERER_H

#include "common.h"

#include <dekoi/graphics/renderer.h>

#include <stdint.h>

struct DkdShaderCreateInfo {
    DkShaderStage stage;
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
    uint32_t vertexBufferCount;
    const DkVertexBufferCreateInfo *pVertexBufferInfos;
    uint32_t vertexBindingDescriptionCount;
    const DkVertexBindingDescriptionCreateInfo *pVertexBindingDescriptionInfos;
    uint32_t vertexAttributeDescriptionCount;
    const DkVertexAttributeDescriptionCreateInfo
        *pVertexAttributeDescriptionInfos;
    uint32_t vertexCount;
    uint32_t instanceCount;
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
