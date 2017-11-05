#ifndef DEKOI_DEKOI_H
#define DEKOI_DEKOI_H

#define DK_NAME "dekoi"
#define DK_MAJOR_VERSION 0
#define DK_MINOR_VERSION 1
#define DK_PATCH_VERSION 0

#if defined(DEBUG) || !defined(NDEBUG)
 #define DK_DEBUG
#endif

#if defined(__GNUC__) || defined(__clang__)
 #define DK_C_EXTENSION __extension__
#else
 #define DK_C_EXTENSION
#endif

#define DK_UNUSED(x) (void)(x)
#define DK_STATIC_ASSERT(x, msg) \
    typedef char dk_static_assertion_failed_##msg[(x) ? 1 : -1]
#define DK_ALLOCATE(pAllocator, size) \
    (pAllocator->pfnAllocate(pAllocator->pContext, size))
#define DK_FREE(pAllocator, pMemory) \
    (pAllocator->pfnFree(pAllocator->pContext, pMemory))

#if defined(__ANDROID__)
 #define DK_PLATFORM_ANDROID
#elif defined(__APPLE__) && defined(__MACH__)
 #if TARGET_OS_IPHONE == 1
  #define DK_PLATFORM_IOS
 #elif TARGET_OS_MAC == 1
  #define DK_PLATFORM_MACOS
 #else
  DK_STATIC_ASSERT(0, apple_platform_not_supported);
  #define DK_PLATFORM_INVALID
 #endif
#elif defined(__linux__)
 #define DK_PLATFORM_LINUX
#elif defined(_WIN32)
 #define DK_PLATFORM_WINDOWS
#else
 DK_STATIC_ASSERT(0, platform_not_supported);
 #define DK_PLATFORM_INVALID
#endif

/*
   Focus on the common ILP32, LP64 and LLP64 data models.
   64-bit integer types aren't part of C89 but should be available anyways.
*/
#ifdef DK_USE_STD_FIXED_TYPES
 #include <stdint.h>
 #define DK_INT8 int8_t
 #define DK_UINT8 uint8_t
 #define DK_INT16 int16_t
 #define DK_UINT16 uint16_t
 #define DK_INT32 int32_t
 #define DK_UINT32 uint32_t
 #define DK_INT64 int64_t
 #define DK_UINT64 uint64_t
#else
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
  #if defined(_MSC_VER)
   #define DK_INT32 __int32
  #else
   #define DK_INT32 int
  #endif
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
#endif

#ifdef DK_USE_STD_BASIC_TYPES
 #include <stddef.h>
 #define DK_SIZE_T size_t
#else
 #if defined(__GNUC__) || defined(__clang__)
  #if !defined(__x86_64__) && !defined(__ppc64__)
   #define DK_SIZE_T unsigned int
  #else
   #define DK_SIZE_T unsigned long
  #endif
 #elif defined(_MSC_VER)
  #if !defined(_WIN64)
   #define DK_SIZE_T unsigned __int32
  #else
   #define DK_SIZE_T unsigned __int64
  #endif
 #else
  #define DK_SIZE_T unsigned long
 #endif
#endif

typedef DK_INT32 DkBool32;
typedef DK_INT8 DkInt8;
typedef DK_UINT8 DkUint8;
typedef DK_INT16 DkInt16;
typedef DK_UINT16 DkUint16;
typedef DK_INT32 DkInt32;
typedef DK_UINT32 DkUint32;
DK_C_EXTENSION typedef DK_INT64 DkInt64;
DK_C_EXTENSION typedef DK_UINT64 DkUint64;
typedef DK_SIZE_T DkSize;

DK_STATIC_ASSERT(sizeof(DkBool32) == 4, invalid_bool32_type);
DK_STATIC_ASSERT(sizeof(DkInt8) == 1, invalid_int8_type);
DK_STATIC_ASSERT(sizeof(DkUint8) == 1, invalid_uint8_type);
DK_STATIC_ASSERT(sizeof(DkInt16) == 2, invalid_int16_type);
DK_STATIC_ASSERT(sizeof(DkUint16) == 2, invalid_uint16_type);
DK_STATIC_ASSERT(sizeof(DkInt32) == 4, invalid_int32_type);
DK_STATIC_ASSERT(sizeof(DkUint32) == 4, invalid_uint32_type);
DK_STATIC_ASSERT(sizeof(DkInt64) == 8, invalid_int64_type);
DK_STATIC_ASSERT(sizeof(DkUint64) == 8, invalid_uint64_type);
DK_STATIC_ASSERT(sizeof(DkSize) == sizeof(void *), invalid_size_type);

#define DK_FALSE ((DkBool32) 0)
#define DK_TRUE ((DkBool32) 1)

typedef struct DkAllocator DkAllocator;
typedef struct DkRenderer DkRenderer;
typedef struct DkRendererCreateInfo DkRendererCreateInfo;
typedef struct DkWindowManagerInterface DkWindowManagerInterface;

typedef enum DkResult {
    DK_SUCCESS = 0,
    DK_ERROR = -1,
    DK_ERROR_ALLOCATION = -2
} DkResult;

const char * dkGetResultString(DkResult result);

#endif /* DEKOI_DEKOI_H */
