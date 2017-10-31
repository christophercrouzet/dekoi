#ifndef DEKOI_RENDERER_H
#define DEKOI_RENDERER_H

#include "dekoi.h"

typedef struct VkAllocationCallbacks VkAllocationCallbacks;

struct DkRendererCreateInfo {
    const char *pApplicationName;
    DkUint32 applicationMajorVersion;
    DkUint32 applicationMinorVersion;
    DkUint32 applicationPatchVersion;
    DkUint32 extensionCount;
    const char **ppExtensionNames;
    const VkAllocationCallbacks *pBackEndAllocator;
};

DkResult dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                          const DkAllocator *pAllocator,
                          DkRenderer **ppRenderer);
void dkDestroyRenderer(DkRenderer *pRenderer);

#endif /* DEKOI_RENDERER_H */
