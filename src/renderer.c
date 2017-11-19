#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "memory.h"
#include "renderer.h"
#include "internal/assert.h"
#include "internal/dekoi.h"
#include "internal/io.h"
#include "internal/memory.h"


#ifdef DK_DEBUG
 #define DK_ENABLE_DEBUG_REPORT 1
 #define DK_ENABLE_VALIDATION_LAYERS 1
#else
 #define DK_ENABLE_DEBUG_REPORT 0
 #define DK_ENABLE_VALIDATION_LAYERS 0
#endif /* DK_DEBUG */


#define DKP_CLAMP(x, low, high) \
    (((x) > (high)) ? (high) : (x) < (low) ? (low) : (x))


typedef enum DkpLogging {
    DKP_LOGGING_DISABLED = 0,
    DKP_LOGGING_ENABLED = 1
} DkpLogging;


typedef enum DkpPresentSupport {
    DKP_PRESENT_SUPPORT_DISABLED = 0,
    DKP_PRESENT_SUPPORT_ENABLED = 1
} DkpPresentSupport;


typedef enum DkpSemaphoreId {
    DKP_SEMAPHORE_ID_IMAGE_ACQUIRED = 0,
    DKP_SEMAPHORE_ID_PRESENT_COMPLETED = 1,
    DKP_SEMAPHORE_ID_ENUM_LAST = DKP_SEMAPHORE_ID_PRESENT_COMPLETED,
    DKP_SEMAPHORE_ID_ENUM_COUNT = DKP_SEMAPHORE_ID_ENUM_LAST + 1
} DkpSemaphoreId;


typedef struct DkpQueueFamilyIndices {
    uint32_t graphics;
    uint32_t present;
} DkpQueueFamilyIndices;


typedef struct DkpDevice {
    DkpQueueFamilyIndices queueFamilyIndices;
    VkPhysicalDevice physicalHandle;
    VkDevice logicalHandle;
} DkpDevice;


typedef struct DkpSwapChainProperties {
    uint32_t minImageCount;
    VkExtent2D imageExtent;
    VkImageUsageFlags imageUsage;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
} DkpSwapChainProperties;


typedef struct DkpQueues {
    VkQueue graphicsHandle;
    VkQueue presentHandle;
} DkpQueues;


typedef struct DkpSwapChain {
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR format;
    uint32_t imageCount;
    VkImage *pImageHandles;
    VkImageView *pImageViewHandles;
} DkpSwapChain;


struct DkRenderer {
    const DkAllocator *pAllocator;
    const VkAllocationCallbacks *pBackEndAllocator;
    VkInstance instanceHandle;
    VkExtent2D surfaceExtent;
    VkSurfaceKHR surfaceHandle;
    VkDebugReportCallbackEXT debugReportCallbackHandle;
    DkpDevice device;
    DkpQueues queues;
    VkSemaphore *pSemaphoreHandles;
    DkpSwapChain swapChain;
    VkRenderPass renderPassHandle;
    VkPipelineLayout pipelineLayoutHandle;
    VkPipeline graphicsPipelineHandle;
    VkFramebuffer *pFramebufferHandles;
};


static const char *
dkpGetSemaphoreIdString(DkpSemaphoreId semaphoreId)
{
    switch (semaphoreId) {
        case DKP_SEMAPHORE_ID_IMAGE_ACQUIRED:
            return "image acquired";
        case DKP_SEMAPHORE_ID_PRESENT_COMPLETED:
            return "present completed";
        default:
            return "invalid";
    }
}


static int
dkpCheckShaderStage(DkShaderStage shaderStage)
{
    switch (shaderStage) {
        case DK_SHADER_STAGE_VERTEX:
        case DK_SHADER_STAGE_TESSELLATION_CONTROL:
        case DK_SHADER_STAGE_TESSELLATION_EVALUATION:
        case DK_SHADER_STAGE_GEOMETRY:
        case DK_SHADER_STAGE_FRAGMENT:
        case DK_SHADER_STAGE_COMPUTE:
            return DKP_TRUE;
        default:
            return DKP_FALSE;
    }
}


static VkShaderStageFlagBits
dkpTranslateShaderStage(DkShaderStage shaderStage)
{
    switch (shaderStage) {
        case DK_SHADER_STAGE_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case DK_SHADER_STAGE_TESSELLATION_CONTROL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case DK_SHADER_STAGE_TESSELLATION_EVALUATION:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case DK_SHADER_STAGE_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case DK_SHADER_STAGE_FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case DK_SHADER_STAGE_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            DK_ASSERT(1);
            return (VkShaderStageFlagBits) -1;
    }
}


static DkResult
dkpCreateInstanceLayerNames(const DkAllocator *pAllocator,
                            uint32_t *pLayerCount,
                            const char ***pppLayerNames)
{
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pLayerCount != NULL);
    DK_ASSERT(pppLayerNames != NULL);

    if (DK_ENABLE_VALIDATION_LAYERS) {
        *pLayerCount = 1;
        *pppLayerNames = (const char **)
            DK_ALLOCATE(pAllocator, sizeof **pppLayerNames * *pLayerCount);
        if (*pppLayerNames == NULL) {
            fprintf(stderr, "failed to allocate the instance layer names\n");
            return DK_ERROR_ALLOCATION;
        }

        (*pppLayerNames)[0] = "VK_LAYER_LUNARG_standard_validation";
    } else {
        *pLayerCount = 0;
        *pppLayerNames = NULL;
    }

    return DK_SUCCESS;
}


static void
dkpDestroyInstanceLayerNames(const char **ppLayerNames,
                             const DkAllocator *pAllocator)
{
    if (DK_ENABLE_VALIDATION_LAYERS)
        DK_FREE(pAllocator, ppLayerNames);
}


static DkResult
dkpCheckInstanceLayersSupport(uint32_t requiredLayerCount,
                              const char * const *ppRequiredLayerNames,
                              const DkAllocator *pAllocator,
                              int *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t layerCount;
    VkLayerProperties *pLayers;

    DK_ASSERT(ppRequiredLayerNames != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateInstanceLayerProperties(&layerCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of instance layer "
                        "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    pLayers = (VkLayerProperties *)
        DK_ALLOCATE(pAllocator, sizeof *pLayers * layerCount);
    if (pLayers == NULL) {
        fprintf(stderr, "failed to allocate the instance layer properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateInstanceLayerProperties(&layerCount, pLayers)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the instance layer properties "
                        "available\n");
        out = DK_ERROR;
        goto layers_cleanup;
    }

    *pSupported = DKP_FALSE;
    for (i = 0; i < requiredLayerCount; ++i) {
        int found;

        found = DKP_FALSE;
        for (j = 0; j < layerCount; ++j) {
            if (strcmp(pLayers[j].layerName, ppRequiredLayerNames[i]) == 0) {
                found = DKP_TRUE;
                break;
            }
        }

        if (!found)
            goto layers_cleanup;
    }

    *pSupported = DKP_TRUE;

layers_cleanup:
    DK_FREE(pAllocator, pLayers);

exit:
    return out;
}


static DkResult
dkpCreateInstanceExtensionNames(const DkWindowCallbacks *pWindowCallbacks,
                                const DkAllocator *pAllocator,
                                uint32_t *pExtensionCount,
                                const char ***pppExtensionNames)
{
    const char **ppBuffer;

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pExtensionCount != NULL);
    DK_ASSERT(pppExtensionNames != NULL);

    if (pWindowCallbacks == NULL) {
        *pExtensionCount = 0;
        *pppExtensionNames = NULL;
    } else if (pWindowCallbacks->pfnCreateInstanceExtensionNames(
                   pWindowCallbacks->pContext,
                   (uint32_t *) pExtensionCount,
                   pppExtensionNames)
               != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (DK_ENABLE_DEBUG_REPORT) {
        ppBuffer = (const char **)
            DK_ALLOCATE(pAllocator, sizeof *ppBuffer * (*pExtensionCount + 1));
        if (ppBuffer == NULL) {
            fprintf(stderr, "failed to allocate the instance extension "
                            "names\n");
            return DK_ERROR_ALLOCATION;
        }

        if (*pppExtensionNames != NULL)
            memcpy(ppBuffer, *pppExtensionNames,
                   sizeof *ppBuffer * *pExtensionCount);

        ppBuffer[*pExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

        if (pWindowCallbacks != NULL)
            pWindowCallbacks->pfnDestroyInstanceExtensionNames(
                pWindowCallbacks->pContext,
                *pppExtensionNames);

        *pExtensionCount += 1;
        *pppExtensionNames = ppBuffer;
    }

    return DK_SUCCESS;
}


static void
dkpDestroyInstanceExtensionNames(const char **ppExtensionNames,
                                 const DkWindowCallbacks *pWindowCallbacks,
                                 const DkAllocator *pAllocator)
{
    DK_ASSERT(pAllocator != NULL);

    if (DK_ENABLE_DEBUG_REPORT) {
        DK_FREE(pAllocator, ppExtensionNames);
    } else {
        if (pWindowCallbacks != NULL)
            pWindowCallbacks->pfnDestroyInstanceExtensionNames(
                pWindowCallbacks->pContext,
                ppExtensionNames);
    }
}


static DkResult
dkpCheckInstanceExtensionsSupport(uint32_t requiredExtensionCount,
                                  const char * const *ppRequiredExtensionNames,
                                  const DkAllocator *pAllocator,
                                  int *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t extensionCount;
    VkExtensionProperties *pExtensions;

    DK_ASSERT(ppRequiredExtensionNames != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of instance extension "
                        "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    pExtensions = (VkExtensionProperties *)
        DK_ALLOCATE(pAllocator, sizeof *pExtensions * extensionCount);
    if (pExtensions == NULL) {
        fprintf(stderr, "failed to allocate the instance extension "
                        "properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateInstanceExtensionProperties(NULL, &extensionCount,
                                               pExtensions)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the instance extension properties "
                        "available\n");
        out = DK_ERROR;
        goto extensions_cleanup;
    }

    *pSupported = DKP_FALSE;
    for (i = 0; i < requiredExtensionCount; ++i) {
        int found;

        found = DKP_FALSE;
        for (j = 0; j < extensionCount; ++j) {
            if (strcmp(pExtensions[j].extensionName,
                       ppRequiredExtensionNames[i])
                == 0)
            {
                found = DKP_TRUE;
                break;
            }
        }

        if (!found)
            goto extensions_cleanup;
    }

    *pSupported = DKP_TRUE;

extensions_cleanup:
    DK_FREE(pAllocator, pExtensions);

exit:
    return out;
}


static DkResult
dkpCreateInstance(const char *pApplicationName,
                  unsigned int applicationMajorVersion,
                  unsigned int applicationMinorVersion,
                  unsigned int applicationPatchVersion,
                  const DkWindowCallbacks *pWindowCallbacks,
                  const VkAllocationCallbacks *pBackEndAllocator,
                  const DkAllocator *pAllocator,
                  VkInstance *pInstanceHandle)
{
    DkResult out;
    uint32_t layerCount;
    const char **ppLayerNames;
    int layersSupported;
    uint32_t extensionCount;
    const char **ppExtensionNames;
    int extensionsSupported;
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo createInfo;

    DK_ASSERT(pApplicationName != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pInstanceHandle != NULL);

    out = DK_SUCCESS;

    if (dkpCreateInstanceLayerNames(pAllocator, &layerCount, &ppLayerNames)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto exit;
    }

    if (dkpCheckInstanceLayersSupport(layerCount, ppLayerNames, pAllocator,
                                      &layersSupported)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto layer_names_cleanup;
    }

    if (!layersSupported) {
        fprintf(stderr, "one or more instance layers are not supported\n");
        out = DK_ERROR;
        goto layer_names_cleanup;
    }

    if (dkpCreateInstanceExtensionNames(pWindowCallbacks, pAllocator,
                                        &extensionCount, &ppExtensionNames)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto layer_names_cleanup;
    }

    if (dkpCheckInstanceExtensionsSupport(extensionCount, ppExtensionNames,
                                          pAllocator, &extensionsSupported)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto extension_names_cleanup;
    }

    if (!extensionsSupported) {
        fprintf(stderr, "one or more instance extensions are not supported\n");
        out = DK_ERROR;
        goto extension_names_cleanup;
    }

    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = NULL;
    applicationInfo.pApplicationName = pApplicationName;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(
        applicationMajorVersion,
        applicationMinorVersion,
        applicationPatchVersion);
    applicationInfo.pEngineName = DK_NAME;
    applicationInfo.engineVersion = VK_MAKE_VERSION(
        DK_MAJOR_VERSION,
        DK_MINOR_VERSION,
        DK_PATCH_VERSION);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = ppLayerNames;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;

    switch (vkCreateInstance(&createInfo, pBackEndAllocator, pInstanceHandle)) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            fprintf(stderr, "some requested instance layers are not "
                            "supported\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            fprintf(stderr, "some requested instance extensions are not "
                            "supported\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            fprintf(stderr, "the driver is incompatible\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_INITIALIZATION_FAILED:
            fprintf(stderr, "failed to initialize the instance\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        default:
            fprintf(stderr, "failed to create the instance\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
    }

extension_names_cleanup:
    dkpDestroyInstanceExtensionNames(ppExtensionNames, pWindowCallbacks,
                                     pAllocator);

layer_names_cleanup:
    dkpDestroyInstanceLayerNames(ppLayerNames, pAllocator);

exit:
    return out;
}


static void
dkpDestroyInstance(VkInstance instanceHandle,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(instanceHandle != NULL);

    vkDestroyInstance(instanceHandle, pBackEndAllocator);
}


static VkBool32
dkpDebugReportCallback(VkDebugReportFlagsEXT flags,
                       VkDebugReportObjectTypeEXT objectType,
                       uint64_t object,
                       size_t location,
                       int32_t messageCode,
                       const char *pLayerPrefix,
                       const char *pMessage,
                       void *pUserData)
{
    DKP_UNUSED(flags);
    DKP_UNUSED(objectType);
    DKP_UNUSED(object);
    DKP_UNUSED(location);
    DKP_UNUSED(messageCode);
    DKP_UNUSED(pLayerPrefix);
    DKP_UNUSED(pUserData);

    fprintf(stderr, "validation layer: %s\n", pMessage);
    return VK_FALSE;
}


static DkResult
dkpCreateDebugReportCallback(VkInstance instanceHandle,
                             const VkAllocationCallbacks *pBackEndAllocator,
                             VkDebugReportCallbackEXT *pCallbackHandle)
{
    VkDebugReportCallbackCreateInfoEXT createInfo;
    PFN_vkCreateDebugReportCallbackEXT function;

    DK_ASSERT(instanceHandle != NULL);
    DK_ASSERT(pCallbackHandle != NULL);

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.pNext = NULL;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
                       | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = dkpDebugReportCallback;
    createInfo.pUserData = NULL;

    function = (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(instanceHandle,
                              "vkCreateDebugReportCallbackEXT");
    if (function == NULL) {
        fprintf(stderr, "could not retrieve the "
                        "'vkCreateDebugReportCallbackEXT' function\n");
        return DK_ERROR;
    }

    if (function(instanceHandle, &createInfo, pBackEndAllocator,
                 pCallbackHandle)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the debug report callback\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


static void
dkpDestroyDebugReportCallback(VkInstance instanceHandle,
                              VkDebugReportCallbackEXT callbackHandle,
                              const VkAllocationCallbacks *pBackEndAllocator)
{
    PFN_vkDestroyDebugReportCallbackEXT function;

    DK_ASSERT(instanceHandle != NULL);
    DK_ASSERT(callbackHandle != VK_NULL_HANDLE);

    function = (PFN_vkDestroyDebugReportCallbackEXT)
        vkGetInstanceProcAddr(instanceHandle,
                              "vkDestroyDebugReportCallbackEXT");
    if (function == NULL) {
        fprintf(stderr, "could not retrieve the "
                        "'vkDestroyDebugReportCallbackEXT' function\n");
        return;
    }

    function(instanceHandle, callbackHandle, pBackEndAllocator);
}


static DkResult
dkpCreateSurface(VkInstance instanceHandle,
                 const DkWindowCallbacks *pWindowCallbacks,
                 const VkAllocationCallbacks *pBackEndAllocator,
                 VkSurfaceKHR *pSurfaceHandle)
{
    DK_ASSERT(instanceHandle != NULL);
    DK_ASSERT(pWindowCallbacks != NULL);
    DK_ASSERT(pSurfaceHandle != NULL);

    if (pWindowCallbacks->pfnCreateSurface(pWindowCallbacks->pContext,
                                           instanceHandle,
                                           pBackEndAllocator,
                                           pSurfaceHandle)
             != DK_SUCCESS)
    {
        fprintf(stderr, "the window manager interface's 'createSurface' "
                        "callback returned an error\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


static void
dkpDestroySurface(VkInstance instanceHandle,
                  VkSurfaceKHR surfaceHandle,
                  const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(instanceHandle != NULL);
    DK_ASSERT(surfaceHandle != VK_NULL_HANDLE);

    vkDestroySurfaceKHR(instanceHandle, surfaceHandle, pBackEndAllocator);
}


static DkResult
dkpCreateDeviceExtensionNames(DkpPresentSupport presentSupport,
                              const DkAllocator *pAllocator,
                              uint32_t *pExtensionCount,
                              const char ***pppExtensionNames)
{
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pExtensionCount != NULL);
    DK_ASSERT(pppExtensionNames != NULL);

    if (presentSupport == DKP_PRESENT_SUPPORT_DISABLED) {
        *pExtensionCount = 0;
        *pppExtensionNames = NULL;
        return DK_SUCCESS;
    }

    *pExtensionCount = 1;
    *pppExtensionNames = (const char **)
        DK_ALLOCATE(pAllocator, sizeof **pppExtensionNames * *pExtensionCount);
    if (*pppExtensionNames == NULL) {
        fprintf(stderr, "failed to allocate the device extension names\n");
        return DK_ERROR_ALLOCATION;
    }

    (*pppExtensionNames)[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    return DK_SUCCESS;
}


static void
dkpDestroyDeviceExtensionNames(const char **ppExtensionNames,
                               const DkAllocator *pAllocator)
{
    DK_FREE(pAllocator, ppExtensionNames);
}


static DkResult
dkpCheckDeviceExtensionsSupport(VkPhysicalDevice physicalDeviceHandle,
                                uint32_t requiredExtensionCount,
                                const char * const *ppRequiredExtensionNames,
                                const DkAllocator *pAllocator,
                                int *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t extensionCount;
    VkExtensionProperties *pExtensions;

    DK_ASSERT(physicalDeviceHandle != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateDeviceExtensionProperties(physicalDeviceHandle, NULL,
                                             &extensionCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of device extension "
                        "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    pExtensions = (VkExtensionProperties *)
        DK_ALLOCATE(pAllocator, sizeof *pExtensions * extensionCount);
    if (pExtensions == NULL) {
        fprintf(stderr, "failed to allocate the device extension properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateDeviceExtensionProperties(physicalDeviceHandle, NULL,
                                             &extensionCount, pExtensions)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the device extension properties "
                        "available\n");
        out = DK_ERROR;
        goto extensions_cleanup;
    }

    *pSupported = DKP_FALSE;
    for (i = 0; i < requiredExtensionCount; ++i) {
        int found;

        found = DKP_FALSE;
        for (j = 0; j < extensionCount; ++j) {
            if (strcmp(pExtensions[j].extensionName,
                       ppRequiredExtensionNames[i])
                == 0)
            {
                found = DKP_TRUE;
                break;
            }
        }

        if (!found)
            goto extensions_cleanup;
    }

    *pSupported = DKP_TRUE;

extensions_cleanup:
    DK_FREE(pAllocator, pExtensions);

exit:
    return out;
}


static DkResult
dkpPickDeviceQueueFamilies(VkPhysicalDevice physicalDeviceHandle,
                           VkSurfaceKHR surfaceHandle,
                           const DkAllocator *pAllocator,
                           DkpQueueFamilyIndices *pQueueFamilyIndices)
{
    DkResult out;
    uint32_t i;
    uint32_t propertyCount;
    VkQueueFamilyProperties *pProperties;

    DK_ASSERT(physicalDeviceHandle != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pQueueFamilyIndices != NULL);

    out = DK_SUCCESS;

    pQueueFamilyIndices->graphics = UINT32_MAX;
    pQueueFamilyIndices->present = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle,
                                             &propertyCount, NULL);
    if (propertyCount == 0)
        goto exit;

    pProperties = (VkQueueFamilyProperties *)
        DK_ALLOCATE(pAllocator, sizeof *pProperties * propertyCount);
    if (pProperties == NULL) {
        fprintf(stderr, "failed to allocate the queue family properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceHandle,
                                             &propertyCount, pProperties);
    for (i = 0; i < propertyCount; ++i) {
        int graphicsSupported;
        VkBool32 presentSupported;

        graphicsSupported = pProperties[i].queueCount > 0
            && pProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        if (surfaceHandle == VK_NULL_HANDLE) {
            if (graphicsSupported) {
                pQueueFamilyIndices->graphics = i;
                break;
            }
        } else {
            if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeviceHandle, i,
                                                     surfaceHandle,
                                                     &presentSupported)
                != VK_SUCCESS)
            {
                fprintf(stderr, "could not determine support for "
                                "presentation\n");
                out = DK_ERROR;
                goto properties_cleanup;
            }

            if (graphicsSupported && presentSupported) {
                pQueueFamilyIndices->graphics = i;
                pQueueFamilyIndices->present = i;
                break;
            }

            if (graphicsSupported
                && pQueueFamilyIndices->graphics == UINT32_MAX)
            {
                pQueueFamilyIndices->graphics = i;
            } else if (presentSupported
                       && pQueueFamilyIndices->present == UINT32_MAX)
            {
                pQueueFamilyIndices->present = i;
            }
        }
    }

properties_cleanup:
    DK_FREE(pAllocator, pProperties);

exit:
    return out;
}


static void
dkpPickSwapChainFormat(uint32_t formatCount,
                       const VkSurfaceFormatKHR *pFormats,
                       VkSurfaceFormatKHR *pFormat)
{
    uint32_t i;

    DK_ASSERT(pFormats != NULL);
    DK_ASSERT(pFormat != NULL);

    if (formatCount == 1 && pFormats[0].format == VK_FORMAT_UNDEFINED) {
        pFormat->format = VK_FORMAT_B8G8R8A8_UNORM;
        pFormat->colorSpace = pFormats[0].colorSpace;
        return;
    }

    for (i = 0; i < formatCount; ++i) {
        if (pFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
            pFormat->format = pFormats[i].format;
            pFormat->colorSpace = pFormats[i].colorSpace;
            return;
        }
    }

    pFormat->format = pFormats[0].format;
    pFormat->colorSpace = pFormats[0].colorSpace;
}


static void
dkpPickSwapChainPresentMode(uint32_t presentModeCount,
                            const VkPresentModeKHR *pPresentModes,
                            VkPresentModeKHR *pPresentMode)
{
    uint32_t i;

    DK_ASSERT(pPresentModes != NULL);
    DK_ASSERT(pPresentMode != NULL);

    for (i = 0; i < presentModeCount; ++i) {
        if (pPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            *pPresentMode = pPresentModes[i];
            return;
        }
    }

    for (i = 0; i < presentModeCount; ++i) {
        if (pPresentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            *pPresentMode = pPresentModes[i];
            return;
        }
    }

    for (i = 0; i < presentModeCount; ++i) {
        if (pPresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            *pPresentMode = pPresentModes[i];
            return;
        }
    }

    *pPresentMode = (VkPresentModeKHR) -1;
}


static void
dkpPickSwapChainMinImageCount(VkSurfaceCapabilitiesKHR capabilities,
                              VkPresentModeKHR presentMode,
                              uint32_t *pMinImageCount)
{
    DK_ASSERT(pMinImageCount != NULL);

    *pMinImageCount = capabilities.minImageCount;

    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        ++(*pMinImageCount);

    if (capabilities.maxImageCount > 0
        && *pMinImageCount > capabilities.maxImageCount)
    {
        *pMinImageCount = capabilities.maxImageCount;
    }
}


static void
dkpPickSwapChainImageExtent(VkSurfaceCapabilitiesKHR capabilities,
                            const VkExtent2D *pDesiredImageExtent,
                            VkExtent2D *pImageExtent)
{
    DK_ASSERT(pDesiredImageExtent != NULL);
    DK_ASSERT(pImageExtent != NULL);

    if (capabilities.currentExtent.width == UINT32_MAX
        || capabilities.currentExtent.height == UINT32_MAX)
    {
        pImageExtent->width = DKP_CLAMP(pDesiredImageExtent->width,
                                        capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        pImageExtent->height = DKP_CLAMP(pDesiredImageExtent->height,
                                         capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);
        return;
    }

    *pImageExtent = capabilities.currentExtent;
}


static void
dkpPickSwapChainImageUsage(VkSurfaceCapabilitiesKHR capabilities,
                           VkImageUsageFlags *pImageUsage)
{
    DK_ASSERT(pImageUsage != NULL);

    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        *pImageUsage = (VkImageUsageFlags) -1;
        return;
    }

    *pImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
}


static void
dkpPickSwapChainPreTransform(VkSurfaceCapabilitiesKHR capabilities,
                             VkSurfaceTransformFlagBitsKHR *pPreTransform)
{
    DK_ASSERT(pPreTransform != NULL);

    *pPreTransform = capabilities.currentTransform;
}


static DkResult
dkpPickSwapChainProperties(VkPhysicalDevice physicalDeviceHandle,
                           VkSurfaceKHR surfaceHandle,
                           const VkExtent2D *pDesiredImageExtent,
                           const DkAllocator *pAllocator,
                           DkpSwapChainProperties *pSwapChainProperties)
{
    DkResult out;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR *pFormats;
    uint32_t presentModeCount;
    VkPresentModeKHR *pPresentModes;

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSwapChainProperties != NULL);

    out = DK_SUCCESS;

    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDeviceHandle,
                                                  surfaceHandle,
                                                  &capabilities)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the surface capabilities\n");
        out = DK_ERROR;
        goto exit;
    }

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceHandle,
                                             surfaceHandle,
                                             &formatCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the surface formats\n");
        out = DK_ERROR;
        goto exit;
    }

    pFormats = (VkSurfaceFormatKHR *)
        DK_ALLOCATE(pAllocator, sizeof *pFormats * formatCount);
    if (pFormats == NULL) {
        fprintf(stderr, "failed to allocate the surface formats\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceHandle,
                                             surfaceHandle,
                                             &formatCount, pFormats)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the surface formats\n");
        out = DK_ERROR;
        goto formats_cleanup;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceHandle,
                                                  surfaceHandle,
                                                  &presentModeCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the surface present "
                        "modes\n");
        out = DK_ERROR;
        goto formats_cleanup;
    }

    pPresentModes = (VkPresentModeKHR *)
        DK_ALLOCATE(pAllocator, sizeof *pPresentModes * presentModeCount);
    if (pPresentModes == NULL) {
        fprintf(stderr, "failed to allocate the surface present modes\n");
        out = DK_ERROR_ALLOCATION;
        goto formats_cleanup;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceHandle,
                                                  surfaceHandle,
                                                  &presentModeCount,
                                                  pPresentModes)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the surface present modes\n");
        out = DK_ERROR;
        goto present_modes_cleanup;
    }

    dkpPickSwapChainFormat(formatCount, pFormats,
                           &pSwapChainProperties->format);
    dkpPickSwapChainPresentMode(presentModeCount, pPresentModes,
                                &pSwapChainProperties->presentMode);
    dkpPickSwapChainMinImageCount(capabilities,
                                  pSwapChainProperties->presentMode,
                                  &pSwapChainProperties->minImageCount);
    dkpPickSwapChainImageExtent(capabilities, pDesiredImageExtent,
                                &pSwapChainProperties->imageExtent);
    dkpPickSwapChainImageUsage(capabilities,
                               &pSwapChainProperties->imageUsage);
    dkpPickSwapChainPreTransform(capabilities,
                                 &pSwapChainProperties->preTransform);

present_modes_cleanup:
    DK_FREE(pAllocator, pPresentModes);

formats_cleanup:
    DK_FREE(pAllocator, pFormats);

exit:
    return out;
}


static void
dkpCheckSwapChainProperties(const DkpSwapChainProperties *pSwapChainProperties,
                            DkpLogging logging,
                            int *pValid)
{
    DK_ASSERT(pSwapChainProperties != NULL);

    *pValid = DKP_FALSE;

    if (pSwapChainProperties->imageUsage == (VkImageUsageFlags) -1) {
        if (logging == DKP_LOGGING_ENABLED)
            fprintf(stderr, "one or more image usage flags are not "
                            "supported\n");
        return;
    }

    if (pSwapChainProperties->presentMode == (VkPresentModeKHR) -1) {
        if (logging == DKP_LOGGING_ENABLED)
            fprintf(stderr, "could not find a suitable present mode\n");

        return;
    }

    *pValid = DKP_TRUE;
}


static DkResult
dkpCheckSwapChainSupport(VkPhysicalDevice physicalDeviceHandle,
                         VkSurfaceKHR surfaceHandle,
                         const DkAllocator *pAllocator,
                         int *pSupported)
{
    VkExtent2D imageExtent;
    DkpSwapChainProperties swapChainProperties;

    *pSupported = DKP_FALSE;

    /*
       Use dummy image extent values here as we're only interested in checking
       swap chain support rather than actually creating a valid swap chain.
    */
    imageExtent.width = 0;
    imageExtent.height = 0;
    if (dkpPickSwapChainProperties(physicalDeviceHandle, surfaceHandle,
                                   &imageExtent, pAllocator,
                                   &swapChainProperties)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    dkpCheckSwapChainProperties(&swapChainProperties, DKP_LOGGING_DISABLED,
                                pSupported);
    return DK_SUCCESS;
}


static DkResult
dkpInspectPhysicalDevice(VkPhysicalDevice physicalDeviceHandle,
                         VkSurfaceKHR surfaceHandle,
                         uint32_t extensionCount,
                         const char * const *ppExtensionNames,
                         const DkAllocator *pAllocator,
                         DkpQueueFamilyIndices *pQueueFamilyIndices,
                         int *pSuitable)
{
    VkPhysicalDeviceProperties properties;
    int extensionsSupported;
    int swapChainSupported;

    DK_ASSERT(physicalDeviceHandle != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pQueueFamilyIndices != NULL);
    DK_ASSERT(pSuitable != NULL);

    *pSuitable = DKP_FALSE;

    vkGetPhysicalDeviceProperties(physicalDeviceHandle, &properties);
    if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return DK_SUCCESS;

    if (dkpCheckDeviceExtensionsSupport(physicalDeviceHandle, extensionCount,
                                        ppExtensionNames, pAllocator,
                                        &extensionsSupported)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (!extensionsSupported)
        return DK_SUCCESS;

    if (dkpPickDeviceQueueFamilies(physicalDeviceHandle, surfaceHandle,
                                   pAllocator, pQueueFamilyIndices)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (pQueueFamilyIndices->graphics == UINT32_MAX
        || (surfaceHandle != VK_NULL_HANDLE
            && pQueueFamilyIndices->present == UINT32_MAX))
    {
        return DK_SUCCESS;
    }

    if (surfaceHandle != VK_NULL_HANDLE) {
        if (dkpCheckSwapChainSupport(physicalDeviceHandle, surfaceHandle,
                                     pAllocator, &swapChainSupported)
            != DK_SUCCESS)
        {
            return DK_ERROR;
        }

        if (!swapChainSupported)
            return DK_SUCCESS;
    }

    *pSuitable = DKP_TRUE;
    return DK_SUCCESS;
}


static DkResult
dkpPickPhysicalDevice(VkInstance instanceHandle,
                      VkSurfaceKHR surfaceHandle,
                      uint32_t extensionCount,
                      const char * const *ppExtensionNames,
                      const DkAllocator *pAllocator,
                      DkpQueueFamilyIndices *pQueueFamilyIndices,
                      VkPhysicalDevice *pPhysicalDeviceHandle)
{
    DkResult out;
    uint32_t i;
    uint32_t physicalDeviceCount;
    VkPhysicalDevice *pPhysicalDeviceHandles;

    DK_ASSERT(instanceHandle != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pQueueFamilyIndices != NULL);
    DK_ASSERT(pPhysicalDeviceHandle != NULL);

    out = DK_SUCCESS;

    if (vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the physical devices\n");
        out = DK_ERROR;
        goto exit;
    }

    pPhysicalDeviceHandles = (VkPhysicalDevice *)
        DK_ALLOCATE(pAllocator,
                    sizeof *pPhysicalDeviceHandles * physicalDeviceCount);
    if (pPhysicalDeviceHandles == NULL) {
        fprintf(stderr, "failed to allocate the physical devices\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount,
                                   pPhysicalDeviceHandles)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the physical devices\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

    *pPhysicalDeviceHandle = NULL;
    for (i = 0; i < physicalDeviceCount; ++i) {
        int suitable;

        if (dkpInspectPhysicalDevice(pPhysicalDeviceHandles[i], surfaceHandle,
                                     extensionCount, ppExtensionNames,
                                     pAllocator, pQueueFamilyIndices,
                                     &suitable)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto physical_devices_cleanup;
        }

        if (suitable) {
            *pPhysicalDeviceHandle = pPhysicalDeviceHandles[i];
            break;
        }
    }

    if (*pPhysicalDeviceHandle == NULL) {
        fprintf(stderr, "could not find a suitable physical device\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

physical_devices_cleanup:
    DK_FREE(pAllocator, pPhysicalDeviceHandles);

exit:
    return out;
}


static DkResult
dkpCreateDevice(VkInstance instanceHandle,
                VkSurfaceKHR surfaceHandle,
                const VkAllocationCallbacks *pBackEndAllocator,
                const DkAllocator *pAllocator,
                DkpDevice *pDevice)
{
    DkResult out;
    uint32_t i;
    DkpPresentSupport presentSupport;
    uint32_t extensionCount;
    const char **ppExtensionNames;
    uint32_t queueCount;
    float *pQueuePriorities;
    uint32_t queueInfoCount;
    VkDeviceQueueCreateInfo *pQueueInfos;
    VkDeviceCreateInfo createInfo;

    DK_ASSERT(instanceHandle != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pDevice != NULL);

    out = DK_SUCCESS;

    presentSupport = surfaceHandle == VK_NULL_HANDLE
        ? DKP_PRESENT_SUPPORT_DISABLED
        : DKP_PRESENT_SUPPORT_ENABLED;

    if (dkpCreateDeviceExtensionNames(presentSupport, pAllocator,
                                      &extensionCount, &ppExtensionNames)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto exit;
    }

    if (dkpPickPhysicalDevice(instanceHandle, surfaceHandle, extensionCount,
                              ppExtensionNames, pAllocator,
                              &pDevice->queueFamilyIndices,
                              &pDevice->physicalHandle)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto extension_names_cleanup;
    }

    queueCount = 1;
    pQueuePriorities = (float *)
        DK_ALLOCATE(pAllocator, sizeof *pQueuePriorities * queueCount);
    if (pQueuePriorities == NULL) {
        fprintf(stderr, "failed to allocate the device queue priorities\n");
        out = DK_ERROR_ALLOCATION;
        goto extension_names_cleanup;
    }

    for (i = 0; i < queueCount; ++i)
        pQueuePriorities[i] = 1.0f;

    queueInfoCount = (presentSupport == DKP_PRESENT_SUPPORT_ENABLED
                      && pDevice->queueFamilyIndices.graphics
                         != pDevice->queueFamilyIndices.present)
        ? 2 : 1;
    pQueueInfos = (VkDeviceQueueCreateInfo *)
        DK_ALLOCATE(pAllocator, sizeof *pQueueInfos * queueInfoCount);
    if (pQueueInfos == NULL) {
        fprintf(stderr, "failed to allocate the device queue infos\n");
        out = DK_ERROR_ALLOCATION;
        goto queue_priorities_cleanup;
    }

    pQueueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pQueueInfos[0].pNext = NULL;
    pQueueInfos[0].flags = 0;
    pQueueInfos[0].queueFamilyIndex = pDevice->queueFamilyIndices.graphics;
    pQueueInfos[0].queueCount = queueCount;
    pQueueInfos[0].pQueuePriorities = pQueuePriorities;

    if (queueInfoCount == 2) {
        pQueueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        pQueueInfos[1].pNext = NULL;
        pQueueInfos[1].flags = 0;
        pQueueInfos[1].queueFamilyIndex = pDevice->queueFamilyIndices.present;
        pQueueInfos[1].queueCount = queueCount;
        pQueueInfos[1].pQueuePriorities = pQueuePriorities;
    }

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.queueCreateInfoCount = queueInfoCount;
    createInfo.pQueueCreateInfos = pQueueInfos;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;
    createInfo.pEnabledFeatures = NULL;

    switch (vkCreateDevice(pDevice->physicalHandle, &createInfo,
                           pBackEndAllocator, &pDevice->logicalHandle))
    {
        case VK_SUCCESS:
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            fprintf(stderr, "some requested device extensions are not "
                            "supported\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            fprintf(stderr, "some requested device features are not "
                            "supported\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        case VK_ERROR_TOO_MANY_OBJECTS:
            fprintf(stderr, "too many devices have already been created\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        case VK_ERROR_DEVICE_LOST:
            fprintf(stderr, "the device has been lost\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        default:
            fprintf(stderr, "failed to create a device\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
    }

queue_infos_cleanup:
    DK_FREE(pAllocator, pQueueInfos);

queue_priorities_cleanup:
    DK_FREE(pAllocator, pQueuePriorities);

extension_names_cleanup:
    dkpDestroyDeviceExtensionNames(ppExtensionNames, pAllocator);

exit:
    return out;
}


static void
dkpDestroyDevice(DkpDevice *pDevice,
                 const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);

    vkDestroyDevice(pDevice->logicalHandle, pBackEndAllocator);
}


static DkResult
dkpGetDeviceQueues(const DkpDevice *pDevice,
                   DkpQueues *pQueues)
{
    uint32_t queueIndex;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pQueues != NULL);

    queueIndex = 0;

    if (pDevice->queueFamilyIndices.graphics != UINT32_MAX)
        vkGetDeviceQueue(pDevice->logicalHandle,
                         pDevice->queueFamilyIndices.graphics,
                         queueIndex,
                         &pQueues->graphicsHandle);
    else
        pQueues->graphicsHandle = NULL;

    if (pDevice->queueFamilyIndices.present != UINT32_MAX)
        vkGetDeviceQueue(pDevice->logicalHandle,
                         pDevice->queueFamilyIndices.present,
                         queueIndex,
                         &pQueues->presentHandle);
    else
        pQueues->presentHandle = NULL;

    return DK_SUCCESS;
}


static DkResult
dkpCreateSemaphores(const DkpDevice *pDevice,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const DkAllocator *pAllocator,
                    VkSemaphore **ppSemaphoreHandles)
{
    DkResult out;
    int i;
    VkSemaphoreCreateInfo createInfo;

    DK_ASSERT(ppSemaphoreHandles != NULL);

    out = DK_SUCCESS;

    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    *ppSemaphoreHandles = (VkSemaphore *)
        DK_ALLOCATE(pAllocator,
                    sizeof **ppSemaphoreHandles * DKP_SEMAPHORE_ID_ENUM_COUNT);
    if (*ppSemaphoreHandles == NULL) {
        fprintf(stderr, "failed to allocate the semaphores\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i)
        (*ppSemaphoreHandles)[i] = VK_NULL_HANDLE;

    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        if (vkCreateSemaphore(pDevice->logicalHandle, &createInfo,
                              pBackEndAllocator, &(*ppSemaphoreHandles)[i])
            != VK_SUCCESS)
        {
            fprintf(stderr, "failed to create the '%s' semaphores\n",
                dkpGetSemaphoreIdString((DkpSemaphoreId) i));
            out = DK_ERROR;
            goto semaphores_undo;
        }
    }

    goto exit;

semaphores_undo:
    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        if ((*ppSemaphoreHandles)[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(pDevice->logicalHandle, (*ppSemaphoreHandles)[i],
                               pBackEndAllocator);
    }

    DK_FREE(pAllocator, *ppSemaphoreHandles);

exit:
    return out;
}


static void
dkpDestroySemaphores(const DkpDevice *pDevice,
                     VkSemaphore *pSemaphoreHandles,
                     const VkAllocationCallbacks *pBackEndAllocator,
                     const DkAllocator *pAllocator)
{
    int i;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pSemaphoreHandles != NULL);

    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        DK_ASSERT(pSemaphoreHandles[i] != VK_NULL_HANDLE);
        vkDestroySemaphore(pDevice->logicalHandle, pSemaphoreHandles[i],
                           pBackEndAllocator);
    }

    DK_FREE(pAllocator, pSemaphoreHandles);
}


static DkResult
dkpCreateSwapChainImages(const DkpDevice *pDevice,
                         VkSwapchainKHR swapChainHandle,
                         const DkAllocator *pAllocator,
                         uint32_t *pImageCount,
                         VkImage **ppImageHandles)
{
    DkResult out;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(swapChainHandle != VK_NULL_HANDLE);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pImageCount != NULL);
    DK_ASSERT(ppImageHandles != NULL);

    out = DK_SUCCESS;

    if (vkGetSwapchainImagesKHR(pDevice->logicalHandle, swapChainHandle,
                                pImageCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of swap chain images\n");
        out = DK_ERROR;
        goto exit;
    }

    *ppImageHandles = (VkImage *)
        DK_ALLOCATE(pAllocator, sizeof **ppImageHandles * *pImageCount);
    if (*ppImageHandles == NULL) {
        fprintf(stderr, "failed to allocate the swap chain images\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkGetSwapchainImagesKHR(pDevice->logicalHandle, swapChainHandle,
                                pImageCount, *ppImageHandles)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the swap chain images\n");
        out = DK_ERROR;
        goto images_undo;
    }

    goto exit;

images_undo:
    DK_FREE(pAllocator, *ppImageHandles);

exit:
    return out;
}


static void
dkpDestroySwapChainImages(VkImage *pImageHandles,
                          const DkAllocator *pAllocator)
{
    DK_ASSERT(pImageHandles != NULL);
    DK_ASSERT(pAllocator != NULL);

    DK_FREE(pAllocator, pImageHandles);
}


static DkResult
dkpCreateSwapChainImageViewHandles(
    const DkpDevice *pDevice,
    uint32_t imageCount,
    const VkImage *pImageHandles,
    VkFormat format,
    const VkAllocationCallbacks *pBackEndAllocator,
    const DkAllocator *pAllocator,
    VkImageView **ppImageViewHandles)
{
    DkResult out;
    uint32_t i;
    VkImageViewCreateInfo createInfo;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pImageHandles != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(ppImageViewHandles != NULL);

    out = DK_SUCCESS;

    *ppImageViewHandles = (VkImageView *)
        DK_ALLOCATE(pAllocator, sizeof *ppImageViewHandles * imageCount);
    if (*ppImageViewHandles == NULL) {
        fprintf(stderr, "failed to allocate the swap chain image views\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < imageCount; ++i)
        (*ppImageViewHandles)[i] = VK_NULL_HANDLE;

    for (i = 0; i < imageCount; ++i) {
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.image = pImageHandles[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(pDevice->logicalHandle, &createInfo,
                              pBackEndAllocator, &(*ppImageViewHandles)[i])
            != VK_SUCCESS)
        {
            fprintf(stderr, "failed to create an image view\n");
            out = DK_ERROR;
            goto image_views_undo;
        }
    }

    goto exit;

image_views_undo:
    for (i = 0; i < imageCount; ++i) {
        if ((*ppImageViewHandles)[i] != VK_NULL_HANDLE)
            vkDestroyImageView(pDevice->logicalHandle, (*ppImageViewHandles)[i],
                               pBackEndAllocator);
    }

    DK_FREE(pAllocator, *ppImageViewHandles);

exit:
    return out;
}


static void
dkpDestroySwapChainImageViewHandles(
    const DkpDevice *pDevice,
    uint32_t imageCount,
    VkImageView *pImageViewHandles,
    const VkAllocationCallbacks *pBackEndAllocator,
    const DkAllocator *pAllocator)
{
    uint32_t i;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pImageViewHandles != NULL);
    DK_ASSERT(pAllocator != NULL);

    for (i = 0; i < imageCount; ++i) {
        DK_ASSERT(pImageViewHandles[i] != VK_NULL_HANDLE);
        vkDestroyImageView(pDevice->logicalHandle, pImageViewHandles[i],
                           pBackEndAllocator);
    }

    DK_FREE(pAllocator, pImageViewHandles);
}


static DkResult
dkpCreateSwapChain(const DkpDevice *pDevice,
                   VkSurfaceKHR surfaceHandle,
                   const VkExtent2D *pDesiredImageExtent,
                   VkSwapchainKHR oldSwapChainHandle,
                   const VkAllocationCallbacks *pBackEndAllocator,
                   const DkAllocator *pAllocator,
                   DkpSwapChain *pSwapChain)
{
    DkResult out;
    DkpSwapChainProperties swapChainProperties;
    int valid;
    VkSharingMode imageSharingMode;
    uint32_t queueFamilyIndexCount;
    uint32_t *pQueueFamilyIndices;
    VkSwapchainCreateInfoKHR createInfo;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSwapChain != NULL);

    out = DK_SUCCESS;

    if (dkpPickSwapChainProperties(pDevice->physicalHandle, surfaceHandle,
                                   pDesiredImageExtent, pAllocator,
                                   &swapChainProperties)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto exit;
    }

    dkpCheckSwapChainProperties(&swapChainProperties, DKP_LOGGING_ENABLED,
                                &valid);
    if (!valid) {
        out = DK_ERROR;
        goto exit;
    }

    if (pDevice->queueFamilyIndices.graphics
        != pDevice->queueFamilyIndices.present)
    {
        imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        queueFamilyIndexCount = 2;
        pQueueFamilyIndices = (uint32_t *)
            DK_ALLOCATE(pAllocator,
                        sizeof *pQueueFamilyIndices * queueFamilyIndexCount);
        if (pQueueFamilyIndices == NULL) {
            fprintf(stderr, "failed to allocate the queue family indices\n");
            out = DK_ERROR_ALLOCATION;
            goto exit;
        }

        pQueueFamilyIndices[0] = pDevice->queueFamilyIndices.graphics;
        pQueueFamilyIndices[1] = pDevice->queueFamilyIndices.present;
    } else {
        imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        queueFamilyIndexCount = 0;
        pQueueFamilyIndices = NULL;
    }

    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.surface = surfaceHandle;
    createInfo.minImageCount = swapChainProperties.minImageCount;
    createInfo.imageFormat = swapChainProperties.format.format;
    createInfo.imageColorSpace = swapChainProperties.format.colorSpace;
    createInfo.imageExtent = swapChainProperties.imageExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = swapChainProperties.imageUsage;
    createInfo.imageSharingMode = imageSharingMode;
    createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
    createInfo.preTransform = swapChainProperties.preTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = swapChainProperties.presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapChainHandle;

    switch (vkCreateSwapchainKHR(pDevice->logicalHandle, &createInfo,
                                 pBackEndAllocator, &pSwapChain->handle))
    {
        case VK_SUCCESS:
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            fprintf(stderr, "the swap chain's surface is already in use\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
        case VK_ERROR_DEVICE_LOST:
            fprintf(stderr, "the swap chain's device has been lost\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
        case VK_ERROR_SURFACE_LOST_KHR:
            fprintf(stderr, "the swap chain's surface has been lost\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
        default:
            fprintf(stderr, "failed to create the swap chain\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
    }

    if (dkpCreateSwapChainImages(pDevice, pSwapChain->handle, pAllocator,
                                 &pSwapChain->imageCount,
                                 &pSwapChain->pImageHandles)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto queue_family_indices_cleanup;
    }

    if (dkpCreateSwapChainImageViewHandles(pDevice, pSwapChain->imageCount,
                                           pSwapChain->pImageHandles,
                                           swapChainProperties.format.format,
                                           pBackEndAllocator, pAllocator,
                                           &pSwapChain->pImageViewHandles)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto images_undo;
    }

    pSwapChain->format = swapChainProperties.format;
    goto cleanup;

images_undo:
    dkpDestroySwapChainImages(pSwapChain->pImageHandles, pAllocator);

cleanup: ;

queue_family_indices_cleanup:
    if (pQueueFamilyIndices != NULL)
        DK_FREE(pAllocator, pQueueFamilyIndices);

exit:
    if (oldSwapChainHandle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(pDevice->logicalHandle, oldSwapChainHandle,
                              pBackEndAllocator);
    }

    return out;
}


static void
dkpDestroySwapChain(const DkpDevice *pDevice,
                    DkpSwapChain *pSwapChain,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const DkAllocator *pAllocator)
{
    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pSwapChain != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pAllocator != NULL);

    if (pSwapChain->handle == VK_NULL_HANDLE)
        return;

    DK_ASSERT(pSwapChain->pImageHandles != NULL);
    DK_ASSERT(pSwapChain->pImageViewHandles != NULL);

    dkpDestroySwapChainImageViewHandles(pDevice, pSwapChain->imageCount,
                                        pSwapChain->pImageViewHandles,
                                        pBackEndAllocator, pAllocator);
    dkpDestroySwapChainImages(pSwapChain->pImageHandles, pAllocator);
    vkDestroySwapchainKHR(pDevice->logicalHandle, pSwapChain->handle,
                          pBackEndAllocator);
}


static DkResult
dkpRecreateRendererSwapChain(DkRenderer *pRenderer)
{
    if (dkpCreateSwapChain(&pRenderer->device,
                           pRenderer->surfaceHandle,
                           &pRenderer->surfaceExtent,
                           pRenderer->swapChain.handle,
                           pRenderer->pBackEndAllocator,
                           pRenderer->pAllocator,
                           &pRenderer->swapChain)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


static DkResult
dkpCreateRenderPass(const DkpDevice *pDevice,
                    const DkpSwapChain *pSwapChain,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const DkAllocator *pAllocator,
                    VkRenderPass *pRenderPassHandle)
{
    DkResult out;
    uint32_t i;
    uint32_t colorAttachmentCount;
    VkAttachmentDescription *pColorAttachments;
    VkAttachmentReference *pColorAttachmentReferences;
    uint32_t subpassCount;
    VkSubpassDescription *pSubpasses;
    VkRenderPassCreateInfo renderPassInfo;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pSwapChain != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pRenderPassHandle != NULL);

    out = DK_SUCCESS;

    colorAttachmentCount = 1;
    pColorAttachments = (VkAttachmentDescription *)
        DK_ALLOCATE(pAllocator,
                    sizeof *pColorAttachments * colorAttachmentCount);
    if (pColorAttachments == NULL) {
        fprintf(stderr, "failed to allocate the color attachments\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    pColorAttachmentReferences = (VkAttachmentReference *)
        DK_ALLOCATE(pAllocator,
                    sizeof *pColorAttachmentReferences * colorAttachmentCount);
    if (pColorAttachmentReferences == NULL) {
        fprintf(stderr, "failed to allocate the color attachment references\n");
        out = DK_ERROR_ALLOCATION;
        goto color_attachments_cleanup;
    }

    for (i = 0; i < colorAttachmentCount; ++i) {
        pColorAttachments[i].flags = 0;
        pColorAttachments[i].format = pSwapChain->format.format;
        pColorAttachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        pColorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        pColorAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        pColorAttachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        pColorAttachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        pColorAttachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        pColorAttachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        pColorAttachmentReferences[i].attachment = i;
        pColorAttachmentReferences[i].layout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    subpassCount = 1;
    pSubpasses = (VkSubpassDescription *)
        DK_ALLOCATE(pAllocator, sizeof *pSubpasses * subpassCount);
    if (pSubpasses == NULL) {
        fprintf(stderr, "failed to allocate the subpasses\n");
        out = DK_ERROR_ALLOCATION;
        goto color_attachment_references_cleanup;
    }

    pSubpasses[0].flags = 0;
    pSubpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    pSubpasses[0].inputAttachmentCount = 0;
    pSubpasses[0].pInputAttachments = NULL;
    pSubpasses[0].colorAttachmentCount = colorAttachmentCount;
    pSubpasses[0].pColorAttachments = pColorAttachmentReferences;
    pSubpasses[0].pResolveAttachments = NULL;
    pSubpasses[0].pDepthStencilAttachment = NULL;
    pSubpasses[0].preserveAttachmentCount = 0;
    pSubpasses[0].pPreserveAttachments = NULL;

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = NULL;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = colorAttachmentCount;
    renderPassInfo.pAttachments = pColorAttachments;
    renderPassInfo.subpassCount = subpassCount;
    renderPassInfo.pSubpasses = pSubpasses;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = NULL;

    if (vkCreateRenderPass(pDevice->logicalHandle, &renderPassInfo,
                           pBackEndAllocator, pRenderPassHandle)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to created the render pass\n");
        out = DK_ERROR;
        goto subpasses_cleanup;
    }

subpasses_cleanup:
    DK_FREE(pAllocator, pSubpasses);

color_attachment_references_cleanup:
    DK_FREE(pAllocator, pColorAttachmentReferences);

color_attachments_cleanup:
    DK_FREE(pAllocator, pColorAttachments);

exit:
    return out;
}


static void
dkpDestroyRenderPass(const DkpDevice *pDevice,
                     VkRenderPass renderPassHandle,
                     const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(renderPassHandle != VK_NULL_HANDLE);

    vkDestroyRenderPass(pDevice->logicalHandle, renderPassHandle,
                        pBackEndAllocator);
}


static DkResult
dkpCreatePipelineLayout(const DkpDevice *pDevice,
                        const VkAllocationCallbacks *pBackEndAllocator,
                        VkPipelineLayout *pPipelineLayoutHandle)
{
    DkResult out;
    VkPipelineLayoutCreateInfo layoutInfo;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pPipelineLayoutHandle != NULL);

    out = DK_SUCCESS;

    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = NULL;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = NULL;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = 0;

    if (vkCreatePipelineLayout(pDevice->logicalHandle, &layoutInfo,
                               pBackEndAllocator, pPipelineLayoutHandle)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the pipeline layout\n");
        out = DK_ERROR;
        goto exit;
    }

exit:
    return out;
}


static void
dkpDestroyPipelineLayout(const DkpDevice *pDevice,
                         VkPipelineLayout pipelineLayoutHandle,
                         const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pipelineLayoutHandle != VK_NULL_HANDLE);

    vkDestroyPipelineLayout(pDevice->logicalHandle, pipelineLayoutHandle,
                            pBackEndAllocator);
}


static DkResult
dkpCreateShaderCode(const char *pFilePath,
                    const DkAllocator *pAllocator,
                    size_t *pShaderCodeSize,
                    uint32_t **ppShaderCode)
{
    DkResult out;
    DkpFile file;

    DK_ASSERT(pFilePath != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(ppShaderCode != NULL);

    out = DK_SUCCESS;

    if (dkpOpenFile(&file, pFilePath, "rb") != DK_SUCCESS) {
        out = DK_ERROR;
        goto exit;
    }

    if (dkpGetFileSize(&file, pShaderCodeSize) != DK_SUCCESS) {
        out = DK_ERROR;
        goto file_closing;
    }

    DK_ASSERT(*pShaderCodeSize % sizeof **ppShaderCode == 0);

    *ppShaderCode = DK_ALLOCATE(pAllocator, *pShaderCodeSize);
    if (*ppShaderCode == NULL) {
        fprintf(stderr, "failed to allocate the shader code for the file "
                        "'%s'\n", pFilePath);
        out = DK_ERROR_ALLOCATION;
        goto file_closing;
    }

    if (dkpReadFile(&file, *pShaderCodeSize, (void *) *ppShaderCode)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto code_undo;
    }

    goto cleanup;

code_undo:
    DK_FREE(pAllocator, *ppShaderCode);

cleanup: ;

file_closing:
    if (dkpCloseFile(&file) != DK_SUCCESS)
        out = DK_ERROR;

exit:
    return out;
}


static void
dkpDestroyShaderCode(uint32_t *pShaderCode,
                     const DkAllocator *pAllocator)
{
    DK_ASSERT(pShaderCode != NULL);
    DK_ASSERT(pAllocator != NULL);

    DK_FREE(pAllocator, pShaderCode);
}


static DkResult
dkpCreateShaderModule(const DkpDevice *pDevice,
                      const char *pFilePath,
                      const VkAllocationCallbacks *pBackEndAllocator,
                      const DkAllocator *pAllocator,
                      VkShaderModule *pShaderModuleHandle)
{
    DkResult out;
    size_t codeSize;
    uint32_t *pCode;
    VkShaderModuleCreateInfo createInfo;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pFilePath != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pShaderModuleHandle != NULL);

    out = DK_SUCCESS;

    if (dkpCreateShaderCode(pFilePath, pAllocator, &codeSize, &pCode)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto exit;
    }

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.codeSize = codeSize;
    createInfo.pCode = pCode;

    if (vkCreateShaderModule(pDevice->logicalHandle, &createInfo,
                             pBackEndAllocator, pShaderModuleHandle)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the shader module from the file "
                        "'%s'\n", pFilePath);
        out = DK_ERROR;
        goto code_cleanup;
    }

code_cleanup:
    dkpDestroyShaderCode(pCode, pAllocator);

exit:
    return out;
}


static void
dkpDestroyShaderModule(const DkpDevice *pDevice,
                       VkShaderModule shaderModuleHandle,
                       const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(shaderModuleHandle != VK_NULL_HANDLE);

    vkDestroyShaderModule(pDevice->logicalHandle, shaderModuleHandle,
                          pBackEndAllocator);
}


static DkResult
dkpCreateGraphicsPipeline(const DkpDevice *pDevice,
                          VkPipelineLayout pipelineLayoutHandle,
                          VkRenderPass renderPassHandle,
                          uint32_t shaderCount,
                          const DkShaderCreateInfo *pShaderInfos,
                          const VkExtent2D *pSurfaceExtent,
                          const VkAllocationCallbacks *pBackEndAllocator,
                          const DkAllocator *pAllocator,
                          VkPipeline *pPipelineHandle)
{
    DkResult out;
    uint32_t i;
    VkShaderModule *pShaderModuleHandles;
    VkPipelineShaderStageCreateInfo *pShaderStageInfos;
    uint32_t viewportCount;
    VkViewport *pViewports;
    uint32_t scissorCount;
    VkRect2D *pScissors;
    uint32_t colorBlendAttachmentStateCount;
    VkPipelineColorBlendAttachmentState *pColorBlendAttachmentStates;
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
    VkPipelineViewportStateCreateInfo viewportStateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo;
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo;
    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
    uint32_t createInfoCount;
    VkGraphicsPipelineCreateInfo *pCreateInfos;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pipelineLayoutHandle != VK_NULL_HANDLE);
    DK_ASSERT(renderPassHandle != VK_NULL_HANDLE);
    DK_ASSERT(pShaderInfos != NULL);
    DK_ASSERT(pSurfaceExtent != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pPipelineHandle != NULL);

    out = DK_SUCCESS;

    pShaderModuleHandles = (VkShaderModule *)
        DK_ALLOCATE(pAllocator, sizeof *pShaderModuleHandles * shaderCount);
    if (pShaderModuleHandles == NULL) {
        fprintf(stderr, "failed to allocate the shader modules\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < shaderCount; ++i)
        pShaderModuleHandles[i] = VK_NULL_HANDLE;

    for (i = 0; i < shaderCount; ++i) {
        if (dkpCreateShaderModule(pDevice, pShaderInfos[i].pFilePath,
                                  pBackEndAllocator, pAllocator,
                                  &pShaderModuleHandles[i])
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto shader_modules_cleanup;
        }
    }

    pShaderStageInfos = (VkPipelineShaderStageCreateInfo *)
        DK_ALLOCATE(pAllocator, sizeof *pShaderStageInfos * shaderCount);
    if (pShaderStageInfos == NULL) {
        fprintf(stderr, "failed to allocate the shader stage infos\n");
        out = DK_ERROR_ALLOCATION;
        goto shader_modules_cleanup;
    }

    for (i = 0; i < shaderCount; ++i) {
        VkShaderStageFlagBits shaderStage;

        shaderStage = dkpTranslateShaderStage(pShaderInfos[i].stage);

        pShaderStageInfos[i].sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pShaderStageInfos[i].pNext = NULL;
        pShaderStageInfos[i].flags = 0;
        pShaderStageInfos[i].stage = shaderStage;
        pShaderStageInfos[i].module = pShaderModuleHandles[i];
        pShaderStageInfos[i].pName = pShaderInfos[i].pEntryPointName;
        pShaderStageInfos[i].pSpecializationInfo = NULL;
    }

    viewportCount = 1;
    pViewports = (VkViewport *)
        DK_ALLOCATE(pAllocator, sizeof *pViewports * viewportCount);
    if (pViewports == NULL) {
        fprintf(stderr, "failed to allocate the viewports\n");
        out = DK_ERROR_ALLOCATION;
        goto shader_stage_infos_cleanup;
    }

    pViewports[0].x = 0.0f;
    pViewports[0].y = 0.0f;
    pViewports[0].width = (float) pSurfaceExtent->width;
    pViewports[0].height = (float) pSurfaceExtent->height;
    pViewports[0].minDepth = 0.0f;
    pViewports[0].maxDepth = 1.0f;

    scissorCount = 1;
    pScissors = (VkRect2D *)
        DK_ALLOCATE(pAllocator, sizeof *pScissors * scissorCount);
    if (pScissors == NULL) {
        fprintf(stderr, "failed to allocate the scissors\n");
        out = DK_ERROR_ALLOCATION;
        goto viewports_cleanup;
    }

    pScissors[0].offset.x = 0;
    pScissors[0].offset.y = 0;
    pScissors[0].extent = *pSurfaceExtent;

    colorBlendAttachmentStateCount = 1;
    pColorBlendAttachmentStates = (VkPipelineColorBlendAttachmentState *)
        DK_ALLOCATE(pAllocator,
                    (sizeof *pColorBlendAttachmentStates
                     * colorBlendAttachmentStateCount));
    if (pColorBlendAttachmentStates == NULL) {
        fprintf(stderr, "failed to allocate the color blend attachment "
                        "states\n");
        out = DK_ERROR_ALLOCATION;
        goto scissors_cleanup;
    }

    pColorBlendAttachmentStates[0].blendEnable = VK_FALSE;
    pColorBlendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pColorBlendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pColorBlendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
    pColorBlendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pColorBlendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pColorBlendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
    pColorBlendAttachmentStates[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;

    vertexInputStateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfo.pNext = NULL;
    vertexInputStateInfo.flags = 0;
    vertexInputStateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateInfo.pVertexBindingDescriptions = NULL;
    vertexInputStateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputStateInfo.pVertexAttributeDescriptions = NULL;

    inputAssemblyStateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.pNext = NULL;
    inputAssemblyStateInfo.flags = 0;
    inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;

    viewportStateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.pNext = NULL;
    viewportStateInfo.flags = 0;
    viewportStateInfo.viewportCount = viewportCount;
    viewportStateInfo.pViewports = pViewports;
    viewportStateInfo.scissorCount = scissorCount;
    viewportStateInfo.pScissors = pScissors;

    rasterizationStateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.pNext = NULL;
    rasterizationStateInfo.flags = 0;
    rasterizationStateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateInfo.depthBiasClamp = 0.0f;
    rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStateInfo.lineWidth = 1.0f;

    multisampleStateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.pNext = NULL;
    multisampleStateInfo.flags = 0;
    multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateInfo.minSampleShading = 1.0;
    multisampleStateInfo.pSampleMask = NULL;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;

    colorBlendStateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.pNext = NULL;
    colorBlendStateInfo.flags = 0;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = colorBlendAttachmentStateCount;
    colorBlendStateInfo.pAttachments = pColorBlendAttachmentStates;
    colorBlendStateInfo.blendConstants[0] = 0.0f;
    colorBlendStateInfo.blendConstants[1] = 0.0f;
    colorBlendStateInfo.blendConstants[2] = 0.0f;
    colorBlendStateInfo.blendConstants[3] = 0.0f;

    createInfoCount = 1;
    pCreateInfos = (VkGraphicsPipelineCreateInfo *)
        DK_ALLOCATE(pAllocator, sizeof *pCreateInfos * createInfoCount);
    if (pCreateInfos == NULL) {
        fprintf(stderr, "failed to allocate the graphics pipeline create "
                        "infos\n");
        out = DK_ERROR_ALLOCATION;
        goto color_blend_attachment_states_cleanup;
    }

    pCreateInfos[0].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pCreateInfos[0].pNext = NULL;
    pCreateInfos[0].flags = 0;
    pCreateInfos[0].stageCount = shaderCount;
    pCreateInfos[0].pStages = pShaderStageInfos;
    pCreateInfos[0].pVertexInputState = &vertexInputStateInfo;
    pCreateInfos[0].pInputAssemblyState = &inputAssemblyStateInfo;
    pCreateInfos[0].pTessellationState = NULL;
    pCreateInfos[0].pViewportState = &viewportStateInfo;
    pCreateInfos[0].pRasterizationState = &rasterizationStateInfo;
    pCreateInfos[0].pMultisampleState = &multisampleStateInfo;
    pCreateInfos[0].pDepthStencilState = NULL;
    pCreateInfos[0].pColorBlendState = &colorBlendStateInfo;
    pCreateInfos[0].pDynamicState = NULL;
    pCreateInfos[0].layout = pipelineLayoutHandle;
    pCreateInfos[0].renderPass = renderPassHandle;
    pCreateInfos[0].subpass = 0;
    pCreateInfos[0].basePipelineHandle = VK_NULL_HANDLE;
    pCreateInfos[0].basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(pDevice->logicalHandle, VK_NULL_HANDLE,
                                  createInfoCount, pCreateInfos,
                                  pBackEndAllocator, pPipelineHandle)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the graphics pipelines\n");
        out = DK_ERROR;
        goto create_infos_cleanup;
    }

create_infos_cleanup:
    DK_FREE(pAllocator, pCreateInfos);

color_blend_attachment_states_cleanup:
    DK_FREE(pAllocator, pColorBlendAttachmentStates);

scissors_cleanup:
    DK_FREE(pAllocator, pScissors);

viewports_cleanup:
    DK_FREE(pAllocator, pViewports);

shader_stage_infos_cleanup:
    DK_FREE(pAllocator, pShaderStageInfos);

shader_modules_cleanup:
    for (i = 0; i < shaderCount; ++i) {
        if (pShaderModuleHandles[i] != VK_NULL_HANDLE)
            dkpDestroyShaderModule(pDevice, pShaderModuleHandles[i],
                                   pBackEndAllocator);
    }

    DK_FREE(pAllocator, pShaderModuleHandles);

exit:
    return out;
}


static void
dkpDestroyGraphicsPipeline(const DkpDevice *pDevice,
                           VkPipeline pipelineHandle,
                           const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pipelineHandle != VK_NULL_HANDLE);

    vkDestroyPipeline(pDevice->logicalHandle, pipelineHandle,
                      pBackEndAllocator);
}


static DkResult
dkpCreateFramebuffers(const DkpDevice *pDevice,
                      const DkpSwapChain *pSwapChain,
                      VkRenderPass renderPassHandle,
                      const VkExtent2D *pSurfaceExtent,
                      const VkAllocationCallbacks *pBackEndAllocator,
                      const DkAllocator *pAllocator,
                      VkFramebuffer **ppFramebufferHandles)
{
    DkResult out;
    uint32_t i;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);
    DK_ASSERT(pSwapChain != NULL);
    DK_ASSERT(pSwapChain->handle != VK_NULL_HANDLE);
    DK_ASSERT(renderPassHandle != VK_NULL_HANDLE);
    DK_ASSERT(pSurfaceExtent != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(ppFramebufferHandles != NULL);

    out = DK_SUCCESS;

    *ppFramebufferHandles = (VkFramebuffer *)
        DK_ALLOCATE(pAllocator,
                    sizeof **ppFramebufferHandles * pSwapChain->imageCount);
    if (*ppFramebufferHandles == NULL) {
        fprintf(stderr, "failed to allocate the frame buffers\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < pSwapChain->imageCount; ++i)
        (*ppFramebufferHandles)[i] = VK_NULL_HANDLE;

    for (i = 0; i < pSwapChain->imageCount; ++i) {
        VkImageView pAttachments[1];
        VkFramebufferCreateInfo framebufferInfo;

        pAttachments[0] = pSwapChain->pImageViewHandles[i];

        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.pNext = NULL;
        framebufferInfo.flags = 0;
        framebufferInfo.renderPass = renderPassHandle;
        framebufferInfo.attachmentCount = (uint32_t)
            sizeof pAttachments / sizeof *pAttachments;
        framebufferInfo.pAttachments = pAttachments;
        framebufferInfo.width = pSurfaceExtent->width;
        framebufferInfo.height = pSurfaceExtent->height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(pDevice->logicalHandle, &framebufferInfo,
                                pBackEndAllocator, &(*ppFramebufferHandles)[i])
            != VK_SUCCESS)
        {
            fprintf(stderr, "failed to create a frame buffer\n");
            out = DK_ERROR;
            goto frame_buffers_undo;
        }
    }

    goto exit;

frame_buffers_undo:
    for (i = 0; i < pSwapChain->imageCount; ++i) {
        if ((*ppFramebufferHandles)[i] != VK_NULL_HANDLE)
            vkDestroyFramebuffer(pDevice->logicalHandle,
                                 (*ppFramebufferHandles)[i], pBackEndAllocator);
    }

    DK_FREE(pAllocator, *ppFramebufferHandles);

exit:
    return out;
}


static void
dkpDestroyFramebuffers(const DkpDevice *pDevice,
                       const DkpSwapChain *pSwapChain,
                       VkFramebuffer *pFramebufferHandles,
                       const VkAllocationCallbacks *pBackEndAllocator,
                       const DkAllocator *pAllocator)
{
    uint32_t i;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logicalHandle != NULL);

    for (i = 0; i < pSwapChain->imageCount; ++i) {
        DK_ASSERT(pFramebufferHandles[i] != VK_NULL_HANDLE);
        vkDestroyFramebuffer(pDevice->logicalHandle, pFramebufferHandles[i],
                             pBackEndAllocator);
    }

    DK_FREE(pAllocator, pFramebufferHandles);
}


static void
dkpCheckRendererCreateInfo(const DkRendererCreateInfo *pCreateInfo,
                           int *pValid)
{
    uint32_t i;

    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(pValid != NULL);

    *pValid = DKP_FALSE;

    for (i = 0; i < pCreateInfo->shaderCount; ++i) {
        if (!dkpCheckShaderStage(pCreateInfo->pShaderInfos[i].stage)) {
            fprintf(stderr, "invalid enum value for 'pCreateInfo->"
                            "pShaderInfos[%d].stage'\n", i);
            return;
        }
    }

    *pValid = DKP_TRUE;
}


DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DkResult out;
    int valid;
    int headless;

    out = DK_SUCCESS;

    if (pCreateInfo == NULL) {
        fprintf(stderr, "invalid argument 'pCreateInfo' (NULL)\n");
        out = DK_ERROR_INVALID_VALUE;
        goto exit;
    }

    if (ppRenderer == NULL) {
        fprintf(stderr, "invalid argument 'ppRenderer' (NULL)\n");
        out = DK_ERROR_INVALID_VALUE;
        goto exit;
    }

    dkpCheckRendererCreateInfo(pCreateInfo, &valid);
    if (!valid) {
        out = DK_ERROR_INVALID_VALUE;
        goto exit;
    }

    if (pAllocator == NULL)
        dkpGetDefaultAllocator(&pAllocator);

    headless = pCreateInfo->pWindowCallbacks == NULL;

    *ppRenderer = (DkRenderer *) DK_ALLOCATE(pAllocator, sizeof **ppRenderer);
    if (*ppRenderer == NULL) {
        fprintf(stderr, "failed to allocate the renderer\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    (*ppRenderer)->pAllocator = pAllocator;
    (*ppRenderer)->pBackEndAllocator = pCreateInfo->pBackEndAllocator;
    (*ppRenderer)->surfaceExtent.width = (uint32_t) pCreateInfo->surfaceWidth;
    (*ppRenderer)->surfaceExtent.height = (uint32_t) pCreateInfo->surfaceHeight;

    if (dkpCreateInstance(pCreateInfo->pApplicationName,
                          (unsigned int) pCreateInfo->applicationMajorVersion,
                          (unsigned int) pCreateInfo->applicationMinorVersion,
                          (unsigned int) pCreateInfo->applicationPatchVersion,
                          pCreateInfo->pWindowCallbacks,
                          (*ppRenderer)->pBackEndAllocator,
                          (*ppRenderer)->pAllocator,
                          &(*ppRenderer)->instanceHandle)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto renderer_undo;
    }

    if (DK_ENABLE_DEBUG_REPORT) {
        if (dkpCreateDebugReportCallback(
                (*ppRenderer)->instanceHandle,
                (*ppRenderer)->pBackEndAllocator,
                &(*ppRenderer)->debugReportCallbackHandle)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto instance_undo;
        }
    } else
        (*ppRenderer)->debugReportCallbackHandle = VK_NULL_HANDLE;

    if (!headless) {
        if (dkpCreateSurface((*ppRenderer)->instanceHandle,
                             pCreateInfo->pWindowCallbacks,
                             (*ppRenderer)->pBackEndAllocator,
                             &(*ppRenderer)->surfaceHandle)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto debug_report_callback_undo;
        }
    } else
        (*ppRenderer)->surfaceHandle = VK_NULL_HANDLE;

    if (dkpCreateDevice((*ppRenderer)->instanceHandle,
                        (*ppRenderer)->surfaceHandle,
                        (*ppRenderer)->pBackEndAllocator,
                        (*ppRenderer)->pAllocator,
                        &(*ppRenderer)->device)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto surface_undo;
    }

    if (dkpGetDeviceQueues(&(*ppRenderer)->device,
                           &(*ppRenderer)->queues)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto device_undo;
    }

    if (dkpCreateSemaphores(&(*ppRenderer)->device,
                            (*ppRenderer)->pBackEndAllocator,
                            (*ppRenderer)->pAllocator,
                            &(*ppRenderer)->pSemaphoreHandles)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto device_undo;
    }

    if (!headless) {
        if (dkpCreateSwapChain(&(*ppRenderer)->device,
                               (*ppRenderer)->surfaceHandle,
                               &(*ppRenderer)->surfaceExtent,
                               VK_NULL_HANDLE,
                               (*ppRenderer)->pBackEndAllocator,
                               (*ppRenderer)->pAllocator,
                               &(*ppRenderer)->swapChain)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto semaphores_undo;
        }

        if (dkpCreateRenderPass(&(*ppRenderer)->device,
                                &(*ppRenderer)->swapChain,
                                (*ppRenderer)->pBackEndAllocator,
                                (*ppRenderer)->pAllocator,
                                &(*ppRenderer)->renderPassHandle)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto swap_chain_undo;
        }

        if (dkpCreatePipelineLayout(&(*ppRenderer)->device,
                                    (*ppRenderer)->pBackEndAllocator,
                                    &(*ppRenderer)->pipelineLayoutHandle)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto render_pass_undo;
        }

        if (dkpCreateGraphicsPipeline(&(*ppRenderer)->device,
                                      (*ppRenderer)->pipelineLayoutHandle,
                                      (*ppRenderer)->renderPassHandle,
                                      (uint32_t) pCreateInfo->shaderCount,
                                      pCreateInfo->pShaderInfos,
                                      &(*ppRenderer)->surfaceExtent,
                                      (*ppRenderer)->pBackEndAllocator,
                                      (*ppRenderer)->pAllocator,
                                      &(*ppRenderer)->graphicsPipelineHandle)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto pipeline_layout_undo;
        }

        if (dkpCreateFramebuffers(&(*ppRenderer)->device,
                                  &(*ppRenderer)->swapChain,
                                  (*ppRenderer)->renderPassHandle,
                                  &(*ppRenderer)->surfaceExtent,
                                  (*ppRenderer)->pBackEndAllocator,
                                  (*ppRenderer)->pAllocator,
                                  &(*ppRenderer)->pFramebufferHandles)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto graphics_pipeline_undo;
        }
    }

    goto exit;

graphics_pipeline_undo:
    if (!headless)
        dkpDestroyGraphicsPipeline(&(*ppRenderer)->device,
                                   (*ppRenderer)->graphicsPipelineHandle,
                                   (*ppRenderer)->pBackEndAllocator);

pipeline_layout_undo:
    if (!headless)
        dkpDestroyPipelineLayout(&(*ppRenderer)->device,
                                 (*ppRenderer)->pipelineLayoutHandle,
                                 (*ppRenderer)->pBackEndAllocator);

render_pass_undo:
    if (!headless)
        dkpDestroyRenderPass(&(*ppRenderer)->device,
                             (*ppRenderer)->renderPassHandle,
                             (*ppRenderer)->pBackEndAllocator);

swap_chain_undo:
    if (!headless)
        dkpDestroySwapChain(&(*ppRenderer)->device,
                            &(*ppRenderer)->swapChain,
                            (*ppRenderer)->pBackEndAllocator,
                            (*ppRenderer)->pAllocator);

semaphores_undo:
    dkpDestroySemaphores(&(*ppRenderer)->device,
                         (*ppRenderer)->pSemaphoreHandles,
                         (*ppRenderer)->pBackEndAllocator,
                         (*ppRenderer)->pAllocator);

device_undo:
    dkpDestroyDevice(&(*ppRenderer)->device, (*ppRenderer)->pBackEndAllocator);

surface_undo:
    if (!headless)
        dkpDestroySurface((*ppRenderer)->instanceHandle,
                          (*ppRenderer)->surfaceHandle,
                          (*ppRenderer)->pBackEndAllocator);

debug_report_callback_undo:
    if (DK_ENABLE_DEBUG_REPORT)
        dkpDestroyDebugReportCallback((*ppRenderer)->instanceHandle,
                                      (*ppRenderer)->debugReportCallbackHandle,
                                      (*ppRenderer)->pBackEndAllocator);

instance_undo:
    dkpDestroyInstance((*ppRenderer)->instanceHandle,
                       (*ppRenderer)->pBackEndAllocator);

renderer_undo:
    DK_FREE((*ppRenderer)->pAllocator, (*ppRenderer));
    *ppRenderer = NULL;

exit:
    return out;
}


void
dkDestroyRenderer(DkRenderer *pRenderer,
                  const DkAllocator *pAllocator)
{
    int headless;

    if (pRenderer == NULL)
        return;

    if (pAllocator == NULL)
        dkpGetDefaultAllocator(&pAllocator);

    headless = pRenderer->surfaceHandle == VK_NULL_HANDLE;

    if (!headless) {
        dkpDestroyFramebuffers(&pRenderer->device,
                               &pRenderer->swapChain,
                               pRenderer->pFramebufferHandles,
                               pRenderer->pBackEndAllocator,
                               pRenderer->pAllocator);
        dkpDestroyGraphicsPipeline(&pRenderer->device,
                                   pRenderer->graphicsPipelineHandle,
                                   pRenderer->pBackEndAllocator);
        dkpDestroyPipelineLayout(&pRenderer->device,
                                 pRenderer->pipelineLayoutHandle,
                                 pRenderer->pBackEndAllocator);
        dkpDestroyRenderPass(&pRenderer->device,
                             pRenderer->renderPassHandle,
                             pRenderer->pBackEndAllocator);
        dkpDestroySwapChain(&pRenderer->device, &pRenderer->swapChain,
                            pRenderer->pBackEndAllocator, pAllocator);
    }

    dkpDestroySemaphores(&pRenderer->device, pRenderer->pSemaphoreHandles,
                         pRenderer->pBackEndAllocator, pRenderer->pAllocator);
    dkpDestroyDevice(&pRenderer->device, pRenderer->pBackEndAllocator);

    if (!headless)
        dkpDestroySurface(pRenderer->instanceHandle,
                          pRenderer->surfaceHandle,
                          pRenderer->pBackEndAllocator);

    if (DK_ENABLE_DEBUG_REPORT)
        dkpDestroyDebugReportCallback(pRenderer->instanceHandle,
                                      pRenderer->debugReportCallbackHandle,
                                      pRenderer->pBackEndAllocator);

    dkpDestroyInstance(pRenderer->instanceHandle, pRenderer->pBackEndAllocator);
    DK_FREE(pAllocator, pRenderer);
}


DkResult
dkResizeRendererSurface(DkRenderer *pRenderer,
                        DkUint32 width,
                        DkUint32 height)
{
    DK_ASSERT(pRenderer != NULL);

    pRenderer->surfaceExtent.width = (uint32_t) width;
    pRenderer->surfaceExtent.height = (uint32_t) height;
    return dkpRecreateRendererSwapChain(pRenderer);
}
