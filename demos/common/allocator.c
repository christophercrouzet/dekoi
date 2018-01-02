#include "allocator.h"

#include "common.h"
#include "logger.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
template<typename T>
struct DkdAlignmentOfHelper {
    char first;
    T second;
};
#define DKD_ALIGNMENT_OF(type) offsetof(DkdAlignmentOfHelper<type>, second)
#else
#define DKD_ALIGNMENT_OF(type)                                                 \
    offsetof(                                                                  \
        struct {                                                               \
            char first;                                                        \
            type second;                                                       \
        },                                                                     \
        second)
#endif /* __cplusplus */

#define DKD_IS_POWER_OF_TWO(x)                                                 \
    ((x) == 1 || (x) == 2 || (x) == 4 || (x) == 8 || (x) == 16 || (x) == 32    \
     || (x) == 64 || (x) == 128 || (x) == 256 || (x) == 512 || (x) == 1024     \
     || (x) == 2048 || (x) == 4096 || (x) == 8192 || (x) == 16384              \
     || (x) == 32768 || (x) == 65536 || (x) == 131072 || (x) == 262144         \
     || (x) == 524288 || (x) == 1048576 || (x) == 2097152 || (x) == 4194304    \
     || (x) == 8388608 || (x) == 16777216 || (x) == 33554432                   \
     || (x) == 67108864 || (x) == 134217728 || (x) == 268435456                \
     || (x) == 1073741824 || (x) == 2147483648 || (x) == 4294967296)

typedef struct DkdAlignedBlockHeader {
    ptrdiff_t offset;
    size_t size;
} DkdAlignedBlockHeader;

DKD_STATIC_ASSERT(DKD_IS_POWER_OF_TWO(DKD_ALIGNMENT_OF(DkdAlignedBlockHeader)),
                  invalid_block_header_alignment);
DKD_STATIC_ASSERT(DKD_IS_POWER_OF_TWO(sizeof(void *)),
                  invalid_void_pointer_alignment);

static const size_t dkdMinAlignment
    = DKD_ALIGNMENT_OF(DkdAlignedBlockHeader) > sizeof(void *)
          ? DKD_ALIGNMENT_OF(DkdAlignedBlockHeader)
          : sizeof(void *);

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
    return malloc((size_t)size);
}

static void *
dkdReallocate(void *pData, void *pOriginal, size_t size)
{
    DKD_UNUSED(pData);
    return realloc(pOriginal, (size_t)size);
}

static void
dkdFree(void *pData, void *pMemory)
{
    DKD_UNUSED(pData);
    free(pMemory);
}

static void *
dkdAllocateAligned(void *pData, size_t size, size_t alignment)
{
    void *pOut;
    void *pBlock;
    DkdAlignedBlockHeader *pHeader;

    DKD_UNUSED(pData);

    assert(size != 0);
    assert(dkdIsPowerOfTwo(alignment));

    if (alignment < dkdMinAlignment) {
        alignment = dkdMinAlignment;
    }

    pBlock = malloc(size + alignment - 1 + sizeof(DkdAlignedBlockHeader));
    if (pBlock == NULL) {
        return NULL;
    }

    pOut = (void *)((uintptr_t)((unsigned char *)pBlock + alignment - 1
                                + sizeof(DkdAlignedBlockHeader))
                    & ~(uintptr_t)(alignment - 1));

    pHeader = &((DkdAlignedBlockHeader *)pOut)[-1];
    pHeader->offset = (unsigned char *)pOut - (unsigned char *)pBlock;
    pHeader->size = size;
    return pOut;
}

static void *
dkdReallocateAligned(void *pData,
                     void *pOriginal,
                     size_t size,
                     size_t alignment)
{
    void *pOut;
    DkdAlignedBlockHeader *pOriginalHeader;

    assert(pOriginal != NULL);
    assert(size != 0);
    assert(dkdIsPowerOfTwo(alignment));

    DKD_UNUSED(pData);

    pOriginalHeader = &((DkdAlignedBlockHeader *)pOriginal)[-1];
    if (size <= pOriginalHeader->size) {
        return pOriginal;
    }

    pOut = dkdAllocateAligned(pData, size, alignment);
    if (pOut == NULL) {
        return NULL;
    }

    memcpy(pOut, pOriginal, pOriginalHeader->size);
    return pOut;
}

static void
dkdFreeAligned(void *pData, void *pMemory)
{
    DkdAlignedBlockHeader *pHeader;

    DKD_UNUSED(pData);

    assert(pMemory != NULL);

    pHeader = &((DkdAlignedBlockHeader *)pMemory)[-1];
    free((void *)((unsigned char *)pMemory - pHeader->offset));
}

static const DkdAllocationCallbacks dkdDefaultAllocator = {NULL,
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

    return ((DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnAllocate(
            ((DkdDekoiAllocationCallbacksData *)pData)->pAllocator->pData,
            (size_t)size);
}

static void *
dkdHandleDekoiReallocation(void *pData, void *pOriginal, DkSize size)
{
    assert(pData != NULL);

    return ((DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnReallocate(
            ((DkdDekoiAllocationCallbacksData *)pData)->pAllocator->pData,
            pOriginal,
            (size_t)size);
}

static void
dkdHandleDekoiFreeing(void *pData, void *pMemory)
{
    assert(pData != NULL);

    ((DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnFree(
            ((DkdDekoiAllocationCallbacksData *)pData)->pAllocator->pData,
            pMemory);
}

static void *
dkdHandleDekoiAlignedAllocation(void *pData, DkSize size, DkSize alignment)
{
    assert(pData != NULL);

    return ((DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnAllocateAligned(
            ((DkdDekoiAllocationCallbacksData *)pData)->pAllocator->pData,
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

    return ((DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnReallocateAligned(
            ((DkdDekoiAllocationCallbacksData *)pData)->pAllocator->pData,
            pOriginal,
            (size_t)size,
            (size_t)alignment);
}

static void
dkdHandleDekoiAlignedFreeing(void *pData, void *pMemory)
{
    assert(pData != NULL);

    ((DkdDekoiAllocationCallbacksData *)pData)
        ->pAllocator->pfnFreeAligned(
            ((DkdDekoiAllocationCallbacksData *)pData)->pAllocator->pData,
            pMemory);
}

void
dkdGetDefaultAllocator(const DkdAllocationCallbacks **ppAllocator)
{
    assert(ppAllocator != NULL);

    *ppAllocator = &dkdDefaultAllocator;
}

int
dkdCreateDekoiAllocationCallbacks(DkdDekoiAllocationCallbacksData *pData,
                                  const DkdAllocationCallbacks *pAllocator,
                                  const DkdLoggingCallbacks *pLogger,
                                  DkAllocationCallbacks **ppDekoiAllocator)
{
    assert(pData != NULL);
    assert(pAllocator != NULL);
    assert(pLogger != NULL);
    assert(ppDekoiAllocator != NULL);

    *ppDekoiAllocator = (DkAllocationCallbacks *)DKD_ALLOCATE(
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
dkdDestroyDekoiAllocationCallbacks(DkAllocationCallbacks *pDekoiAllocator,
                                   const DkdAllocationCallbacks *pAllocator)
{
    DKD_FREE(pAllocator, pDekoiAllocator);
}
