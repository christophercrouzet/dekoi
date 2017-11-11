#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "memory.h"
#include "renderer.h"
#include "internal/assert.h"
#include "internal/memory.h"


#ifdef DK_DEBUG
#define DK_ENABLE_DEBUG_REPORT
#define DK_ENABLE_VALIDATION_LAYERS
#endif /* DK_DEBUG */

#define DK_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DK_MAX(a, b) (((a) > (b)) ? (a) : (b))


typedef struct DkpQueueFamilyIndices {
    uint32_t graphics;
    uint32_t present;
} DkpQueueFamilyIndices;


typedef struct DkpDevice {
    DkpQueueFamilyIndices queueFamilyIndices;
    VkPhysicalDevice physical;
    VkDevice logical;
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
    VkQueue graphics;
    VkQueue present;
} DkpQueues;


typedef struct DkpSemaphores {
    VkSemaphore imageAcquired;
    VkSemaphore presentCompleted;
} DkpSemaphores;


typedef struct DkpSwapChain {
    VkSwapchainKHR handle;
    uint32_t imageCount;
    VkImage *pImages;
} DkpSwapChain;


struct DkRenderer {
    const DkAllocator *pAllocator;
    const VkAllocationCallbacks *pBackEndAllocator;
    VkInstance instance;
    VkExtent2D surfaceExtent;
    VkSurfaceKHR surface;
#ifdef DK_ENABLE_DEBUG_REPORT
    VkDebugReportCallbackEXT debugReportCallback;
#endif /* DK_ENABLE_DEBUG_REPORT */
    DkpDevice device;
    DkpQueues queues;
    DkpSemaphores semaphores;
    DkpSwapChain swapChain;
};


typedef enum DkpLogging {
    DKP_LOGGING_DISABLED = 0,
    DKP_LOGGING_ENABLED = 1
} DkpLogging;


typedef enum DkpPresentSupport {
    DKP_PRESENT_SUPPORT_DISABLED = 0,
    DKP_PRESENT_SUPPORT_ENABLED = 1
} DkpPresentSupport;


static DkResult
dkpCreateInstanceLayerNames(const DkAllocator *pAllocator,
                            uint32_t *pLayerCount,
                            const char ***pppLayerNames)
{
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pLayerCount != NULL);
    DK_ASSERT(pppLayerNames != NULL);

#ifdef DK_ENABLE_VALIDATION_LAYERS
    *pLayerCount = 1;
    *pppLayerNames = (const char **)
        DK_ALLOCATE(pAllocator, (*pLayerCount) * sizeof(char *));
    if (*pppLayerNames == NULL) {
        fprintf(stderr, "failed to allocate the instance layer names\n");
        return DK_ERROR_ALLOCATION;
    }

    (*pppLayerNames)[0] = "VK_LAYER_LUNARG_standard_validation";
#else
    DK_UNUSED(pAllocator);

    *pLayerCount = 0;
    *pppLayerNames = NULL;
#endif /* DK_ENABLE_VALIDATION_LAYERS */

    return DK_SUCCESS;
}


static void
dkpDestroyInstanceLayerNames(const char **ppLayerNames,
                             const DkAllocator *pAllocator)
{
#ifdef DK_ENABLE_VALIDATION_LAYERS
    DK_FREE(pAllocator, ppLayerNames);
#else
    DK_UNUSED(ppLayerNames);
    DK_UNUSED(pAllocator);
#endif /* DK_ENABLE_VALIDATION_LAYERS */
}


static DkResult
dkpCheckInstanceLayersSupport(uint32_t requiredLayerCount,
                              const char * const * ppRequiredLayerNames,
                              const DkAllocator *pAllocator,
                              DkBool32 *pSupported)
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
        DK_ALLOCATE(pAllocator, layerCount * sizeof(VkLayerProperties));
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

    *pSupported = DK_FALSE;
    for (i = 0; i < requiredLayerCount; ++i) {
        DkBool32 found;

        found = DK_FALSE;
        for (j = 0; j < layerCount; ++j) {
            if (strcmp(pLayers[j].layerName, ppRequiredLayerNames[i]) == 0) {
                found = DK_TRUE;
                break;
            }
        }

        if (!found)
            goto layers_cleanup;
    }

    *pSupported = DK_TRUE;

layers_cleanup:
    DK_FREE(pAllocator, pLayers);

exit:
    return out;
}


static DkResult
dkpCreateInstanceExtensionNames(
    const DkWindowManagerInterface *pWindowManagerInterface,
    const DkAllocator *pAllocator,
    uint32_t *pExtensionCount,
    const char ***pppExtensionNames)
{
#ifdef DK_ENABLE_DEBUG_REPORT
    const char **ppBuffer;
#else
    DK_UNUSED(pAllocator);
#endif /* DK_ENABLE_DEBUG_REPORT */

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pExtensionCount != NULL);
    DK_ASSERT(pppExtensionNames != NULL);

    if (pWindowManagerInterface == NULL) {
        *pExtensionCount = 0;
        *pppExtensionNames = NULL;
    } else if (pWindowManagerInterface->pfnCreateInstanceExtensionNames(
                   pWindowManagerInterface->pContext,
                   (uint32_t *) pExtensionCount,
                   &(*pppExtensionNames))
               != DK_SUCCESS)
    {
        return DK_ERROR;
    }

#ifdef DK_ENABLE_DEBUG_REPORT
    ppBuffer = (const char **)
        DK_ALLOCATE(pAllocator, ((*pExtensionCount) + 1) * sizeof(char *));
    if (ppBuffer == NULL) {
        fprintf(stderr, "failed to allocate the instance extension names\n");
        return DK_ERROR_ALLOCATION;
    }

    if (*pppExtensionNames != NULL)
        memcpy(ppBuffer, *pppExtensionNames,
               (*pExtensionCount) * sizeof(char *));

    ppBuffer[*pExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

    if (pWindowManagerInterface != NULL)
        pWindowManagerInterface->pfnDestroyInstanceExtensionNames(
            pWindowManagerInterface->pContext,
            *pppExtensionNames);

    *pExtensionCount += 1;
    *pppExtensionNames = ppBuffer;
#endif /* DK_ENABLE_DEBUG_REPORT */

    return DK_SUCCESS;
}


static void
dkpDestroyInstanceExtensionNames(
    const char **ppExtensionNames,
    const DkWindowManagerInterface *pWindowManagerInterface,
    const DkAllocator *pAllocator)
{
#ifdef DK_ENABLE_DEBUG_REPORT
    DK_UNUSED(pWindowManagerInterface);

    DK_ASSERT(pAllocator != NULL);

    DK_FREE(pAllocator, ppExtensionNames);
#else
    DK_UNUSED(pAllocator);

    if (pWindowManagerInterface != NULL)
        pWindowManagerInterface->pfnDestroyInstanceExtensionNames(
            pWindowManagerInterface->pContext,
            ppExtensionNames);
#endif /* DK_ENABLE_DEBUG_REPORT */
}


static DkResult
dkpCheckInstanceExtensionsSupport(uint32_t requiredExtensionCount,
                                  const char * const * ppRequiredExtensionNames,
                                  const DkAllocator *pAllocator,
                                  DkBool32 *pSupported)
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
        DK_ALLOCATE(pAllocator, extensionCount * sizeof(VkExtensionProperties));
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

    *pSupported = DK_FALSE;
    for (i = 0; i < requiredExtensionCount; ++i) {
        DkBool32 found;

        found = DK_FALSE;
        for (j = 0; j < extensionCount; ++j) {
            if (strcmp(pExtensions[j].extensionName,
                       ppRequiredExtensionNames[i])
                == 0)
            {
                found = DK_TRUE;
                break;
            }
        }

        if (!found)
            goto extensions_cleanup;
    }

    *pSupported = DK_TRUE;

extensions_cleanup:
    DK_FREE(pAllocator, pExtensions);

exit:
    return out;
}


static DkResult
dkpCreateInstance(const char *pApplicationName,
                  DkUint32 applicationMajorVersion,
                  DkUint32 applicationMinorVersion,
                  DkUint32 applicationPatchVersion,
                  const DkWindowManagerInterface *pWindowManagerInterface,
                  const VkAllocationCallbacks *pBackEndAllocator,
                  const DkAllocator *pAllocator,
                  VkInstance *pInstance)
{
    DkResult out;
    uint32_t layerCount;
    const char **ppLayerNames;
    DkBool32 layersSupported;
    uint32_t extensionCount;
    const char **ppExtensionNames;
    DkBool32 extensionsSupported;
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo createInfo;

    DK_ASSERT(pApplicationName != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pInstance != NULL);

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

    if (dkpCreateInstanceExtensionNames(pWindowManagerInterface, pAllocator,
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

    memset(&applicationInfo, 0, sizeof(VkApplicationInfo));
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

    memset(&createInfo, 0, sizeof(VkInstanceCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = ppLayerNames;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;

    switch (vkCreateInstance(&createInfo, pBackEndAllocator, pInstance)) {
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
    dkpDestroyInstanceExtensionNames(ppExtensionNames, pWindowManagerInterface,
                                     pAllocator);

layer_names_cleanup:
    dkpDestroyInstanceLayerNames(ppLayerNames, pAllocator);

exit:
    return out;
}


static void
dkpDestroyInstance(VkInstance instance,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    if (instance == NULL) {
        DK_UNUSED(pBackEndAllocator);
        return;
    }

    vkDestroyInstance(instance, pBackEndAllocator);
}


#ifdef DK_ENABLE_DEBUG_REPORT
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
    DK_UNUSED(flags);
    DK_UNUSED(objectType);
    DK_UNUSED(object);
    DK_UNUSED(location);
    DK_UNUSED(messageCode);
    DK_UNUSED(pLayerPrefix);
    DK_UNUSED(pUserData);

    fprintf(stderr, "validation layer: %s\n", pMessage);
    return VK_FALSE;
}


static DkResult
dkpCreateDebugReportCallback(VkInstance instance,
                             const VkAllocationCallbacks *pBackEndAllocator,
                             VkDebugReportCallbackEXT *pCallback)
{
    VkDebugReportCallbackCreateInfoEXT createInfo;
    PFN_vkCreateDebugReportCallbackEXT function;

    DK_ASSERT(instance != NULL);
    DK_ASSERT(pCallback != NULL);

    memset(&createInfo, 0, sizeof(VkDebugReportCallbackCreateInfoEXT));
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.pNext = NULL;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
                       | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = dkpDebugReportCallback;
    createInfo.pUserData = NULL;

    function = (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    if (function == NULL) {
        fprintf(stderr, "could not retrieve the "
                        "'vkCreateDebugReportCallbackEXT' function\n");
        return DK_ERROR;
    }

    if (function(instance, &createInfo, pBackEndAllocator, pCallback)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the debug report callback\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


static void
dkpDestroyDebugReportCallback(VkInstance instance,
                              VkDebugReportCallbackEXT callback,
                              const VkAllocationCallbacks *pBackEndAllocator)
{
    PFN_vkDestroyDebugReportCallbackEXT function;

    if (callback == VK_NULL_HANDLE) {
        DK_UNUSED(instance);
        DK_UNUSED(pBackEndAllocator);
        DK_UNUSED(function);
        return;
    }

    DK_ASSERT(instance != NULL);

    function = (PFN_vkDestroyDebugReportCallbackEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (function == NULL) {
        fprintf(stderr, "could not retrieve the "
                        "'vkDestroyDebugReportCallbackEXT' function\n");
        return;
    }

    function(instance, callback, pBackEndAllocator);
}
#endif /* DK_ENABLE_DEBUG_REPORT */


static DkResult
dkpCreateSurface(VkInstance instance,
                 const DkWindowManagerInterface *pWindowManagerInterface,
                 const VkAllocationCallbacks *pBackEndAllocator,
                 VkSurfaceKHR *pSurface)
{
    DK_ASSERT(instance != NULL);
    DK_ASSERT(pSurface != NULL);

    if (pWindowManagerInterface == NULL) {
        DK_UNUSED(instance);
        DK_UNUSED(pBackEndAllocator);

        *pSurface = VK_NULL_HANDLE;
    } else if (pWindowManagerInterface->pfnCreateSurface(
                   pWindowManagerInterface->pContext,
                   instance,
                   pBackEndAllocator,
                   pSurface)
               != DK_SUCCESS)
    {
        fprintf(stderr, "the window manager interface's 'createSurface' "
                        "callback returned an error\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


static void
dkpDestroySurface(VkInstance instance,
                  VkSurfaceKHR surface,
                  const VkAllocationCallbacks *pBackEndAllocator)
{
    if (surface == VK_NULL_HANDLE) {
        DK_UNUSED(instance);
        DK_UNUSED(pBackEndAllocator);
        return;
    }

    DK_ASSERT(instance != NULL);

    vkDestroySurfaceKHR(instance, surface, pBackEndAllocator);
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
        DK_UNUSED(pAllocator);

        *pExtensionCount = 0;
        *pppExtensionNames = NULL;
        return DK_SUCCESS;
    }

    *pExtensionCount = 1;
    *pppExtensionNames = (const char **)
        DK_ALLOCATE(pAllocator, (*pExtensionCount) * sizeof(char *));
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
dkpCheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice,
                                uint32_t requiredExtensionCount,
                                const char * const * ppRequiredExtensionNames,
                                const DkAllocator *pAllocator,
                                DkBool32 *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t extensionCount;
    VkExtensionProperties *pExtensions;

    DK_ASSERT(physicalDevice != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateDeviceExtensionProperties(physicalDevice, NULL,
                                             &extensionCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of device extension "
                        "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    pExtensions = (VkExtensionProperties *)
        DK_ALLOCATE(pAllocator, extensionCount * sizeof(VkExtensionProperties));
    if (pExtensions == NULL) {
        fprintf(stderr, "failed to allocate the device extension properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateDeviceExtensionProperties(physicalDevice, NULL,
                                             &extensionCount, pExtensions)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the device extension properties "
                        "available\n");
        out = DK_ERROR;
        goto extensions_cleanup;
    }

    *pSupported = DK_FALSE;
    for (i = 0; i < requiredExtensionCount; ++i) {
        DkBool32 found;

        found = DK_FALSE;
        for (j = 0; j < extensionCount; ++j) {
            if (strcmp(pExtensions[j].extensionName,
                       ppRequiredExtensionNames[i])
                == 0)
            {
                found = DK_TRUE;
                break;
            }
        }

        if (!found)
            goto extensions_cleanup;
    }

    *pSupported = DK_TRUE;

extensions_cleanup:
    DK_FREE(pAllocator, pExtensions);

exit:
    return out;
}


static DkResult
dkpPickDeviceQueueFamilies(VkPhysicalDevice physicalDevice,
                           VkSurfaceKHR surface,
                           const DkAllocator *pAllocator,
                           DkpQueueFamilyIndices *pQueueFamilyIndices)
{
    DkResult out;
    uint32_t i;
    uint32_t propertyCount;
    VkQueueFamilyProperties *pProperties;

    DK_ASSERT(physicalDevice != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pQueueFamilyIndices != NULL);

    out = DK_SUCCESS;

    pQueueFamilyIndices->graphics = UINT32_MAX;
    pQueueFamilyIndices->present = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &propertyCount, NULL);
    if (propertyCount == 0)
        goto exit;

    pProperties = (VkQueueFamilyProperties *)
        DK_ALLOCATE(pAllocator,
                    propertyCount * sizeof(VkQueueFamilyProperties));
    if (pProperties == NULL) {
        fprintf(stderr, "failed to allocate the queue family properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &propertyCount, pProperties);
    for (i = 0; i < propertyCount; ++i) {
        DkBool32 graphicsSupported;
        VkBool32 presentSupported;

        graphicsSupported = pProperties[i].queueCount > 0
            && pProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        if (surface == VK_NULL_HANDLE) {
            DK_UNUSED(presentSupported);

            if (graphicsSupported) {
                pQueueFamilyIndices->graphics = i;
                goto properties_cleanup;
            }
        } else {
            if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
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
                goto properties_cleanup;
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
        pImageExtent->width = DK_MAX(capabilities.minImageExtent.width,
                                     DK_MIN(capabilities.maxImageExtent.width,
                                            pDesiredImageExtent->width));
        pImageExtent->height = DK_MAX(capabilities.minImageExtent.height,
                                      DK_MIN(capabilities.maxImageExtent.height,
                                             pDesiredImageExtent->height));
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
dkpPickSwapChainProperties(VkPhysicalDevice physicalDevice,
                           VkSurfaceKHR surface,
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

    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                  &capabilities)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the surface capabilities\n");
        out = DK_ERROR;
        goto exit;
    }

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                             &formatCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the surface formats\n");
        out = DK_ERROR;
        goto exit;
    }

    pFormats = (VkSurfaceFormatKHR *)
        DK_ALLOCATE(pAllocator, formatCount * sizeof(VkSurfaceFormatKHR));
    if (pFormats == NULL) {
        fprintf(stderr, "failed to allocate the surface formats\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                             &formatCount, pFormats)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the surface formats\n");
        out = DK_ERROR;
        goto formats_cleanup;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
                                                  &presentModeCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the surface present "
                        "modes\n");
        out = DK_ERROR;
        goto formats_cleanup;
    }

    pPresentModes = (VkPresentModeKHR *)
        DK_ALLOCATE(pAllocator, presentModeCount * sizeof(VkPresentModeKHR));
    if (pPresentModes == NULL) {
        fprintf(stderr, "failed to allocate the surface present modes\n");
        out = DK_ERROR_ALLOCATION;
        goto formats_cleanup;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface,
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
                            DkBool32 *pValid)
{
    DK_ASSERT(pSwapChainProperties != NULL);

    *pValid = DK_FALSE;

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

    *pValid = DK_TRUE;
}


static DkResult
dkpCheckSwapChainSupport(VkPhysicalDevice physicalDevice,
                         VkSurfaceKHR surface,
                         const DkAllocator *pAllocator,
                         DkBool32 *pSupported)
{
    VkExtent2D imageExtent;
    DkpSwapChainProperties swapChainProperties;

    *pSupported = DK_FALSE;

    if (surface == VK_NULL_HANDLE)
        return DK_SUCCESS;

    /*
       Use dummy image extent values here as we're only interested in checking
       swap chain support rather than actually creating a valid swap chain.
    */
    imageExtent.width = 0;
    imageExtent.height = 0;
    if (dkpPickSwapChainProperties(physicalDevice, surface, &imageExtent,
                                   pAllocator, &swapChainProperties)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    dkpCheckSwapChainProperties(&swapChainProperties, DKP_LOGGING_DISABLED,
                                pSupported);
    return DK_SUCCESS;
}


static DkResult
dkpInspectPhysicalDevice(VkPhysicalDevice physicalDevice,
                         VkSurfaceKHR surface,
                         uint32_t extensionCount,
                         const char * const * ppExtensionNames,
                         const DkAllocator *pAllocator,
                         DkpQueueFamilyIndices *pQueueFamilyIndices,
                         DkBool32 *pSuitable)
{
    VkPhysicalDeviceProperties properties;
    DkBool32 extensionsSupported;
    DkBool32 swapChainSupported;

    DK_ASSERT(physicalDevice != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pQueueFamilyIndices != NULL);
    DK_ASSERT(pSuitable != NULL);

    *pSuitable = DK_FALSE;

    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return DK_SUCCESS;

    if (dkpCheckDeviceExtensionsSupport(physicalDevice, extensionCount,
                                        ppExtensionNames, pAllocator,
                                        &extensionsSupported)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (!extensionsSupported)
        return DK_SUCCESS;

    if (dkpPickDeviceQueueFamilies(physicalDevice, surface, pAllocator,
                                   pQueueFamilyIndices)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (pQueueFamilyIndices->graphics == UINT32_MAX
        || (surface != VK_NULL_HANDLE
            && pQueueFamilyIndices->present == UINT32_MAX))
    {
        return DK_SUCCESS;
    }

    if (dkpCheckSwapChainSupport(physicalDevice, surface, pAllocator,
                                 &swapChainSupported)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (surface != VK_NULL_HANDLE && !swapChainSupported)
        return DK_SUCCESS;

    *pSuitable = DK_TRUE;
    return DK_SUCCESS;
}


static DkResult
dkpPickPhysicalDevice(VkInstance instance,
                      VkSurfaceKHR surface,
                      uint32_t extensionCount,
                      const char * const * ppExtensionNames,
                      const DkAllocator *pAllocator,
                      DkpQueueFamilyIndices *pQueueFamilyIndices,
                      VkPhysicalDevice *pPhysicalDevice)
{
    DkResult out;
    uint32_t i;
    uint32_t physicalDeviceCount;
    VkPhysicalDevice *pPhysicalDevices;

    DK_ASSERT(instance != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pQueueFamilyIndices != NULL);
    DK_ASSERT(pPhysicalDevice != NULL);

    out = DK_SUCCESS;

    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the physical devices\n");
        out = DK_ERROR;
        goto exit;
    }

    pPhysicalDevices = (VkPhysicalDevice *)
        DK_ALLOCATE(pAllocator, physicalDeviceCount * sizeof(VkPhysicalDevice));
    if (pPhysicalDevices == NULL) {
        fprintf(stderr, "failed to allocate the physical devices\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                   pPhysicalDevices)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the physical devices\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

    *pPhysicalDevice = NULL;
    for (i = 0; i < physicalDeviceCount; ++i) {
        DkBool32 suitable;

        if (dkpInspectPhysicalDevice(pPhysicalDevices[i], surface,
                                     extensionCount, ppExtensionNames,
                                     pAllocator, pQueueFamilyIndices,
                                     &suitable)
            != DK_SUCCESS)
        {
            out = DK_ERROR;
            goto physical_devices_cleanup;
        }

        if (suitable) {
            *pPhysicalDevice = pPhysicalDevices[i];
            break;
        }
    }

    if (*pPhysicalDevice == NULL) {
        fprintf(stderr, "could not find a suitable physical device\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

physical_devices_cleanup:
    DK_FREE(pAllocator, pPhysicalDevices);

exit:
    return out;
}


static DkResult
dkpCreateDevice(VkInstance instance,
                VkSurfaceKHR surface,
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
    DkUint32 queueInfoCount;
    VkDeviceQueueCreateInfo *pQueueInfos;
    VkDeviceCreateInfo createInfo;

    DK_ASSERT(instance != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pDevice != NULL);

    out = DK_SUCCESS;

    presentSupport = surface == VK_NULL_HANDLE
        ? DKP_PRESENT_SUPPORT_DISABLED
        : DKP_PRESENT_SUPPORT_ENABLED;

    if (dkpCreateDeviceExtensionNames(presentSupport, pAllocator,
                                      &extensionCount, &ppExtensionNames)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto exit;
    }

    if (dkpPickPhysicalDevice(instance, surface, extensionCount,
                              ppExtensionNames, pAllocator,
                              &pDevice->queueFamilyIndices,
                              &pDevice->physical)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto extension_names_cleanup;
    }

    queueCount = 1;
    pQueuePriorities = (float *)
        DK_ALLOCATE(pAllocator, queueCount * sizeof(float));
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
        DK_ALLOCATE(pAllocator,
                    queueInfoCount * sizeof(VkDeviceQueueCreateInfo));
    if (pQueueInfos == NULL) {
        fprintf(stderr, "failed to allocate the device queue infos\n");
        out = DK_ERROR_ALLOCATION;
        goto queue_priorities_cleanup;
    }

    memset(&pQueueInfos[0], 0, sizeof(VkDeviceQueueCreateInfo));
    pQueueInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pQueueInfos[0].pNext = NULL;
    pQueueInfos[0].flags = 0;
    pQueueInfos[0].queueFamilyIndex = pDevice->queueFamilyIndices.graphics;
    pQueueInfos[0].queueCount = queueCount;
    pQueueInfos[0].pQueuePriorities = pQueuePriorities;

    if (queueInfoCount == 2) {
        memset(&pQueueInfos[1], 0, sizeof(VkDeviceQueueCreateInfo));
        pQueueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        pQueueInfos[1].pNext = NULL;
        pQueueInfos[1].flags = 0;
        pQueueInfos[1].queueFamilyIndex = pDevice->queueFamilyIndices.present;
        pQueueInfos[1].queueCount = queueCount;
        pQueueInfos[1].pQueuePriorities = pQueuePriorities;
    }

    memset(&createInfo, 0, sizeof(VkDeviceCreateInfo));
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

    switch (vkCreateDevice(pDevice->physical, &createInfo, pBackEndAllocator,
                           &pDevice->logical))
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
            fprintf(stderr, "failed to create the device\n");
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
    if (pDevice == NULL || pDevice->logical == NULL) {
        DK_UNUSED(pBackEndAllocator);
        return;
    }

    vkDestroyDevice(pDevice->logical, pBackEndAllocator);
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
        vkGetDeviceQueue(pDevice->logical,
                         pDevice->queueFamilyIndices.graphics,
                         queueIndex,
                         &pQueues->graphics);
    else
        pQueues->graphics = NULL;

    if (pDevice->queueFamilyIndices.present != UINT32_MAX)
        vkGetDeviceQueue(pDevice->logical,
                         pDevice->queueFamilyIndices.present,
                         queueIndex,
                         &pQueues->present);
    else
        pQueues->present = NULL;

    return DK_SUCCESS;
}


static DkResult
dkpCreateSemaphores(const DkpDevice *pDevice,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    DkpSemaphores *pSemaphores)
{
    DkResult out;
    VkSemaphoreCreateInfo createInfo;

    DK_ASSERT(pSemaphores != NULL);

    out = DK_SUCCESS;

    memset(&createInfo, 0, sizeof(VkSemaphoreCreateInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    if (vkCreateSemaphore(pDevice->logical, &createInfo, pBackEndAllocator,
                          &pSemaphores->imageAcquired)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the 'image acquired' semaphore\n");
        out = DK_ERROR;
        goto exit;
    }

    if (vkCreateSemaphore(pDevice->logical, &createInfo, pBackEndAllocator,
                          &pSemaphores->presentCompleted)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the 'present completed' semaphore\n");
        out = DK_ERROR;
        goto image_acquired_semaphore_cleanup;
    }

    goto exit;

image_acquired_semaphore_cleanup:
    vkDestroySemaphore(pDevice->logical, pSemaphores->imageAcquired,
                       pBackEndAllocator);

exit:
    return out;
}


static void
dkpDestroySemaphores(const DkpDevice *pDevice,
                     DkpSemaphores *pSemaphores,
                     const VkAllocationCallbacks *pBackEndAllocator)
{
    if (pSemaphores == NULL) {
        DK_UNUSED(pDevice);
        DK_UNUSED(pBackEndAllocator);
        return;
    }

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logical != NULL);

    if (pSemaphores->imageAcquired != VK_NULL_HANDLE)
        vkDestroySemaphore(pDevice->logical, pSemaphores->imageAcquired,
                           pBackEndAllocator);

    if (pSemaphores->presentCompleted != VK_NULL_HANDLE)
        vkDestroySemaphore(pDevice->logical, pSemaphores->presentCompleted,
                           pBackEndAllocator);
}


static DkResult
dkpCreateSwapChain(const DkpDevice *pDevice,
                   VkSurfaceKHR surface,
                   const VkExtent2D *pDesiredImageExtent,
                   VkSwapchainKHR oldSwapChainHandle,
                   const VkAllocationCallbacks *pBackEndAllocator,
                   const DkAllocator *pAllocator,
                   DkpSwapChain *pSwapChain)
{
    DkResult out;
    DkpSwapChainProperties swapChainProperties;
    DkBool32 valid;
    VkSharingMode imageSharingMode;
    uint32_t queueFamilyIndexCount;
    uint32_t *pQueueFamilyIndices;
    VkSwapchainCreateInfoKHR createInfo;

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSwapChain != NULL);

    out = DK_SUCCESS;
    pSwapChain->handle = VK_NULL_HANDLE;
    pSwapChain->imageCount = 0;
    pSwapChain->pImages = NULL;

    if (surface == VK_NULL_HANDLE)
        goto exit;

    if (dkpPickSwapChainProperties(pDevice->physical, surface,
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
            DK_ALLOCATE(pAllocator, queueFamilyIndexCount * sizeof (uint32_t));
        if (pQueueFamilyIndices == NULL) {
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

    memset(&createInfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.surface = surface;
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

    switch (vkCreateSwapchainKHR(pDevice->logical, &createInfo,
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

    if (vkGetSwapchainImagesKHR(pDevice->logical, pSwapChain->handle,
                                &pSwapChain->imageCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of swap chain images "
                        "available\n");
        out = DK_ERROR;
        goto queue_family_indices_cleanup;
    }

    pSwapChain->pImages = (VkImage *)
        DK_ALLOCATE(pAllocator, pSwapChain->imageCount * sizeof(VkImage));
    if (pSwapChain->pImages == NULL) {
        fprintf(stderr, "failed to allocate the swap chain images\n");
        out = DK_ERROR_ALLOCATION;
        goto queue_family_indices_cleanup;
    }

queue_family_indices_cleanup:
    if (pQueueFamilyIndices != NULL)
        DK_FREE(pAllocator, pQueueFamilyIndices);

exit:
    if (oldSwapChainHandle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(pDevice->logical, oldSwapChainHandle,
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
    if (pSwapChain == NULL) {
        DK_UNUSED(pDevice);
        DK_UNUSED(pBackEndAllocator);
        DK_UNUSED(pAllocator);
        return;
    }

    DK_ASSERT(pDevice != NULL);
    DK_ASSERT(pDevice->logical != NULL);
    DK_ASSERT(pAllocator != NULL);

    if (pSwapChain->pImages != NULL)
        DK_FREE(pAllocator, pSwapChain->pImages);

    if (pSwapChain->handle != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(pDevice->logical, pSwapChain->handle,
                              pBackEndAllocator);
}


DkResult
dkpRecreateRendererSwapChain(DkRenderer *pRenderer)
{
    if (dkpCreateSwapChain(&pRenderer->device,
                           pRenderer->surface,
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


DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DkResult out;

    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(ppRenderer != NULL);

    out = DK_SUCCESS;

    if (pAllocator == NULL)
        dkpGetDefaultAllocator(&pAllocator);

    *ppRenderer = (DkRenderer *) DK_ALLOCATE(pAllocator, sizeof(DkRenderer));
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
                          pCreateInfo->applicationMajorVersion,
                          pCreateInfo->applicationMinorVersion,
                          pCreateInfo->applicationPatchVersion,
                          pCreateInfo->pWindowManagerInterface,
                          (*ppRenderer)->pBackEndAllocator,
                          (*ppRenderer)->pAllocator,
                          &(*ppRenderer)->instance)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto renderer_cleanup;
    }

#ifdef DK_ENABLE_DEBUG_REPORT
    if (dkpCreateDebugReportCallback((*ppRenderer)->instance,
                                     (*ppRenderer)->pBackEndAllocator,
                                     &(*ppRenderer)->debugReportCallback)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto instance_cleanup;
    }
#endif /* DK_ENABLE_DEBUG_REPORT */

    if (dkpCreateSurface((*ppRenderer)->instance,
                         pCreateInfo->pWindowManagerInterface,
                         (*ppRenderer)->pBackEndAllocator,
                         &(*ppRenderer)->surface)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto debug_report_callback_cleanup;
    }

    if (dkpCreateDevice((*ppRenderer)->instance,
                        (*ppRenderer)->surface,
                        (*ppRenderer)->pBackEndAllocator,
                        (*ppRenderer)->pAllocator,
                        &(*ppRenderer)->device)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto surface_cleanup;
    }

    if (dkpGetDeviceQueues(&(*ppRenderer)->device,
                           &(*ppRenderer)->queues)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto device_cleanup;
    }

    if (dkpCreateSemaphores(&(*ppRenderer)->device,
                            (*ppRenderer)->pBackEndAllocator,
                            &(*ppRenderer)->semaphores)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto device_cleanup;
    }

    if (dkpCreateSwapChain(&(*ppRenderer)->device,
                           (*ppRenderer)->surface,
                           &(*ppRenderer)->surfaceExtent,
                           VK_NULL_HANDLE,
                           (*ppRenderer)->pBackEndAllocator,
                           (*ppRenderer)->pAllocator,
                           &(*ppRenderer)->swapChain)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto semaphores_cleanup;
    }

    goto exit;

semaphores_cleanup:
    dkpDestroySemaphores(&(*ppRenderer)->device, &(*ppRenderer)->semaphores,
                         (*ppRenderer)->pBackEndAllocator);

device_cleanup:
    dkpDestroyDevice(&(*ppRenderer)->device, (*ppRenderer)->pBackEndAllocator);

surface_cleanup:
    dkpDestroySurface((*ppRenderer)->instance,
                      (*ppRenderer)->surface,
                      (*ppRenderer)->pBackEndAllocator);

debug_report_callback_cleanup:
#ifdef DK_ENABLE_DEBUG_REPORT
    dkpDestroyDebugReportCallback((*ppRenderer)->instance,
                                  (*ppRenderer)->debugReportCallback,
                                  (*ppRenderer)->pBackEndAllocator);
#else
    goto instance_cleanup;
#endif /* DK_ENABLE_DEBUG_REPORT */

instance_cleanup:
    dkpDestroyInstance((*ppRenderer)->instance,
                       (*ppRenderer)->pBackEndAllocator);

renderer_cleanup:
    DK_FREE((*ppRenderer)->pAllocator, (*ppRenderer));
    *ppRenderer = NULL;

exit:
    return out;
}


void
dkDestroyRenderer(DkRenderer *pRenderer,
                  const DkAllocator *pAllocator)
{
    if (pRenderer == NULL)
        return;

    if (pAllocator == NULL)
        dkpGetDefaultAllocator(&pAllocator);

    dkpDestroySwapChain(&pRenderer->device, &pRenderer->swapChain,
                        pRenderer->pBackEndAllocator, pAllocator);
    dkpDestroySemaphores(&pRenderer->device, &pRenderer->semaphores,
                         pRenderer->pBackEndAllocator);
    dkpDestroyDevice(&pRenderer->device, pRenderer->pBackEndAllocator);
    dkpDestroySurface(pRenderer->instance,
                      pRenderer->surface,
                      pRenderer->pBackEndAllocator);
#ifdef DK_ENABLE_DEBUG_REPORT
    dkpDestroyDebugReportCallback(pRenderer->instance,
                                  pRenderer->debugReportCallback,
                                  pRenderer->pBackEndAllocator);
#endif /* DK_ENABLE_DEBUG_REPORT */
    dkpDestroyInstance(pRenderer->instance, pRenderer->pBackEndAllocator);
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
