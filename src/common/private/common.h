#ifndef DEKOI_COMMON_PRIVATE_COMMON_H
#define DEKOI_COMMON_PRIVATE_COMMON_H

#if (defined(DK_DEBUGGING) && DK_DEBUGGING != 0)                               \
    || (!defined(DK_DEBUGGING) && (defined(DEBUG) || !defined(NDEBUG)))
#define DKP_DEBUGGING 1
#else
#define DKP_DEBUGGING 0
#endif

#define DKP_UNUSED(x) (void)(x)

#define DKP_GET_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DKP_FALSE ((int)0)
#define DKP_TRUE ((int)1)

#endif /* DEKOI_COMMON_PRIVATE_COMMON_H */
