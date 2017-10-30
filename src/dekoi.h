#ifndef DEKOI_DEKOI_H
#define DEKOI_DEKOI_H

#if defined(DEBUG) || !defined(NDEBUG)
 #define DK_DEBUG
#endif

#define DK_UNUSED(x) (void)(x)
#define DK_STATIC_ASSERT(x, msg) \
    typedef char dk_static_assertion_failed_##msg[(x) ? 1 : -1]

/*
   Focus on the common ILP32, LP64 and LLP64 data models.
   64-bit integer types aren't part of C89 but should be available anyways.
*/
#ifndef DK_INT8
 #define DK_INT8 char
#endif
#ifndef DK_UINT8
 #define DK_UINT8 unsigned DK_INT8
#endif
#ifndef DK_INT16
 #define DK_INT16 short
#endif
#ifndef DK_UINT16
 #define DK_UINT16 unsigned DK_INT16
#endif
#ifndef DK_INT32
 #define DK_INT32 int
#endif
#ifndef DK_UINT32
 #define DK_UINT32 unsigned DK_INT32
#endif
#ifndef DK_INT64
 #if defined(_MSC_VER)
  #define DK_INT64 __int64
 #else
  #define DK_INT64 long long
 #endif
#endif
#ifndef DK_UINT64
 #define DK_UINT64 unsigned DK_INT64
#endif

#if defined(__GNUC__) || defined(__clang__)
 #if defined(__x86_64__) || defined(__ppc64__)
  #define DK_SIZE_T unsigned long
 #else
  #define DK_SIZE_T unsigned int
 #endif
#elif defined(_MSC_VER)
 #if defined(_WIN64)
  #define DK_SIZE_T __int64
 #else
  #define DK_SIZE_T __int32
 #endif
#else
 #define DK_SIZE_T unsigned long
#endif

typedef DK_INT32 DkBool32;
typedef DK_INT8 DkInt8;
typedef DK_UINT8 DkUint8;
typedef DK_INT16 DkInt16;
typedef DK_UINT16 DkUint16;
typedef DK_INT32 DkInt32;
typedef DK_UINT32 DkUint32;
typedef DK_SIZE_T DkSize;

DK_STATIC_ASSERT(sizeof(DkBool32) == 4, invalid_bool32_type);
DK_STATIC_ASSERT(sizeof(DkInt8) == 1, invalid_int8_type);
DK_STATIC_ASSERT(sizeof(DkUint8) == 1, invalid_uint8_type);
DK_STATIC_ASSERT(sizeof(DkInt16) == 2, invalid_int16_type);
DK_STATIC_ASSERT(sizeof(DkUint16) == 2, invalid_uint16_type);
DK_STATIC_ASSERT(sizeof(DkInt32) == 4, invalid_int32_type);
DK_STATIC_ASSERT(sizeof(DkUint32) == 4, invalid_uint32_type);
DK_STATIC_ASSERT(sizeof(DkSize) == sizeof(void *), invalid_size_type);

typedef struct DkAllocator DkAllocator;
typedef struct DkRenderer DkRenderer;
typedef struct DkRendererCreateInfo DkRendererCreateInfo;

typedef enum DkResult {
    DK_SUCCESS = 0
} DkResult;

#endif /* DEKOI_DEKOI_H */
