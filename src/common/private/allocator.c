#include "allocator.h"

#include "assert.h"
#include "common.h"

#include "../allocator.h"
#include "../common.h"

#include <stddef.h>

#define ZR_SPECIFY_INTERNAL_LINKAGE
#define ZR_DEFINE_IMPLEMENTATION
#define ZR_ASSERT DKP_ASSERT
#define ZR_SIZE_TYPE DkSize
#include <zero/allocator.h>

#ifndef DK_ALLOCATOR_DEBUGGING
#define ZR_ALLOCATOR_DEBUGGING DKP_DEBUGGING
#elif DK_ALLOCATOR_DEBUGGING
#define ZR_ALLOCATOR_DEBUGGING 1
#else
#define ZR_ALLOCATOR_DEBUGGING 0
#endif /* DK_ALLOCATOR_DEBUGGING */

#ifndef NDEBUG
static int
dkdIsPowerOfTwo(size_t x)
{
    /* Complement and compare approach. */
    return (x != 0) && ((x & (~x + 1)) == x);
}
#endif /* NDEBUG */

static void *
dkpAllocate(void *pData, DkSize size)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(size != 0);

    return zrAllocate((ZrSize)size);
}

static void *
dkpReallocate(void *pData, void *pOriginal, DkSize size)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(pOriginal != NULL);
    DKP_ASSERT(size != 0);

    return zrReallocate(pOriginal, (ZrSize)size);
}

static void
dkpFree(void *pData, void *pMemory)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(pMemory != NULL);

    zrFree(pMemory);
}

static void *
dkpAllocateAligned(void *pData, DkSize size, DkSize alignment)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(size != 0);
    DKP_ASSERT(alignment != 0 && dkdIsPowerOfTwo(alignment));

    return zrAllocateAligned((ZrSize)size, (ZrSize)alignment);
}

static void *
dkpReallocateAligned(void *pData,
                     void *pOriginal,
                     DkSize size,
                     DkSize alignment)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(pOriginal != NULL);
    DKP_ASSERT(size != 0);
    DKP_ASSERT(alignment != 0 && dkdIsPowerOfTwo(alignment));

    return zrReallocateAligned(pOriginal, (ZrSize)size, (ZrSize)alignment);
}

static void
dkpFreeAligned(void *pData, void *pMemory)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(pMemory != NULL);

    zrFreeAligned(pMemory);
}

static const struct DkAllocationCallbacks dkpDefaultAllocator
    = {NULL,
       dkpAllocate,
       dkpReallocate,
       dkpFree,
       dkpAllocateAligned,
       dkpReallocateAligned,
       dkpFreeAligned};

void
dkpGetDefaultAllocator(const struct DkAllocationCallbacks **ppAllocator)
{
    DKP_ASSERT(ppAllocator != NULL);

    *ppAllocator = &dkpDefaultAllocator;
}
