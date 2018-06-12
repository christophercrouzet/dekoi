#ifndef DEKOI_COMMON_PRIVATE_ALLOCATOR_H
#define DEKOI_COMMON_PRIVATE_ALLOCATOR_H

#include "../allocator.h"
#include "../common.h"

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
dkpGetDefaultAllocator(const struct DkAllocationCallbacks **ppAllocator);

#endif /* DEKOI_COMMON_PRIVATE_ALLOCATOR_H */
