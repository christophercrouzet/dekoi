#include "../dekoi.h"
#include "../memory.h"
#include "assert.h"
#include "memory.h"

#ifndef DK_ALLOCATE
#include <stdlib.h>

void *
dkAllocate(void *pContext,
           DkSize size)
{
    DK_UNUSED(pContext);
    return malloc(size);
}

#define DK_ALLOCATE dkAllocate
#endif /* DK_ALLOCATE */

#ifndef DK_FREE
#include <stdlib.h>

void
dkFree(void *pContext,
       void *pMemory)
{
    DK_UNUSED(pContext);
    free(pMemory);
}

#define DK_FREE dkFree
#endif /* DK_FREE */


static const DkAllocator defaultAllocator = { NULL, DK_ALLOCATE, DK_FREE };


void
dkGetDefaultAllocator(const DkAllocator **ppAllocator)
{
    DK_ASSERT(ppAllocator != NULL);

    *ppAllocator = &defaultAllocator;
}
