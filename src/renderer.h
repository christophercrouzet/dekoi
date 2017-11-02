#ifndef DEKOI_RENDERER_H
#define DEKOI_RENDERER_H

#include "dekoi.h"

typedef struct VkAllocationCallbacks VkAllocationCallbacks;

typedef DkResult (*DkPfnCreateInstanceExtensionNamesCallback)
    (void *, DkUint32 *, const char ***);
typedef void (*DkPfnDestroyInstanceExtensionNamesCallback)
    (void *, const char **);

struct DkWindowManagerInterface {
    void *pContext;
    DkPfnCreateInstanceExtensionNamesCallback pfnCreateInstanceExtensionNames;
    DkPfnDestroyInstanceExtensionNamesCallback pfnDestroyInstanceExtensionNames;
};

struct DkRendererCreateInfo {
    const char *pApplicationName;
    DkUint32 applicationMajorVersion;
    DkUint32 applicationMinorVersion;
    DkUint32 applicationPatchVersion;
    const DkWindowManagerInterface *pWindowManagerInterface;
    const VkAllocationCallbacks *pBackEndAllocator;
};

DkResult dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                          const DkAllocator *pAllocator,
                          DkRenderer **ppRenderer);
void dkDestroyRenderer(DkRenderer *pRenderer);

#endif /* DEKOI_RENDERER_H */
