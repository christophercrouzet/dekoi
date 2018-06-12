#include "allocator.h"

#include "common.h"
#include "logger.h"

#include <assert.h>
#include <stddef.h>

#define ZR_SPECIFY_INTERNAL_LINKAGE
#define ZR_DEFINE_IMPLEMENTATION
#define ZR_ASSERT assert
#define ZR_SIZE_TYPE size_t
#include <zero/allocator.h>

#ifndef NDEBUG
static int
dkdIsPowerOfTwo(size_t x)
{
    /* Complement and compare approach. */
    return (x != 0) && ((x & (~x + 1)) == x);
}
#endif /* NDEBUG */

static void *
dkdAllocate(void *pData, size_t size)
{
    DKD_UNUSED(pData);

    assert(size != 0);

    return zrAllocate((ZrSize)size);
}

static void *
dkdReallocate(void *pData, void *pOriginal, size_t size)
{
    DKD_UNUSED(pData);

    assert(pOriginal != NULL);
    assert(size != 0);

    return zrReallocate(pOriginal, (ZrSize)size);
}

static void
dkdFree(void *pData, void *pMemory)
{
    DKD_UNUSED(pData);

    assert(pMemory != NULL);

    zrFree(pMemory);
}

static void *
dkdAllocateAligned(void *pData, size_t size, size_t alignment)
{
    DKD_UNUSED(pData);

    assert(size != 0);
    assert(alignment != 0 && dkdIsPowerOfTwo(alignment));

    return zrAllocateAligned((ZrSize)size, (ZrSize)alignment);
}

static void *
dkdReallocateAligned(void *pData,
                     void *pOriginal,
                     size_t size,
                     size_t alignment)
{
    DKD_UNUSED(pData);

    assert(pOriginal != NULL);
    assert(size != 0);
    assert(alignment != 0 && dkdIsPowerOfTwo(alignment));

    return zrReallocateAligned(pOriginal, (ZrSize)size, (ZrSize)alignment);
}

static void
dkdFreeAligned(void *pData, void *pMemory)
{
    DKD_UNUSED(pData);

    assert(pMemory != NULL);

    zrFreeAligned(pMemory);
}

static const struct DkdAllocationCallbacks dkdDefaultAllocator
    = {NULL,
       dkdAllocate,
       dkdReallocate,
       dkdFree,
       dkdAllocateAligned,
       dkdReallocateAligned,
       dkdFreeAligned};

static void *
dkdHandleDekoiAllocation(void *pData, DkSize size)
{
    assert(pData != NULL);

    return ((struct DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnAllocate(
            ((struct DkdDekoiAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            (size_t)size);
}

static void *
dkdHandleDekoiReallocation(void *pData, void *pOriginal, DkSize size)
{
    assert(pData != NULL);

    return ((struct DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnReallocate(
            ((struct DkdDekoiAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            pOriginal,
            (size_t)size);
}

static void
dkdHandleDekoiFreeing(void *pData, void *pMemory)
{
    assert(pData != NULL);

    ((struct DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnFree(((struct DkdDekoiAllocationCallbacksData *)pData)
                                  ->pAllocator->pData,
                              pMemory);
}

static void *
dkdHandleDekoiAlignedAllocation(void *pData, DkSize size, DkSize alignment)
{
    assert(pData != NULL);

    return ((struct DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnAllocateAligned(
            ((struct DkdDekoiAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            (size_t)size,
            (size_t)alignment);
}

static void *
dkdHandleDekoiAlignedReallocation(void *pData,
                                  void *pOriginal,
                                  DkSize size,
                                  DkSize alignment)
{
    assert(pData != NULL);

    return ((struct DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnReallocateAligned(
            ((struct DkdDekoiAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            pOriginal,
            (size_t)size,
            (size_t)alignment);
}

static void
dkdHandleDekoiAlignedFreeing(void *pData, void *pMemory)
{
    assert(pData != NULL);

    ((struct DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnFreeAligned(
            ((struct DkdDekoiAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            pMemory);
}

void
dkdGetDefaultAllocator(const struct DkdAllocationCallbacks **ppAllocator)
{
    assert(ppAllocator != NULL);

    *ppAllocator = &dkdDefaultAllocator;
}

int
dkdCreateDekoiAllocationCallbacks(
    struct DkAllocationCallbacks **ppDekoiAllocator,
    struct DkdDekoiAllocationCallbacksData *pData,
    const struct DkdAllocationCallbacks *pAllocator,
    const struct DkdLoggingCallbacks *pLogger)
{
    assert(ppDekoiAllocator != NULL);
    assert(pData != NULL);
    assert(pAllocator != NULL);
    assert(pLogger != NULL);

    *ppDekoiAllocator = (struct DkAllocationCallbacks *)DKD_ALLOCATE(
        pAllocator, sizeof **ppDekoiAllocator);
    if (*ppDekoiAllocator == NULL) {
        DKD_LOG_ERROR(pLogger,
                      "failed to allocate Dekoi's allocation callbacks\n");
        return 1;
    }

    (*ppDekoiAllocator)->pData = pData;
    (*ppDekoiAllocator)->pfnAllocate = dkdHandleDekoiAllocation;
    (*ppDekoiAllocator)->pfnReallocate = dkdHandleDekoiReallocation;
    (*ppDekoiAllocator)->pfnFree = dkdHandleDekoiFreeing;
    (*ppDekoiAllocator)->pfnAllocateAligned = dkdHandleDekoiAlignedAllocation;
    (*ppDekoiAllocator)->pfnReallocateAligned
        = dkdHandleDekoiAlignedReallocation;
    (*ppDekoiAllocator)->pfnFreeAligned = dkdHandleDekoiAlignedFreeing;
    return 0;
}

void
dkdDestroyDekoiAllocationCallbacks(
    struct DkAllocationCallbacks *pDekoiAllocator,
    const struct DkdAllocationCallbacks *pAllocator)
{
    DKD_FREE(pAllocator, pDekoiAllocator);
}
