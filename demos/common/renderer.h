#ifndef DEKOI_DEMOS_COMMON_RENDERER_H
#define DEKOI_DEMOS_COMMON_RENDERER_H

#include <dekoi/graphics/renderer.h>

#include <stdint.h>

struct DkdRenderer;
struct DkdWindow;

struct DkdShaderCreateInfo {
    enum DkShaderStage stage;
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
    const struct DkdShaderCreateInfo *pShaderInfos;
    float clearColor[4];
    uint32_t vertexBufferCount;
    const struct DkVertexBufferCreateInfo *pVertexBufferInfos;
    uint32_t vertexBindingDescriptionCount;
    const struct DkVertexBindingDescriptionCreateInfo
        *pVertexBindingDescriptionInfos;
    uint32_t vertexAttributeDescriptionCount;
    const struct DkVertexAttributeDescriptionCreateInfo
        *pVertexAttributeDescriptionInfos;
    uint32_t vertexCount;
    uint32_t instanceCount;
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;
};

int
dkdCreateRenderer(struct DkdWindow *pWindow,
                  const struct DkdRendererCreateInfo *pCreateInfo,
                  struct DkdRenderer **ppRenderer);

void
dkdDestroyRenderer(struct DkdWindow *pWindow, struct DkdRenderer *pRenderer);

int
dkdResizeRendererSurface(struct DkdRenderer *pRenderer,
                         unsigned int width,
                         unsigned int height);

int
dkdDrawRendererImage(struct DkdRenderer *pRenderer);

#endif /* DEKOI_DEMOS_COMMON_RENDERER_H */
