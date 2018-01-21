#ifndef DEKOI_DEMOS_CUSTOMALLOCATOR_ALLOCATOR_H
#define DEKOI_DEMOS_CUSTOMALLOCATOR_ALLOCATOR_H

#include "../common/common.h"

#include <stddef.h>

typedef struct DkdAllocationCallbacksData {
    size_t used;
} DkdAllocationCallbacksData;

int
dkdCreateCustomAllocator(DkdAllocationCallbacks **ppAllocator);

void
dkdDestroyCustomAllocator(DkdAllocationCallbacks *pAllocator);

#endif /* DEKOI_DEMOS_CUSTOMALLOCATOR_ALLOCATOR_H */
