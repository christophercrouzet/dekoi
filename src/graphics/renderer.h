#ifndef DEKOI_GRAPHICS_RENDERING_H
#define DEKOI_GRAPHICS_RENDERING_H

#include <dekoi/common/common.h>

enum DkShaderStage {
    DK_SHADER_STAGE_VERTEX = 0,
    DK_SHADER_STAGE_TESSELLATION_CONTROL = 1,
    DK_SHADER_STAGE_TESSELLATION_EVALUATION = 2,
    DK_SHADER_STAGE_GEOMETRY = 3,
    DK_SHADER_STAGE_FRAGMENT = 4,
    DK_SHADER_STAGE_COMPUTE = 5
};

enum DkVertexInputRate {
    DK_VERTEX_INPUT_RATE_VERTEX = 0,
    DK_VERTEX_INPUT_RATE_INSTANCE = 1
};

enum DkFormat { DK_FORMAT_R32G32_SFLOAT = 0, DK_FORMAT_R32G32B32_SFLOAT = 1 };

#define DKP_DEFINE_VULKAN_HANDLE(object) typedef struct object##_T *object

#if DK_ENVIRONMENT == 32
#define DKP_DEFINE_NON_DISPATCHABLE_VULKAN_HANDLE(object)                      \
    typedef DkUint64 object
#else
#define DKP_DEFINE_NON_DISPATCHABLE_VULKAN_HANDLE(object)                      \
    typedef struct object##_T *object
#endif /* DK_ENVIRONMENT */

typedef struct VkAllocationCallbacks VkAllocationCallbacks;
DKP_DEFINE_VULKAN_HANDLE(VkInstance);
DKP_DEFINE_NON_DISPATCHABLE_VULKAN_HANDLE(VkSurfaceKHR);

struct DkLoggingCallbacks;
struct DkRenderer;

typedef enum DkStatus (*DkPfnCreateInstanceExtensionNamesCallback)(
    DkUint32 *pExtensionCount,
    const char ***pppExtensionNames,
    void *pData,
    const struct DkLoggingCallbacks *pLogger);
typedef void (*DkPfnDestroyInstanceExtensionNamesCallback)(
    void *pData,
    const struct DkLoggingCallbacks *pLogger,
    const char **ppExtensionNames);
typedef enum DkStatus (*DkPfnCreateSurfaceCallback)(
    VkSurfaceKHR *pSurfaceHandle,
    void *pData,
    VkInstance instanceHandle,
    const VkAllocationCallbacks *pBackEndAllocator,
    const struct DkLoggingCallbacks *pLogger);

struct DkWindowSystemIntegrationCallbacks {
    void *pData;
    DkPfnCreateInstanceExtensionNamesCallback pfnCreateInstanceExtensionNames;
    DkPfnDestroyInstanceExtensionNamesCallback pfnDestroyInstanceExtensionNames;
    DkPfnCreateSurfaceCallback pfnCreateSurface;
};

struct DkShaderCreateInfo {
    enum DkShaderStage stage;
    DkSize codeSize;
    DkUint32 *pCode;
    const char *pEntryPointName;
};

struct DkVertexBufferCreateInfo {
    DkUint64 size;
    DkUint64 offset;
    const void *pData;
};

struct DkVertexBindingDescriptionCreateInfo {
    DkUint32 stride;
    enum DkVertexInputRate inputRate;
};

struct DkVertexAttributeDescriptionCreateInfo {
    DkUint32 binding;
    DkUint32 location;
    DkUint32 offset;
    enum DkFormat format;
};

struct DkRendererCreateInfo {
    const char *pApplicationName;
    DkUint32 applicationMajorVersion;
    DkUint32 applicationMinorVersion;
    DkUint32 applicationPatchVersion;
    DkUint32 surfaceWidth;
    DkUint32 surfaceHeight;
    const struct DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator;
    DkUint32 shaderCount;
    const struct DkShaderCreateInfo *pShaderInfos;
    DkFloat32 clearColor[4];
    DkUint32 vertexBufferCount;
    const struct DkVertexBufferCreateInfo *pVertexBufferInfos;
    DkUint32 vertexBindingDescriptionCount;
    const struct DkVertexBindingDescriptionCreateInfo
        *pVertexBindingDescriptionInfos;
    DkUint32 vertexAttributeDescriptionCount;
    const struct DkVertexAttributeDescriptionCreateInfo
        *pVertexAttributeDescriptionInfos;
    DkUint32 vertexCount;
    DkUint32 instanceCount;
    const struct DkLoggingCallbacks *pLogger;
    const struct DkAllocationCallbacks *pAllocator;
};

enum DkStatus
dkCreateRenderer(struct DkRenderer **ppRenderer,
                 const struct DkRendererCreateInfo *pCreateInfo);

void
dkDestroyRenderer(struct DkRenderer *pRenderer);

enum DkStatus
dkResizeRendererSurface(struct DkRenderer *pRenderer,
                        DkUint32 width,
                        DkUint32 height);

enum DkStatus
dkDrawRendererImage(struct DkRenderer *pRenderer);

#endif /* DEKOI_GRAPHICS_RENDERING_H */
