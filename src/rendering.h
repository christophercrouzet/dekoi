#ifndef DEKOI_RENDERING_H
#define DEKOI_RENDERING_H

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

typedef DkResult (*DkPfnCreateInstanceExtensionNamesCallback)(
    void *pData,
    DkUint32 *pExtensionCount,
    const char ***pppExtensionNames);
typedef void (*DkPfnDestroyInstanceExtensionNamesCallback)(
    void *pData,
    const char **ppExtensionNames);
typedef DkResult (*DkPfnCreateSurfaceCallback)(
    void *pData,
    VkInstance instanceHandle,
    const VkAllocationCallbacks *pBackEndAllocator,
    VkSurfaceKHR *pSurfaceHandle);

struct DkWindowSystemIntegrationCallbacks {
    void *pData;
    DkPfnCreateInstanceExtensionNamesCallback pfnCreateInstanceExtensionNames;
    DkPfnDestroyInstanceExtensionNamesCallback pfnDestroyInstanceExtensionNames;
    DkPfnCreateSurfaceCallback pfnCreateSurface;
};

struct DkShaderCreateInfo {
    DkShaderStage stage;
    DkSize codeSize;
    DkUint32 *pCode;
    const char *pEntryPointName;
};

struct DkRendererCreateInfo {
    const char *pApplicationName;
    DkUint32 applicationMajorVersion;
    DkUint32 applicationMinorVersion;
    DkUint32 applicationPatchVersion;
    DkUint32 surfaceWidth;
    DkUint32 surfaceHeight;
    const DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator;
    DkUint32 shaderCount;
    const DkShaderCreateInfo *pShaderInfos;
    DkFloat32 clearColor[4];
    const VkAllocationCallbacks *pBackEndAllocator;
};

DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocationCallbacks *pAllocator,
                 DkRenderer **ppRenderer);

void
dkDestroyRenderer(DkRenderer *pRenderer);

DkResult
dkResizeRendererSurface(DkRenderer *pRenderer, DkUint32 width, DkUint32 height);

DkResult
dkDrawRendererImage(DkRenderer *pRenderer);

#endif /* DEKOI_RENDERING_H */