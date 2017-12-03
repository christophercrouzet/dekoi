#ifndef DEKOI_INTERNAL_MEMORY_H
#define DEKOI_INTERNAL_MEMORY_H

#include "../dekoi.h"
#include "../memory.h"

#define DKP_ALLOCATE(pAllocator, size) \
    (pAllocator->pfnAllocate(pAllocator->pContext, size))
#define DKP_FREE(pAllocator, pMemory) \
    (pAllocator->pfnFree(pAllocator->pContext, pMemory))

void dkpGetDefaultAllocator(const DkAllocator **ppAllocator);

#endif /* DEKOI_INTERNAL_MEMORY_H */
