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


struct DkRenderer {
    const DkAllocator *pAllocator;
    const VkAllocationCallbacks *pBackEndAllocator;
    VkInstance instance;
    VkSurfaceKHR surface;
#ifdef DK_ENABLE_DEBUG_REPORT
    VkDebugReportCallbackEXT debugReportCallback;
#endif /* DK_ENABLE_DEBUG_REPORT */
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentQueueFamilyIndex;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};


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
        return DK_ERROR;
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
dkpCheckInstanceLayersSupport(const DkAllocator *pAllocator,
                              uint32_t requiredLayerCount,
                              const char **ppRequiredLayerNames,
                              DkBool32 *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t layerCount;
    VkLayerProperties *pLayers;

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(ppRequiredLayerNames != NULL);
    DK_ASSERT(pSupported != NULL);

    if (vkEnumerateInstanceLayerProperties(&layerCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of instance layer "
                        "properties available\n");
        return DK_ERROR;
    }

    pLayers = (VkLayerProperties *)
        DK_ALLOCATE(pAllocator, layerCount * sizeof(VkLayerProperties));
    if (pLayers == NULL) {
        fprintf(stderr, "failed to allocate the instance layer properties\n");
        return DK_ERROR_ALLOCATION;
    }

    out = DK_SUCCESS;
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
    ppBuffer = *pppExtensionNames;
    *pppExtensionNames = (const char **)
        DK_ALLOCATE(pAllocator, ((*pExtensionCount) + 1) * sizeof(char *));
    if (*pppExtensionNames == NULL) {
        fprintf(stderr, "failed to allocate the instance extension names\n");
        return DK_ERROR_ALLOCATION;
    }

    if (ppBuffer != NULL)
        memcpy(*pppExtensionNames, ppBuffer,
               (*pExtensionCount) * sizeof(char *));

    (*pppExtensionNames)[*pExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    *pExtensionCount += 1;

    if (pWindowManagerInterface != NULL)
        pWindowManagerInterface->pfnDestroyInstanceExtensionNames(
            pWindowManagerInterface->pContext,
            ppBuffer);
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
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo createInfo;

    DK_ASSERT(pApplicationName != NULL);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pInstance != NULL);

    if (dkpCreateInstanceLayerNames(pAllocator, &layerCount, &ppLayerNames)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    out = DK_SUCCESS;
    if (dkpCheckInstanceLayersSupport(pAllocator, layerCount, ppLayerNames,
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
            goto extension_names_cleanup;
        case VK_ERROR_LAYER_NOT_PRESENT:
            fprintf(stderr, "could not find the requested layers\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            fprintf(stderr, "could not find the requested extensions\n");
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
    return out;
}


static void
dkpDestroyInstance(VkInstance instance,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(instance != VK_NULL_HANDLE);

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

    DK_ASSERT(instance != VK_NULL_HANDLE);
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

    DK_ASSERT(instance != VK_NULL_HANDLE);
    DK_ASSERT(callback != VK_NULL_HANDLE);

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
    DK_ASSERT(instance != VK_NULL_HANDLE);
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
                                const char **ppRequiredExtensionNames,
                                const DkAllocator *pAllocator,
                                DkBool32 *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t extensionCount;
    VkExtensionProperties *pExtensions;

    DK_ASSERT(physicalDevice != VK_NULL_HANDLE);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    if (vkEnumerateDeviceExtensionProperties(physicalDevice, NULL,
                                             &extensionCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of device extension "
                        "properties available\n");
        return DK_ERROR;
    }

    pExtensions = (VkExtensionProperties *)
        DK_ALLOCATE(pAllocator, extensionCount * sizeof(VkExtensionProperties));
    if (pExtensions == NULL) {
        fprintf(stderr, "failed to allocate the device extension properties\n");
        return DK_ERROR_ALLOCATION;
    }

    out = DK_SUCCESS;
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
    return out;
}


static DkResult
dkpPickDeviceQueueFamilies(VkPhysicalDevice physicalDevice,
                           VkSurfaceKHR surface,
                           const DkAllocator *pAllocator,
                           uint32_t *pGraphicsQueueFamilyIndex,
                           uint32_t *pPresentQueueFamilyIndex)
{
    DkResult out;
    uint32_t i;
    uint32_t propertyCount;
    VkQueueFamilyProperties *pProperties;

    DK_ASSERT(physicalDevice != VK_NULL_HANDLE);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pGraphicsQueueFamilyIndex != NULL);
    DK_ASSERT(pPresentQueueFamilyIndex != NULL);

    *pGraphicsQueueFamilyIndex = UINT32_MAX;
    *pPresentQueueFamilyIndex = UINT32_MAX;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &propertyCount, NULL);
    if (propertyCount == 0)
        return DK_SUCCESS;

    pProperties = (VkQueueFamilyProperties *)
        DK_ALLOCATE(pAllocator,
                    propertyCount * sizeof(VkQueueFamilyProperties));
    if (pProperties == NULL) {
        fprintf(stderr, "failed to allocate the queue family properties\n");
        return DK_ERROR_ALLOCATION;
    }

    out = DK_SUCCESS;
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
                *pGraphicsQueueFamilyIndex = i;
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
                *pGraphicsQueueFamilyIndex = i;
                *pPresentQueueFamilyIndex = i;
                goto properties_cleanup;
            }

            if (graphicsSupported
                && *pGraphicsQueueFamilyIndex == UINT32_MAX)
            {
                *pGraphicsQueueFamilyIndex = i;
            } else if (presentSupported
                       && *pPresentQueueFamilyIndex == UINT32_MAX)
            {
                *pPresentQueueFamilyIndex = i;
            }
        }
    }

properties_cleanup:
    DK_FREE(pAllocator, pProperties);
    return out;
}


static DkResult
dkpInspectPhysicalDevice(VkPhysicalDevice physicalDevice,
                         VkSurfaceKHR surface,
                         uint32_t extensionCount,
                         const char **ppExtensionNames,
                         const DkAllocator *pAllocator,
                         uint32_t *pGraphicsQueueFamilyIndex,
                         uint32_t *pPresentQueueFamilyIndex,
                         DkBool32 *pSuitable)
{
    DkBool32 extensionsSupported;
    VkPhysicalDeviceProperties properties;

    DK_ASSERT(physicalDevice != VK_NULL_HANDLE);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSuitable != NULL);
    DK_ASSERT(pGraphicsQueueFamilyIndex != NULL);
    DK_ASSERT(pPresentQueueFamilyIndex != NULL);

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

    if (!extensionsSupported) {
        fprintf(stderr, "one or more device extensions are not supported\n");
        return DK_ERROR;
    }

    if (dkpPickDeviceQueueFamilies(physicalDevice, surface, pAllocator,
                                   pGraphicsQueueFamilyIndex,
                                   pPresentQueueFamilyIndex)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    if (*pGraphicsQueueFamilyIndex == UINT32_MAX
        || (surface != VK_NULL_HANDLE
            && *pPresentQueueFamilyIndex == UINT32_MAX))
    {
        return DK_SUCCESS;
    }

    *pSuitable = DK_TRUE;
    return DK_SUCCESS;
}


static DkResult
dkpPickPhysicalDevice(VkInstance instance,
                      VkSurfaceKHR surface,
                      uint32_t extensionCount,
                      const char **ppExtensionNames,
                      const DkAllocator *pAllocator,
                      uint32_t *pGraphicsQueueFamilyIndex,
                      uint32_t *pPresentQueueFamilyIndex,
                      VkPhysicalDevice *pPhysicalDevice)
{
    DkResult out;
    uint32_t i;
    uint32_t physicalDeviceCount;
    VkPhysicalDevice *pPhysicalDevices;

    DK_ASSERT(instance != VK_NULL_HANDLE);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pGraphicsQueueFamilyIndex != NULL);
    DK_ASSERT(pPresentQueueFamilyIndex != NULL);
    DK_ASSERT(pPhysicalDevice != NULL);

    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not count the number of the physical devices\n");
        return DK_ERROR;
    }

    pPhysicalDevices = (VkPhysicalDevice *)
        DK_ALLOCATE(pAllocator, physicalDeviceCount * sizeof(VkPhysicalDevice));
    if (pPhysicalDevices == NULL) {
        fprintf(stderr, "failed to allocate the physical devices\n");
        return DK_ERROR_ALLOCATION;
    }

    out = DK_SUCCESS;
    if (vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                   pPhysicalDevices)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not enumerate the physical devices\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

    *pPhysicalDevice = VK_NULL_HANDLE;
    for (i = 0; i < physicalDeviceCount; ++i) {
        DkBool32 suitable;

        if (dkpInspectPhysicalDevice(pPhysicalDevices[i], surface,
                                     extensionCount, ppExtensionNames,
                                     pAllocator,
                                     pGraphicsQueueFamilyIndex,
                                     pPresentQueueFamilyIndex,
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

    if (*pPhysicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "could not find a suitable physical device\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

physical_devices_cleanup:
    DK_FREE(pAllocator, pPhysicalDevices);
    return out;
}


static DkResult
dkpCreateDevice(VkInstance instance,
                VkSurfaceKHR surface,
                const DkAllocator *pAllocator,
                const VkAllocationCallbacks *pBackEndAllocator,
                uint32_t *pGraphicsQueueFamilyIndex,
                uint32_t *pPresentQueueFamilyIndex,
                VkPhysicalDevice *pPhysicalDevice,
                VkDevice *pDevice)
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

    DK_ASSERT(instance != VK_NULL_HANDLE);
    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pGraphicsQueueFamilyIndex != NULL);
    DK_ASSERT(pPresentQueueFamilyIndex != NULL);
    DK_ASSERT(pPhysicalDevice != NULL);
    DK_ASSERT(pDevice != NULL);

    presentSupport = surface == VK_NULL_HANDLE
        ? DKP_PRESENT_SUPPORT_DISABLED
        : DKP_PRESENT_SUPPORT_ENABLED;

    if (dkpCreateDeviceExtensionNames(presentSupport, pAllocator,
                                      &extensionCount, &ppExtensionNames)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    out = DK_SUCCESS;
    if (dkpPickPhysicalDevice(instance, surface, extensionCount,
                              ppExtensionNames, pAllocator,
                              pGraphicsQueueFamilyIndex,
                              pPresentQueueFamilyIndex,
                              pPhysicalDevice)
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

    queueInfoCount = *pGraphicsQueueFamilyIndex == *pPresentQueueFamilyIndex
        ? 1 : 2;
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
    pQueueInfos[0].queueFamilyIndex = *pGraphicsQueueFamilyIndex;
    pQueueInfos[0].queueCount = queueCount;
    pQueueInfos[0].pQueuePriorities = pQueuePriorities;

    if (*pGraphicsQueueFamilyIndex != *pPresentQueueFamilyIndex) {
        memset(&pQueueInfos[1], 0, sizeof(VkDeviceQueueCreateInfo));
        pQueueInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        pQueueInfos[1].pNext = NULL;
        pQueueInfos[1].flags = 0;
        pQueueInfos[1].queueFamilyIndex = *pPresentQueueFamilyIndex;
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

    if (vkCreateDevice(*pPhysicalDevice, &createInfo, pBackEndAllocator,
                       pDevice)
        != VK_SUCCESS)
    {
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
    return out;
}


static void
dkpDestroyDevice(VkDevice device,
                 const VkAllocationCallbacks *pBackEndAllocator)
{
    vkDestroyDevice(device, pBackEndAllocator);
}


DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DkResult out;

    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(ppRenderer != NULL);

    if (pAllocator == NULL)
        dkpGetDefaultAllocator(&pAllocator);

    *ppRenderer = (DkRenderer *) DK_ALLOCATE(pAllocator, sizeof(DkRenderer));
    if (*ppRenderer == NULL) {
        fprintf(stderr, "failed to allocate the renderer\n");
        return DK_ERROR_ALLOCATION;
    }

    (*ppRenderer)->pAllocator = pAllocator;
    (*ppRenderer)->pBackEndAllocator = pCreateInfo->pBackEndAllocator;
    (*ppRenderer)->instance = VK_NULL_HANDLE;
#ifdef DK_ENABLE_DEBUG_REPORT
    (*ppRenderer)->debugReportCallback = VK_NULL_HANDLE;
#endif /* DK_ENABLE_DEBUG_REPORT */
    (*ppRenderer)->surface = VK_NULL_HANDLE;
    (*ppRenderer)->graphicsQueueFamilyIndex = UINT32_MAX;
    (*ppRenderer)->presentQueueFamilyIndex = UINT32_MAX;
    (*ppRenderer)->physicalDevice = VK_NULL_HANDLE;
    (*ppRenderer)->device = VK_NULL_HANDLE;

    out = DK_SUCCESS;
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
                        (*ppRenderer)->pAllocator,
                        (*ppRenderer)->pBackEndAllocator,
                        &(*ppRenderer)->graphicsQueueFamilyIndex,
                        &(*ppRenderer)->presentQueueFamilyIndex,
                        &(*ppRenderer)->physicalDevice,
                        &(*ppRenderer)->device)
        != DK_SUCCESS)
    {
        out = DK_ERROR;
        goto surface_cleanup;
    }

    return out;

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
    return out;
}


void
dkDestroyRenderer(DkRenderer *pRenderer)
{
    if (pRenderer == NULL)
        return;

    dkpDestroyDevice(pRenderer->device, pRenderer->pBackEndAllocator);

    dkpDestroySurface(pRenderer->instance,
                      pRenderer->surface,
                      pRenderer->pBackEndAllocator);

#ifdef DK_ENABLE_DEBUG_REPORT
    dkpDestroyDebugReportCallback(pRenderer->instance,
                                  pRenderer->debugReportCallback,
                                  pRenderer->pBackEndAllocator);
#endif /* DK_ENABLE_DEBUG_REPORT */

    dkpDestroyInstance(pRenderer->instance, pRenderer->pBackEndAllocator);

    DK_FREE(pRenderer->pAllocator, pRenderer);
}
