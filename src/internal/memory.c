#include <stddef.h>

#include "../dekoi.h"
#include "../memory.h"
#include "assert.h"
#include "memory.h"

#ifndef DK_DEFAULT_ALLOCATION_CALLBACK
#include <stdlib.h>

static void *
dkAllocate(void *pContext,
           DkSize size)
{
    DK_UNUSED(pContext);
    return malloc(size);
}

#define DK_DEFAULT_ALLOCATION_CALLBACK dkAllocate
#endif /* DK_DEFAULT_ALLOCATION_CALLBACK */

#ifndef DK_DEFAULT_FREE_CALLBACK
#include <stdlib.h>

static void
dkFree(void *pContext,
       void *pMemory)
{
    DK_UNUSED(pContext);
    free(pMemory);
}

#define DK_DEFAULT_FREE_CALLBACK dkFree
#endif /* DK_DEFAULT_FREE_CALLBACK */


static const DkAllocator defaultAllocator = {
    NULL,
    DK_DEFAULT_ALLOCATION_CALLBACK,
    DK_DEFAULT_FREE_CALLBACK
};


void
dkGetDefaultAllocator(const DkAllocator **ppAllocator)
{
    DK_ASSERT(ppAllocator != NULL);

    *ppAllocator = &defaultAllocator;
}
