#ifndef DEKOI_MEMORY_H
#define DEKOI_MEMORY_H

#include "dekoi.h"

typedef void *(*DkPfnAllocateCallback)(void *pData, DkSize size);
typedef void *(*DkPfnReallocateCallback)(void *pData,
                                         void *pOriginal,
                                         DkSize size);
typedef void (*DkPfnFreeCallback)(void *pData, void *pMemory);
typedef void *(*DkPfnAllocateAlignedCallback)(void *pData,
                                              DkSize size,
                                              DkSize alignment);
typedef void *(*DkPfnReallocateAlignedCallback)(void *pData,
                                                void *pOriginal,
                                                DkSize size,
                                                DkSize alignment);
typedef void (*DkPfnFreeAlignedCallback)(void *pData, void *pMemory);

struct DkAllocationCallbacks {
    void *pData;
    DkPfnAllocateCallback pfnAllocate;
    DkPfnReallocateCallback pfnReallocate;
    DkPfnFreeCallback pfnFree;
    DkPfnAllocateAlignedCallback pfnAllocateAligned;
    DkPfnReallocateAlignedCallback pfnReallocateAligned;
    DkPfnFreeAlignedCallback pfnFreeAligned;
};

#endif /* DEKOI_MEMORY_H */
