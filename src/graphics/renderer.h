#ifndef DEKOI_GRAPHICS_RENDERING_H
#define DEKOI_GRAPHICS_RENDERING_H

#include "graphics.h"

#include "../common/common.h"

typedef enum DkShaderStage {
    DK_SHADER_STAGE_VERTEX = 0,
    DK_SHADER_STAGE_TESSELLATION_CONTROL = 1,
    DK_SHADER_STAGE_TESSELLATION_EVALUATION = 2,
    DK_SHADER_STAGE_GEOMETRY = 3,
    DK_SHADER_STAGE_FRAGMENT = 4,
    DK_SHADER_STAGE_COMPUTE = 5
} DkShaderStage;

#define DKP_DEFINE_VULKAN_HANDLE(object) typedef struct object##_T *object

#if DK_ENVIRONMENT == 32
#define DKP_DEFINE_NON_DISPATCHABLE_VULKAN_HANDLE(object)                      \
    typedef DkUint64 object
#else
#define DKP_DEFINE_NON_DISPATCHABLE_VULKAN_HANDLE(object)                      \
    typedef struct object##_T *object
#endif /* DK_ENVIRONMENT */

typedef struct VkAllocationCallbacks VkAllocationCallbacks;
DKP_DEFINE_VULKAN_HANDLE(VkInstance);
DKP_DEFINE_NON_DISPATCHABLE_VULKAN_HANDLE(VkSurfaceKHR);

typedef DkResult (*DkPfnCreateInstanceExtensionNamesCallback)(
    void *pData,
    const DkLoggingCallbacks *pLogger,
    DkUint32 *pExtensionCount,
    const char ***pppExtensionNames);
typedef void (*DkPfnDestroyInstanceExtensionNamesCallback)(
    void *pData,
    const DkLoggingCallbacks *pLogger,
    const char **ppExtensionNames);
typedef DkResult (*DkPfnCreateSurfaceCallback)(
    void *pData,
    VkInstance instanceHandle,
    const VkAllocationCallbacks *pBackEndAllocator,
    const DkLoggingCallbacks *pLogger,
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
    const DkLoggingCallbacks *pLogger;
    const DkAllocationCallbacks *pAllocator;
};

DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 DkRenderer **ppRenderer);

void
dkDestroyRenderer(DkRenderer *pRenderer);

DkResult
dkResizeRendererSurface(DkRenderer *pRenderer, DkUint32 width, DkUint32 height);

DkResult
dkDrawRendererImage(DkRenderer *pRenderer);

#endif /* DEKOI_GRAPHICS_RENDERING_H */