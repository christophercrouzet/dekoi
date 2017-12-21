#ifndef DEKOI_MEMORY_H
#define DEKOI_MEMORY_H

#include "dekoi.h"

typedef void *(*DkPfnAllocateCallback)(void *pData, DkSize size);
typedef void *(*DkPfnReallocateCallback)(void *pData,
                                         void *pOriginal,
                                         DkSize size);
typedef void (*DkPfnFreeCallback)(void *pData, void *pMemory);

struct DkAllocationCallbacks {
    void *pData;
    DkPfnAllocateCallback pfnAllocate;
    DkPfnReallocateCallback pfnReallocate;
    DkPfnFreeCallback pfnFree;
};

#endif /* DEKOI_MEMORY_H */
