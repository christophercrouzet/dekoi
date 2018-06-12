#ifndef DEKOI_DEMOS_COMMON_COMMON_H
#define DEKOI_DEMOS_COMMON_COMMON_H

#define DKD_STATIC_ASSERT(x, msg) typedef char dkd_##msg[(x) ? 1 : -1]

#define DKD_UNUSED(x) (void)(x)

#define DKD_GET_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif /* DEKOI_DEMOS_COMMON_COMMON_H */
