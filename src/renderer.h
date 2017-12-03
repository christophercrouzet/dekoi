#ifndef DEKOI_RENDERER_H
#define DEKOI_RENDERER_H

#include "dekoi.h"

typedef enum DkShaderStage {
    DK_SHADER_STAGE_VERTEX = 0,
    DK_SHADER_STAGE_TESSELLATION_CONTROL = 1,
    DK_SHADER_STAGE_TESSELLATION_EVALUATION = 2,
    DK_SHADER_STAGE_GEOMETRY = 3,
    DK_SHADER_STAGE_FRAGMENT = 4,
    DK_SHADER_STAGE_COMPUTE = 5
} DkShaderStage;

typedef struct VkAllocationCallbacks VkAllocationCallbacks;
typedef struct VkInstance_T *VkInstance;
typedef struct VkSurfaceKHR_T *VkSurfaceKHR;

typedef DkResult (*DkPfnCreateInstanceExtensionNamesCallback)(void *,
                                                              DkUint32 *,
                                                              const char ***);
typedef void (*DkPfnDestroyInstanceExtensionNamesCallback)(void *,
                                                           const char **);
typedef DkResult (*DkPfnCreateSurfaceCallback)(void *,
                                               VkInstance,
                                               const VkAllocationCallbacks *,
                                               VkSurfaceKHR *);

struct DkWindowCallbacks {
    void *pContext;
    DkPfnCreateInstanceExtensionNamesCallback pfnCreateInstanceExtensionNames;
    DkPfnDestroyInstanceExtensionNamesCallback pfnDestroyInstanceExtensionNames;
    DkPfnCreateSurfaceCallback pfnCreateSurface;
};

struct DkShaderCreateInfo {
    DkShaderStage stage;
    const char *pFilePath;
    const char *pEntryPointName;
};

struct DkRendererCreateInfo {
    const char *pApplicationName;
    DkUint32 applicationMajorVersion;
    DkUint32 applicationMinorVersion;
    DkUint32 applicationPatchVersion;
    DkUint32 surfaceWidth;
    DkUint32 surfaceHeight;
    const DkWindowCallbacks *pWindowCallbacks;
    DkUint32 shaderCount;
    const DkShaderCreateInfo *pShaderInfos;
    const DkFloat32 (*pClearColor)[4];
    const VkAllocationCallbacks *pBackEndAllocator;
};

DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer);

void
dkDestroyRenderer(DkRenderer *pRenderer, const DkAllocator *pAllocator);

DkResult
dkResizeRendererSurface(DkRenderer *pRenderer, DkUint32 width, DkUint32 height);

DkResult
dkDrawRendererImage(DkRenderer *pRenderer);

#endif /* DEKOI_RENDERER_H */
