#include <vulkan/vulkan.h>

#include "memory.h"
#include "renderer.h"
#include "internal/assert.h"
#include "internal/memory.h"


struct DkRenderer {
    const DkAllocator *pAllocator;
};


void
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(ppRenderer != NULL);

    if (pAllocator == NULL)
        dkGetDefaultAllocator(&pAllocator);

    *ppRenderer = (DkRenderer *)
        pAllocator->pfnAllocate(pAllocator->pContext, sizeof(DkRenderer));
}


void
dkDestroyRenderer(DkRenderer *pRenderer)
{
    DK_ASSERT(pRenderer != NULL);
}
