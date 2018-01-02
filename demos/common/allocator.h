#ifndef DEKOI_DEMOS_COMMON_ALLOCATOR_H
#define DEKOI_DEMOS_COMMON_ALLOCATOR_H

#include "common.h"

#include <dekoi/common/allocator>

#include <stddef.h>

#define DKD_ALLOCATE(pAllocator, size)                                         \
    ((pAllocator)->pfnAllocate((pAllocator)->pData, size))
#define DKD_REALLOCATE(pAllocator, pOriginal, size)                            \
    ((pAllocator)->pfnAllocate((pAllocator)->pData, pOriginal, size))
#define DKD_FREE(pAllocator, pMemory)                                          \
    ((pAllocator)->pfnFree((pAllocator)->pData, pMemory))
#define DKD_ALLOCATE_ALIGNED(pAllocator, size)                                 \
    ((pAllocator)->pfnAllocateAligned((pAllocator)->pData, size, alignment))
#define DKD_REALLOCATE_ALIGNED(pAllocator, pOriginal, size)                    \
    ((pAllocator)                                                              \
         ->pfnAllocateAligned(                                                 \
             (pAllocator)->pData, pOriginal, size, alignment))
#define DKD_FREE_ALIGNED(pAllocator, pMemory)                                  \
    ((pAllocator)->pfnFreeAligned((pAllocator)->pData, pMemory))

typedef void *(*DkdPfnAllocateCallback)(void *pData, size_t size);
typedef void *(*DkdPfnReallocateCallback)(void *pData,
                                          void *pOriginal,
                                          size_t size);
typedef void (*DkdPfnFreeCallback)(void *pData, void *pMemory);
typedef void *(*DkdPfnAllocateAlignedCallback)(void *pData,
                                               size_t size,
                                               size_t alignment);
typedef void *(*DkdPfnReallocateAlignedCallback)(void *pData,
                                                 void *pOriginal,
                                                 size_t size,
                                                 size_t alignment);
typedef void (*DkdPfnFreeAlignedCallback)(void *pData, void *pMemory);

struct DkdAllocationCallbacks {
    void *pData;
    DkdPfnAllocateCallback pfnAllocate;
    DkdPfnReallocateCallback pfnReallocate;
    DkdPfnFreeCallback pfnFree;
    DkdPfnAllocateAlignedCallback pfnAllocateAligned;
    DkdPfnReallocateAlignedCallback pfnReallocateAligned;
    DkdPfnFreeAlignedCallback pfnFreeAligned;
};

struct DkdDekoiAllocationCallbacksData {
    const DkdAllocationCallbacks *pAllocator;
};

void
dkdGetDefaultAllocator(const DkdAllocationCallbacks **ppAllocator);

int
dkdCreateDekoiAllocationCallbacks(DkdDekoiAllocationCallbacksData *pData,
                                  const DkdAllocationCallbacks *pAllocator,
                                  const DkdLoggingCallbacks *pLogger,
                                  DkAllocationCallbacks **ppDekoiAllocator);

void
dkdDestroyDekoiAllocationCallbacks(DkAllocationCallbacks *pDekoiAllocator,
                                   const DkdAllocationCallbacks *pAllocator);

#endif /* DEKOI_DEMOS_COMMON_ALLOCATOR_H */
