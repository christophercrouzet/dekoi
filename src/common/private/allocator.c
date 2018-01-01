#include "allocator.h"

#include "assert.h"
#include "common.h"

#include "../allocator.h"
#include "../common.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef DK_ALLOCATOR_DEBUGGING
#define DKP_ALLOCATOR_DEBUGGING DKP_DEBUGGING
#elif DK_ALLOCATOR_DEBUGGING
#define DKP_ALLOCATOR_DEBUGGING 1
#else
#define DKP_ALLOCATOR_DEBUGGING 0
#endif /* DK_ALLOCATOR_DEBUGGING */

#ifdef __cplusplus
template<typename T>
struct DkpAlignmentOfHelper {
    char first;
    T second;
};
#define DKP_ALIGNMENT_OF(type) offsetof(DkpAlignmentOfHelper<type>, second)
#else
#define DKP_ALIGNMENT_OF(type)                                                 \
    offsetof(                                                                  \
        struct {                                                               \
            char first;                                                        \
            type second;                                                       \
        },                                                                     \
        second)
#endif /* __cplusplus */

#define DKP_IS_POWER_OF_TWO(x)                                                 \
    ((x) == 1 || (x) == 2 || (x) == 4 || (x) == 8 || (x) == 16 || (x) == 32    \
     || (x) == 64 || (x) == 128 || (x) == 256 || (x) == 512 || (x) == 1024     \
     || (x) == 2048 || (x) == 4096 || (x) == 8192 || (x) == 16384              \
     || (x) == 32768 || (x) == 65536 || (x) == 131072 || (x) == 262144         \
     || (x) == 524288 || (x) == 1048576 || (x) == 2097152 || (x) == 4194304    \
     || (x) == 8388608 || (x) == 16777216 || (x) == 33554432                   \
     || (x) == 67108864 || (x) == 134217728 || (x) == 268435456                \
     || (x) == 1073741824 || (x) == 2147483648 || (x) == 4294967296)

/*
   The aligned allocator works by allocating a block of larger size than
   requested to hold some padding space to secure the desired alignment
   as well as some necessary bookkeeping.

     block           user pointer
      /                  /
     +---------+--------+------+---------+
     | padding | header | size | padding |
     +---------+--------+------+---------+
      \                  \
       0                offset

   The sum of the front and back paddings equals `alignment - 1`, and the
   pointer returned to the user is located at a distance of `offset` from
   the beginning of the block.
*/

typedef struct DkpAlignedBlockHeader {
    ptrdiff_t offset;
    size_t size;
#if DKP_ALLOCATOR_DEBUGGING
    size_t alignment;
#endif /* DKP_ALLOCATOR_DEBUGGING */
} DkpAlignedBlockHeader;

DKP_STATIC_ASSERT(DKP_IS_POWER_OF_TWO(DKP_ALIGNMENT_OF(DkpAlignedBlockHeader)),
                  invalid_block_header_alignment);
DKP_STATIC_ASSERT(DKP_IS_POWER_OF_TWO(sizeof(void *)),
                  invalid_void_pointer_alignment);

/*
   Any power of two alignment requested for the user pointer that is greater or
   equal to this minimum value is guaranteed to be a multiple of
   `sizeof(void *)`, thus conforming to the requirement of `posix_memalign()`,
   and is also guaranteed to provide correct alignment for the block header.
*/
static const size_t dkpMinAlignment
    = DKP_ALIGNMENT_OF(DkpAlignedBlockHeader) > sizeof(void *)
          ? DKP_ALIGNMENT_OF(DkpAlignedBlockHeader)
          : sizeof(void *);

#if DKP_DEBUGGING
static int
dkpIsPowerOfTwo(DkSize x)
{
    /* Complement and compare approach. */
    return (x != 0) && ((x & (~x + 1)) == x);
}
#endif /* DKP_DEBUGGING */

static void *
dkpAllocate(void *pData, DkSize size)
{
    DKP_UNUSED(pData);
    return malloc((size_t)size);
}

static void *
dkpReallocate(void *pData, void *pOriginal, DkSize size)
{
    DKP_UNUSED(pData);
    return realloc(pOriginal, (size_t)size);
}

static void
dkpFree(void *pData, void *pMemory)
{
    DKP_UNUSED(pData);
    free(pMemory);
}

static void *
dkpAllocateAligned(void *pData, DkSize size, DkSize alignment)
{
    void *pOut;
    void *pBlock;
    DkpAlignedBlockHeader *pHeader;

    DKP_UNUSED(pData);

    DKP_ASSERT(size != 0);
    DKP_ASSERT(dkpIsPowerOfTwo(alignment));

    if (alignment < dkpMinAlignment) {
        alignment = (DkSize)dkpMinAlignment;
    }

    pBlock = malloc(
        (size_t)(size + alignment - 1 + sizeof(DkpAlignedBlockHeader)));
    if (pBlock == NULL) {
        return NULL;
    }

    pOut = (void *)((uintptr_t)((unsigned char *)pBlock + alignment - 1
                                + sizeof(DkpAlignedBlockHeader))
                    & ~(uintptr_t)(alignment - 1));

    pHeader = &((DkpAlignedBlockHeader *)pOut)[-1];
    pHeader->offset = (unsigned char *)pOut - (unsigned char *)pBlock;
    pHeader->size = size;
#if DKP_ALLOCATOR_DEBUGGING
    pHeader->alignment = (size_t)alignment;
#endif /* DKP_ALLOCATOR_DEBUGGING */

    return pOut;
}

static void *
dkpReallocateAligned(void *pData,
                     void *pOriginal,
                     DkSize size,
                     DkSize alignment)
{
    DkpAlignedBlockHeader originalHeader;
    void *pOriginalBlock;
    void *pBlock;

    DKP_ASSERT(pOriginal != NULL);
    DKP_ASSERT(size != 0);
    DKP_ASSERT(dkpIsPowerOfTwo(alignment));

    DKP_UNUSED(pData);

    if (alignment < dkpMinAlignment) {
        alignment = (DkSize)dkpMinAlignment;
    }

    originalHeader = ((DkpAlignedBlockHeader *)pOriginal)[-1];
#if DKP_ALLOCATOR_DEBUGGING
    DKP_ASSERT(alignment == originalHeader.alignment);
#endif /* DKP_ALLOCATOR_DEBUGGING */

    pOriginalBlock
        = (void *)((unsigned char *)pOriginal - originalHeader.offset);
    pBlock = realloc(
        pOriginalBlock,
        (size_t)(size + alignment - 1 + sizeof(DkpAlignedBlockHeader)));
    if (pBlock == NULL) {
        return NULL;
    }

    if (pBlock == pOriginalBlock) {
        /* `realloc()` expanded the block in place. */
        ((DkpAlignedBlockHeader *)pOriginal)[-1].size = size;
        return pOriginal;
    }

    {
        void *pOut;
        DkpAlignedBlockHeader *pHeader;

        pOut = (void *)((uintptr_t)((unsigned char *)pBlock + alignment - 1
                                    + sizeof(DkpAlignedBlockHeader))
                        & ~(uintptr_t)(alignment - 1));

        pHeader = &((DkpAlignedBlockHeader *)pOut)[-1];
        pHeader->offset = (unsigned char *)pOut - (unsigned char *)pBlock;
        pHeader->size = size;

        if (pHeader->offset == originalHeader.offset) {
            /*
               `realloc()` allocated a new block that is still correctly
               aligned.
            */
            return pOut;
        }

        memmove(pOut,
                (void *)((unsigned char *)pBlock + originalHeader.offset),
                originalHeader.size);

        return pOut;
    }
}

static void
dkpFreeAligned(void *pData, void *pMemory)
{
    DkpAlignedBlockHeader *pHeader;

    DKP_UNUSED(pData);

    DKP_ASSERT(pMemory != NULL);

    pHeader = &((DkpAlignedBlockHeader *)pMemory)[-1];
    free((void *)((unsigned char *)pMemory - pHeader->offset));
}

static const DkAllocationCallbacks dkpDefaultAllocator = {NULL,
                                                          dkpAllocate,
                                                          dkpReallocate,
                                                          dkpFree,
                                                          dkpAllocateAligned,
                                                          dkpReallocateAligned,
                                                          dkpFreeAligned};

void
dkpGetDefaultAllocator(const DkAllocationCallbacks **ppAllocator)
{
    DKP_ASSERT(ppAllocator != NULL);

    *ppAllocator = &dkpDefaultAllocator;
}
