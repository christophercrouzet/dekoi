#ifndef DEKOI_MEMORY_H
#define DEKOI_MEMORY_H

#include "dekoi.h"

typedef void *(*DkPfnAllocationCallback)(void *, DkSize);
typedef void (*DkPfnFreeCallback)(void *, void *);

struct DkAllocator {
    void *pContext;
    DkPfnAllocationCallback pfnAllocate;
    DkPfnFreeCallback pfnFree;
};

#endif /* DEKOI_MEMORY_H */
