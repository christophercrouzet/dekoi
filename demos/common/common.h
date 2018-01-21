#ifndef DEKOI_DEMOS_COMMON_COMMON_H
#define DEKOI_DEMOS_COMMON_COMMON_H

#define DKD_STATIC_ASSERT(x, msg) typedef char dkd_##msg[(x) ? 1 : -1]

#define DKD_UNUSED(x) (void)(x)

typedef struct DkdAllocationCallbacks DkdAllocationCallbacks;
typedef struct DkdApplication DkdApplication;
typedef struct DkdApplicationCallbacks DkdApplicationCallbacks;
typedef struct DkdApplicationCreateInfo DkdApplicationCreateInfo;
typedef struct DkdBootstrapCreateInfos DkdBootstrapCreateInfos;
typedef struct DkdBootstrapHandles DkdBootstrapHandles;
typedef struct DkdDekoiAllocationCallbacksData DkdDekoiAllocationCallbacksData;
typedef struct DkdDekoiLoggingCallbacksData DkdDekoiLoggingCallbacksData;
typedef struct DkdLoggingCallbacks DkdLoggingCallbacks;
typedef struct DkdRenderer DkdRenderer;
typedef struct DkdRendererCreateInfo DkdRendererCreateInfo;
typedef struct DkdShaderCreateInfo DkdShaderCreateInfo;
typedef struct DkdWindow DkdWindow;
typedef struct DkdWindowCreateInfo DkdWindowCreateInfo;

#endif /* DEKOI_DEMOS_COMMON_COMMON_H */
