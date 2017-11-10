#ifndef DEKOI_RENDERER_H
#define DEKOI_RENDERER_H

#include "dekoi.h"

typedef struct VkAllocationCallbacks VkAllocationCallbacks;
typedef struct VkInstance_T * VkInstance;
typedef struct VkSurfaceKHR_T * VkSurfaceKHR;

typedef DkResult (*DkPfnCreateInstanceExtensionNamesCallback)
    (void *, DkUint32 *, const char ***);
typedef void (*DkPfnDestroyInstanceExtensionNamesCallback)
    (void *, const char **);
typedef DkResult (*DkPfnCreateSurfaceCallback)
    (void *, VkInstance, const VkAllocationCallbacks *, VkSurfaceKHR *);

struct DkWindowManagerInterface {
    void *pContext;
    DkPfnCreateInstanceExtensionNamesCallback pfnCreateInstanceExtensionNames;
    DkPfnDestroyInstanceExtensionNamesCallback pfnDestroyInstanceExtensionNames;
    DkPfnCreateSurfaceCallback pfnCreateSurface;
};

struct DkRendererCreateInfo {
    const char *pApplicationName;
    DkUint32 applicationMajorVersion;
    DkUint32 applicationMinorVersion;
    DkUint32 applicationPatchVersion;
    DkUint32 surfaceWidth;
    DkUint32 surfaceHeight;
    const DkWindowManagerInterface *pWindowManagerInterface;
    const VkAllocationCallbacks *pBackEndAllocator;
};

DkResult dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                          const DkAllocator *pAllocator,
                          DkRenderer **ppRenderer);
void dkDestroyRenderer(DkRenderer *pRenderer);
DkResult dkResizeRendererSurface(DkRenderer *pRenderer,
                                 DkUint32 width,
                                 DkUint32 height);

#endif /* DEKOI_RENDERER_H */
