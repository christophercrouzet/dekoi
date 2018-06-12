#ifndef DEKOI_DEMOS_CUSTOMALLOCATOR_ALLOCATOR_H
#define DEKOI_DEMOS_CUSTOMALLOCATOR_ALLOCATOR_H

#include <stddef.h>

struct DkdAllocationCallbacks;

struct DkdAllocationCallbacksData {
    size_t used;
};

int
dkdCreateCustomAllocator(struct DkdAllocationCallbacks **ppAllocator);

void
dkdDestroyCustomAllocator(struct DkdAllocationCallbacks *pAllocator);

#endif /* DEKOI_DEMOS_CUSTOMALLOCATOR_ALLOCATOR_H */
