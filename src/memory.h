#ifndef DEKOI_MEMORY_H
#define DEKOI_MEMORY_H

#include "dekoi.h"

typedef void *(*DkPfnAllocateCallback)(void *, DkSize);
typedef void (*DkPfnFreeCallback)(void *, void *);

struct DkAllocationCallbacks {
    void *pData;
    DkPfnAllocateCallback pfnAllocate;
    DkPfnFreeCallback pfnFree;
};

#endif /* DEKOI_MEMORY_H */
