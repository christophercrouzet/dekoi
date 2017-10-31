#include <vulkan/vulkan.h>

#include "memory.h"
#include "renderer.h"
#include "internal/assert.h"
#include "internal/memory.h"


struct DkRenderer {
    const DkAllocator *pAllocator;
};


DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(ppRenderer != NULL);

    if (pAllocator == NULL)
        dkGetDefaultAllocator(&pAllocator);

    *ppRenderer = (DkRenderer *) DK_ALLOCATE(pAllocator, sizeof(DkRenderer));

    return DK_SUCCESS;
}


void
dkDestroyRenderer(DkRenderer *pRenderer)
{
    DK_ASSERT(pRenderer != NULL);
}
