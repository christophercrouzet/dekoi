#include "allocator.h"

#include "../common/allocator.h"
#include "../common/common.h"

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

typedef struct DkdBlockHeader {
    size_t size;
} DkdBlockHeader;

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
    void *pOut;
    void *pBlock;
    DkdBlockHeader *pHeader;

    assert(size != 0);

    pBlock = malloc(size + sizeof(DkdBlockHeader));
    if (pBlock == NULL) {
        return NULL;
    }

    pOut = (void *)((unsigned char *)pBlock + sizeof(DkdBlockHeader));

    pHeader = &((DkdBlockHeader *)pOut)[-1];
    pHeader->size = size;

    ((DkdAllocationCallbacksData *)pData)->used += size;
    return pOut;
}

static void
dkdFree(void *pData, void *pMemory)
{
    DkdBlockHeader *pHeader;

    assert(pMemory != NULL);

    pHeader = &((DkdBlockHeader *)pMemory)[-1];
    ((DkdAllocationCallbacksData *)pData)->used -= pHeader->size;
    free((void *)((unsigned char *)pMemory - sizeof(DkdBlockHeader)));
}

static void *
dkdReallocate(void *pData, void *pOriginal, size_t size)
{
    void *pOut;
    DkdBlockHeader *pHeader;

    pHeader = &((DkdBlockHeader *)pOriginal)[-1];
    if (size <= pHeader->size) {
        pOut = pOriginal;
        goto exit;
    }

    pOut = dkdAllocate(pData, size);
    if (pOut == NULL) {
        goto original_cleanup;
    }

    memcpy(pOut, pOriginal, pHeader->size);
    goto cleanup;

cleanup:;

original_cleanup:
    dkdFree(pData, pOriginal);

exit:
    return pOut;
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

    ((DkdAllocationCallbacksData *)pData)->used += size;
    return pOut;
}

static void
dkdFreeAligned(void *pData, void *pMemory)
{
    DkdAlignedBlockHeader *pHeader;

    DKD_UNUSED(pData);

    assert(pMemory != NULL);

    pHeader = &((DkdAlignedBlockHeader *)pMemory)[-1];
    ((DkdAllocationCallbacksData *)pData)->used -= pHeader->size;
    free((void *)((unsigned char *)pMemory - pHeader->offset));
}

static void *
dkdReallocateAligned(void *pData,
                     void *pOriginal,
                     size_t size,
                     size_t alignment)
{
    void *pOut;
    DkdAlignedBlockHeader *pHeader;

    assert(pOriginal != NULL);
    assert(size != 0);
    assert(dkdIsPowerOfTwo(alignment));

    DKD_UNUSED(pData);

    pHeader = &((DkdAlignedBlockHeader *)pOriginal)[-1];
    if (size <= pHeader->size) {
        pOut = pOriginal;
        goto exit;
    }

    pOut = dkdAllocateAligned(pData, size, alignment);
    if (pOut == NULL) {
        goto original_cleanup;
    }

    memcpy(pOut, pOriginal, pHeader->size);
    goto cleanup;

cleanup:;

original_cleanup:
    dkdFreeAligned(pData, pOriginal);

exit:
    return pOut;
}

int
dkdCreateCustomAllocator(DkdAllocationCallbacks **ppAllocator)
{
    assert(ppAllocator != NULL);

    *ppAllocator = (DkdAllocationCallbacks *)malloc(sizeof **ppAllocator);
    if (*ppAllocator == NULL) {
        return 1;
    }

    (*ppAllocator)->pData = (DkdAllocationCallbacksData *)malloc(
        sizeof(DkdAllocationCallbacksData));
    if ((*ppAllocator)->pData == NULL) {
        return 1;
    }

    (*ppAllocator)->pfnAllocate = dkdAllocate;
    (*ppAllocator)->pfnReallocate = dkdReallocate;
    (*ppAllocator)->pfnFree = dkdFree;
    (*ppAllocator)->pfnAllocateAligned = dkdAllocateAligned;
    (*ppAllocator)->pfnReallocateAligned = dkdReallocateAligned;
    (*ppAllocator)->pfnFreeAligned = dkdFreeAligned;
    return 0;
}

void
dkdDestroyCustomAllocator(DkdAllocationCallbacks *pAllocator)
{
    assert(pAllocator != NULL);

    free(pAllocator->pData);
    free(pAllocator);
}
