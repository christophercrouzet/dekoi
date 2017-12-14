#ifndef DEKOI_PRIVATE_MEMORY_H
#define DEKOI_PRIVATE_MEMORY_H

#include "../dekoi.h"
#include "../memory.h"

#define DKP_ALLOCATE(pAllocator, size)                                         \
    ((pAllocator)->pfnAllocate((pAllocator)->pData, size))
#define DKP_FREE(pAllocator, pMemory)                                          \
    ((pAllocator)->pfnFree((pAllocator)->pData, pMemory))

void
dkpGetDefaultAllocator(const DkAllocationCallbacks **ppAllocator);

#endif /* DEKOI_PRIVATE_MEMORY_H */
