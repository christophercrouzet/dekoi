#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "memory.h"
#include "renderer.h"
#include "internal/assert.h"
#include "internal/memory.h"


struct DkRenderer {
    const DkAllocator *pAllocator;
    const VkAllocationCallbacks *pBackEndAllocator;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugReportCallbackEXT debugReportCallback;
};


#ifdef DK_DEBUG
#define DK_ENABLE_VALIDATION_LAYERS
static const uint32_t dkpRequiredValidationLayerCount = 1;
static const char * const ppDkpRequiredValidationLayerNames[] = {
    "VK_LAYER_LUNARG_standard_validation"
};
#endif /* DK_DEBUG */


#ifdef DK_ENABLE_VALIDATION_LAYERS
static DkResult
dkpCheckValidationLayersSupport(const DkAllocator *pAllocator,
                                DkBool32 *pSupported)
{
    DkResult out;
    uint32_t i;
    uint32_t j;
    uint32_t layerCount;
    VkLayerProperties *pLayers;

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    *pSupported = DK_FALSE;

    if (vkEnumerateInstanceLayerProperties(&layerCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of instance layer "
                        "properties available\n");
        return DK_ERROR;
    }

    out = DK_SUCCESS;
    pLayers = (VkLayerProperties *)
        DK_ALLOCATE(pAllocator, layerCount * sizeof(VkLayerProperties));
    if (vkEnumerateInstanceLayerProperties(&layerCount, pLayers)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the instance layer properties "
                        "available\n");
        out = DK_ERROR;
        goto exit;
    }

    for (i = 0; i < dkpRequiredValidationLayerCount; ++i) {
        DkBool32 found;

        found = DK_FALSE;
        for (j = 0; j < layerCount; ++j) {
            if (strcmp(pLayers[j].layerName,
                       ppDkpRequiredValidationLayerNames[i])
                == 0)
            {
                found = DK_TRUE;
                break;
            }
        }

        if (!found)
            goto exit;
    }

    *pSupported = DK_TRUE;

exit:
    DK_FREE(pAllocator, pLayers);
    return out;
}
#endif /* DK_ENABLE_VALIDATION_LAYERS */


static DkResult
dkpCreateInstanceExtensionNames(
    const DkWindowManagerInterface *pWindowManagerInterface,
    const DkAllocator *pAllocator,
    uint32_t *pExtensionCount,
    const char ***pppExtensionNames)
{
#ifdef DK_ENABLE_VALIDATION_LAYERS
    const char **ppBuffer;
#else
    DK_UNUSED(pAllocator);
#endif /* DK_ENABLE_VALIDATION_LAYERS */

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

#ifdef DK_ENABLE_VALIDATION_LAYERS
    ppBuffer = *pppExtensionNames;
    *pppExtensionNames = (const char **)
        DK_ALLOCATE(pAllocator, ((*pExtensionCount) + 1) * sizeof(char *));

    if (ppBuffer != NULL)
        memcpy(*pppExtensionNames, ppBuffer,
               (*pExtensionCount) * sizeof(char *));

    (*pppExtensionNames)[*pExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    *pExtensionCount += 1;

    if (pWindowManagerInterface != NULL)
        pWindowManagerInterface->pfnDestroyInstanceExtensionNames(
            pWindowManagerInterface->pContext,
            ppBuffer);
#endif /* DK_ENABLE_VALIDATION_LAYERS */

    return DK_SUCCESS;
}


static void
dkpDestroyInstanceExtensionNames(
    const char **ppExtensionNames,
    const DkWindowManagerInterface *pWindowManagerInterface,
    const DkAllocator *pAllocator)
{
#ifdef DK_ENABLE_VALIDATION_LAYERS
    DK_UNUSED(pWindowManagerInterface);

    DK_ASSERT(pAllocator != NULL);

    DK_FREE(pAllocator, ppExtensionNames);
#else
    DK_UNUSED(pAllocator);

    if (pWindowManagerInterface != NULL)
        pWindowManagerInterface->pfnDestroyInstanceExtensionNames(
            pWindowManagerInterface->pContext,
            ppExtensionNames);
#endif /* DK_ENABLE_VALIDATION_LAYERS */
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
#ifdef DK_ENABLE_VALIDATION_LAYERS
    DkBool32 validationLayersSupported;
#endif /* DK_ENABLE_VALIDATION_LAYERS */
    uint32_t extensionCount;
    const char **ppExtensionNames;
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo createInfo;

    DK_ASSERT(pApplicationName != NULL);
    DK_ASSERT(pAllocator != NULL);

#ifdef DK_ENABLE_VALIDATION_LAYERS
    if (dkpCheckValidationLayersSupport(pAllocator, &validationLayersSupported)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    } else if (!validationLayersSupported) {
        fprintf(stderr, "one or more validation layers are not supported\n");
        return DK_ERROR;
    }
#endif /* DK_ENABLE_VALIDATION_LAYERS */

    if (dkpCreateInstanceExtensionNames(pWindowManagerInterface, pAllocator,
                                        &extensionCount, &ppExtensionNames)
        != DK_SUCCESS)
    {
        return DK_ERROR;
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
#ifdef DK_ENABLE_VALIDATION_LAYERS
    createInfo.enabledLayerCount = dkpRequiredValidationLayerCount;
    createInfo.ppEnabledLayerNames = ppDkpRequiredValidationLayerNames;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;
#endif /* DK_ENABLE_VALIDATION_LAYERS */
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;

    out = DK_SUCCESS;
    switch (vkCreateInstance(&createInfo, pBackEndAllocator, pInstance)) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            fprintf(stderr, "could not find the requested layers\n");
            out = DK_ERROR;
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            fprintf(stderr, "could not find the requested extensions\n");
            out = DK_ERROR;
            break;
        default:
            fprintf(stderr, "failed to create the renderer instance\n");
            out = DK_ERROR;
            break;
    }

    dkpDestroyInstanceExtensionNames(ppExtensionNames, pWindowManagerInterface,
                                     pAllocator);
    return out;
}


static void
dkpDestroyInstance(VkInstance instance,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    DK_ASSERT(instance != VK_NULL_HANDLE);

    vkDestroyInstance(instance, pBackEndAllocator);
}


#ifdef DK_ENABLE_VALIDATION_LAYERS
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
#endif /* DK_ENABLE_VALIDATION_LAYERS */


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


DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(ppRenderer != NULL);

    if (pAllocator == NULL)
        dkpGetDefaultAllocator(&pAllocator);

    *ppRenderer = (DkRenderer *) DK_ALLOCATE(pAllocator, sizeof(DkRenderer));

    (*ppRenderer)->pAllocator = pAllocator;
    (*ppRenderer)->pBackEndAllocator = pCreateInfo->pBackEndAllocator;
    (*ppRenderer)->instance = VK_NULL_HANDLE;
    (*ppRenderer)->debugReportCallback = VK_NULL_HANDLE;
    (*ppRenderer)->surface = VK_NULL_HANDLE;

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
        goto instance_error;
    }

#ifdef DK_ENABLE_VALIDATION_LAYERS
    if (dkpCreateDebugReportCallback((*ppRenderer)->instance,
                                     (*ppRenderer)->pBackEndAllocator,
                                     &(*ppRenderer)->debugReportCallback)
        != DK_SUCCESS)
    {
        goto debug_report_callback_error;
    }
#endif /* DK_ENABLE_VALIDATION_LAYERS */

    if (dkpCreateSurface((*ppRenderer)->instance,
                         pCreateInfo->pWindowManagerInterface,
                         (*ppRenderer)->pBackEndAllocator,
                         &(*ppRenderer)->surface)
        != DK_SUCCESS)
    {
        goto create_surface_error;
    }

    return DK_SUCCESS;

create_surface_error:
    dkpDestroySurface((*ppRenderer)->instance,
                      (*ppRenderer)->surface,
                      (*ppRenderer)->pBackEndAllocator);

#ifdef DK_ENABLE_VALIDATION_LAYERS
debug_report_callback_error:
    dkpDestroyDebugReportCallback((*ppRenderer)->instance,
                                  (*ppRenderer)->debugReportCallback,
                                  (*ppRenderer)->pBackEndAllocator);
#endif /* DK_ENABLE_VALIDATION_LAYERS */

instance_error:
    dkpDestroyInstance((*ppRenderer)->instance,
                       (*ppRenderer)->pBackEndAllocator);

    DK_FREE((*ppRenderer)->pAllocator, (*ppRenderer));
    *ppRenderer = NULL;
    return DK_ERROR;
}


void
dkDestroyRenderer(DkRenderer *pRenderer)
{
    if (pRenderer == NULL)
        return;

    dkpDestroySurface(pRenderer->instance,
                      pRenderer->surface,
                      pRenderer->pBackEndAllocator);

#ifdef DK_ENABLE_VALIDATION_LAYERS
    dkpDestroyDebugReportCallback(pRenderer->instance,
                                  pRenderer->debugReportCallback,
                                  pRenderer->pBackEndAllocator);
#endif /* DK_ENABLE_VALIDATION_LAYERS */

    dkpDestroyInstance(pRenderer->instance, pRenderer->pBackEndAllocator);

    DK_FREE(pRenderer->pAllocator, pRenderer);
}
