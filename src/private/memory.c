#include "memory.h"

#include "assert.h"
#include "dekoi.h"

#include "../memory.h"

#include <stddef.h>
#include <stdlib.h>

static void *
dkpAllocate(void *pContext, DkSize size)
{
    DKP_UNUSED(pContext);
    return malloc((size_t)size);
}

static void
dkpFree(void *pContext, void *pMemory)
{
    DKP_UNUSED(pContext);
    free(pMemory);
}

static const DkAllocator dkpDefaultAllocator = {NULL, dkpAllocate, dkpFree};

void
dkpGetDefaultAllocator(const DkAllocator **ppAllocator)
{
    DKP_ASSERT(ppAllocator != NULL);

    *ppAllocator = &dkpDefaultAllocator;
}
