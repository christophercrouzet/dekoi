#include "memory.h"

#include "assert.h"
#include "dekoi.h"

#include "../memory.h"

#include <stddef.h>

#ifndef DK_DEFAULT_ALLOCATION_CALLBACK
#include <stdlib.h>

static void *
dkpAllocate(void *pContext, DkSize size)
{
    DKP_UNUSED(pContext);
    return malloc((size_t)size);
}

#define DK_DEFAULT_ALLOCATION_CALLBACK dkpAllocate
#endif /* DK_DEFAULT_ALLOCATION_CALLBACK */

#ifndef DK_DEFAULT_FREE_CALLBACK
#include <stdlib.h>

static void
dkpFree(void *pContext, void *pMemory)
{
    DKP_UNUSED(pContext);
    free(pMemory);
}

#define DK_DEFAULT_FREE_CALLBACK dkpFree
#endif /* DK_DEFAULT_FREE_CALLBACK */

static const DkAllocator dkpDefaultAllocator
    = {NULL, DK_DEFAULT_ALLOCATION_CALLBACK, DK_DEFAULT_FREE_CALLBACK};

void
dkpGetDefaultAllocator(const DkAllocator **ppAllocator)
{
    DK_ASSERT(ppAllocator != NULL);

    *ppAllocator = &dkpDefaultAllocator;
}
