#ifndef DEKOI_MEMORY_H
#define DEKOI_MEMORY_H

#include "dekoi.h"

#define DK_ALLOCATE(pAllocator, size) \
    (pAllocator->pfnAllocate(pAllocator->pContext, size))
#define DK_FREE(pAllocator, pMemory) \
    (pAllocator->pfnFree(pAllocator->pContext, pMemory))

typedef void *(*DkPfnAllocationCallback)(void *, DkSize);
typedef void (*DkPfnFreeCallback)(void *, void *);

struct DkAllocator {
    void *pContext;
    DkPfnAllocationCallback pfnAllocate;
    DkPfnFreeCallback pfnFree;
};

#endif /* DEKOI_MEMORY_H */
