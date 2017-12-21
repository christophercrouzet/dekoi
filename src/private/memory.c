#include "memory.h"

#include "assert.h"
#include "dekoi.h"

#include "../dekoi.h"
#include "../memory.h"

#include <stddef.h>
#include <stdlib.h>

static void *
dkpAllocate(void *pData, DkSize size)
{
    DKP_UNUSED(pData);
    return malloc((size_t)size);
}

static void *
dkpReallocate(void *pData, void *pOriginal, DkSize size)
{
    DKP_UNUSED(pData);
    return realloc(pOriginal, (size_t)size);
}

static void
dkpFree(void *pData, void *pMemory)
{
    DKP_UNUSED(pData);
    free(pMemory);
}

static const DkAllocationCallbacks dkpDefaultAllocator
    = {NULL, dkpAllocate, dkpReallocate, dkpFree};

void
dkpGetDefaultAllocator(const DkAllocationCallbacks **ppAllocator)
{
    DKP_ASSERT(ppAllocator != NULL);

    *ppAllocator = &dkpDefaultAllocator;
}
