#ifndef DEKOI_COMMON_COMMON_H
#define DEKOI_COMMON_COMMON_H

#define DK_NAME "dekoi"
#define DK_MAJOR_VERSION 0
#define DK_MINOR_VERSION 1
#define DK_PATCH_VERSION 0

#define DKP_STATIC_ASSERT(x, msg) typedef char dkp_##msg[(x) ? 1 : -1]

#if defined(__x86_64__) || defined(_M_X64)
#define DKP_ARCH_X86_64
#elif defined(__i386) || defined(_M_IX86)
#define DKP_ARCH_X86_32
#elif defined(__itanium__) || defined(_M_IA64)
#define DKP_ARCH_ITANIUM
#elif defined(__powerpc64__) || defined(__ppc64__)
#define DKP_ARCH_POWERPC_64
#elif defined(__powerpc__) || defined(__ppc__)
#define DKP_ARCH_POWERPC_32
#elif defined(__aarch64__)
#define DKP_ARCH_ARM_64
#elif defined(__arm__)
#define DKP_ARCH_ARM_32
#endif

/*
   The environment macro represents whether the code is to be generated for a
   32-bit or 64-bit target platform. Some CPUs, such as the x86-64 processors,
   allow generating code for 32-bit environments using the -m32 or -mx32
   compiler switches, in which case `DK_ENVIRONMENT` is set to 32.
*/
#ifndef DK_ENVIRONMENT
#if (!defined(DKP_ARCH_X86_64) || defined(__ILP32__))                          \
    && !defined(DKP_ARCH_ITANIUM) && !defined(DKP_ARCH_POWERPC_64)             \
    && !defined(DKP_ARCH_ARM_64)
#define DK_ENVIRONMENT 32
#else
#define DK_ENVIRONMENT 64
#endif
#endif

DKP_STATIC_ASSERT(DK_ENVIRONMENT == 32 || DK_ENVIRONMENT == 64,
                  invalid_environment_value);

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
/*
   The focus here is on the common data models, that is ILP32 (most recent
   32-bit systems), LP64 (Linux, macOS, ARM), and LLP64 (Windows). All of these
   models have the `int` type set to 32 bits, and `long long` to 64 bits.
*/
typedef char DkInt8;
typedef unsigned char DkUint8;
typedef short DkInt16;
typedef unsigned short DkUint16;
typedef int DkInt32;
typedef unsigned int DkUint32;
typedef long long DkInt64;
typedef unsigned long long DkUint64;
#endif

#ifdef DK_USE_STD_BASIC_TYPES
#include <stddef.h>
typedef size_t DkSize;
#else
/*
   The C standard provides no guarantees about the size of the type `size_t`,
   and some exotic platforms will in fact provide original values, but this
   should cover all of the use cases falling in the scope of this project.
*/
#if DK_ENVIRONMENT == 32
typedef DkUint32 DkSize;
#else
typedef DkUint64 DkSize;
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
DKP_STATIC_ASSERT(sizeof(DkSize) == sizeof sizeof(void *), invalid_size_type);
DKP_STATIC_ASSERT(sizeof(DkFloat32) == 4, invalid_float32_type);
DKP_STATIC_ASSERT(sizeof(DkFloat64) == 8, invalid_float64_type);
DKP_STATIC_ASSERT(sizeof(DkBool32) == 4, invalid_bool32_type);

#define DK_FALSE ((DkBool32)0)
#define DK_TRUE ((DkBool32)1)

typedef struct DkAllocationCallbacks DkAllocationCallbacks;
typedef struct DkLoggingCallbacks DkLoggingCallbacks;

typedef enum DkResult {
    DK_SUCCESS = 0,
    DK_ERROR = -1,
    DK_ERROR_INVALID_VALUE = -2,
    DK_ERROR_ALLOCATION = -3,
    DK_ERROR_NOT_AVAILABLE = -4
} DkResult;

const char *
dkGetResultString(DkResult result);

#endif /* DEKOI_COMMON_COMMON_H */
