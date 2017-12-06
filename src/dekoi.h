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
#define DKP_C_EXTENSION __extension__
#else
#define DKP_C_EXTENSION
#endif

#define DKP_STATIC_ASSERT(x, msg) \
    typedef char DKP_STATIC_ASSERTION_failed_##msg[(x) ? 1 : -1]

/*
   Focus on the common ILP32, LP64 and LLP64 data models.
   64-bit integer types aren't part of C89 but should be available anyways.
*/
#ifdef DK_USE_STD_FIXED_TYPES
#include <stdint.h>
typedef int8_t DkInt8;
typedef uint8_t DkUint8;
typedef int16_t DkInt16;
typedef uint16_t DkUint16;
typedef int32_t DkInt32;
typedef uint32_t DkUint32;
typedef int64_t DkInt64;
typedef uint64_t DkUint64;
#else
typedef char DkInt8;
typedef unsigned char DkUint8;
typedef short DkInt16;
typedef unsigned short DkUint16;
#if defined(_MSC_VER)
typedef __int32 DkInt32;
typedef unsigned __int32 DkUint32;
#else
typedef int DkInt32;
typedef unsigned int DkUint32;
#endif
#if defined(_MSC_VER)
typedef __int64 DkInt64;
typedef unsigned __int64 DkUint64;
#else
DKP_C_EXTENSION typedef long long DkInt64;
DKP_C_EXTENSION typedef unsigned long long DkUint64;
#endif
#endif

#ifdef DK_USE_STD_BASIC_TYPES
#include <stddef.h>
typedef size_t DkSize;
#else
#if defined(__GNUC__) || defined(__clang__)
#if !defined(__x86_64__) && !defined(__ppc64__)
typedef unsigned int DkSize;
#else
typedef unsigned long DkSize;
#endif
#elif defined(_MSC_VER)
#if !defined(_WIN64)
typedef __int32 DkSize;
#else
typedef __int64 DkSize;
#endif
#else
typedef unsigned long DkSize;
#endif
#endif

typedef float DkFloat32;
typedef double DkFloat64;
typedef DkInt32 DkBool32;

DKP_STATIC_ASSERT(sizeof(DkInt8) == 1, invalid_int8_type);
DKP_STATIC_ASSERT(sizeof(DkUint8) == 1, invalid_uint8_type);
DKP_STATIC_ASSERT(sizeof(DkInt16) == 2, invalid_int16_type);
DKP_STATIC_ASSERT(sizeof(DkUint16) == 2, invalid_uint16_type);
DKP_STATIC_ASSERT(sizeof(DkInt32) == 4, invalid_int32_type);
DKP_STATIC_ASSERT(sizeof(DkUint32) == 4, invalid_uint32_type);
DKP_STATIC_ASSERT(sizeof(DkInt64) == 8, invalid_int64_type);
DKP_STATIC_ASSERT(sizeof(DkUint64) == 8, invalid_uint64_type);
DKP_STATIC_ASSERT(sizeof(DkSize) == sizeof(void *), invalid_size_type);
DKP_STATIC_ASSERT(sizeof(DkFloat32) == 4, invalid_float32_type);
DKP_STATIC_ASSERT(sizeof(DkFloat64) == 8, invalid_float64_type);
DKP_STATIC_ASSERT(sizeof(DkBool32) == 4, invalid_bool32_type);

#define DK_FALSE ((DkBool32)0)
#define DK_TRUE ((DkBool32)1)

typedef struct DkAllocator DkAllocator;
typedef struct DkRenderer DkRenderer;
typedef struct DkRendererCreateInfo DkRendererCreateInfo;
typedef struct DkShaderCreateInfo DkShaderCreateInfo;
typedef struct DkWindowCallbacks DkWindowCallbacks;

typedef enum DkResult {
    DK_SUCCESS = 0,
    DK_ERROR = -1,
    DK_ERROR_INVALID_VALUE = -2,
    DK_ERROR_ALLOCATION = -3,
    DK_ERROR_NOT_AVAILABLE = -4
} DkResult;

const char *
dkGetResultString(DkResult result);

#endif /* DEKOI_DEKOI_H */
