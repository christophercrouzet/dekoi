#ifndef DEKOI_COMMON_PRIVATE_MEMORY_H
#define DEKOI_COMMON_PRIVATE_MEMORY_H

#include "../common.h"
#include "../memory.h"

#define DKP_ALLOCATE(pAllocator, size)                                         \
    ((pAllocator)->pfnAllocate((pAllocator)->pData, size))
#define DKP_REALLOCATE(pAllocator, pOriginal, size)                            \
    ((pAllocator)->pfnAllocate((pAllocator)->pData, pOriginal, size))
#define DKP_FREE(pAllocator, pMemory)                                          \
    ((pAllocator)->pfnFree((pAllocator)->pData, pMemory))
#define DKP_ALLOCATE_ALIGNED(pAllocator, size)                                 \
    ((pAllocator)->pfnAllocateAligned((pAllocator)->pData, size, alignment))
#define DKP_REALLOCATE_ALIGNED(pAllocator, pOriginal, size)                    \
    ((pAllocator)                                                              \
         ->pfnAllocateAligned(                                                 \
             (pAllocator)->pData, pOriginal, size, alignment))
#define DKP_FREE_ALIGNED(pAllocator, pMemory)                                  \
    ((pAllocator)->pfnFreeAligned((pAllocator)->pData, pMemory))

void
dkpGetDefaultAllocator(const DkAllocationCallbacks **ppAllocator);

#endif /* DEKOI_COMMON_PRIVATE_MEMORY_H */
