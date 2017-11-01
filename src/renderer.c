#include <stdio.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "memory.h"
#include "renderer.h"
#include "internal/assert.h"
#include "internal/memory.h"


struct DkRenderer {
    const DkAllocator *pAllocator;
    const VkAllocationCallbacks *pBackEndAllocator;
    DkUint32 extensionCount;
    const char **ppExtensionNames;
    VkInstance instance;
};


#ifdef DK_DEBUG
#define DK_ENABLE_VALIDATION_LAYERS
static const DkUint32 validationLayerCount = 1;
static const char * const ppValidationLayerNames[] = {
    "VK_LAYER_LUNARG_standard_validation"
};
#endif


#ifdef DK_ENABLE_VALIDATION_LAYERS
static void
createDebugExtensionNames(const DkAllocator *pAllocator,
                          DkUint32 *pExtensionCount,
                          const char ***pppExtensionNames)
{
    const char **ppBuffer;

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pExtensionCount != NULL);
    DK_ASSERT(pppExtensionNames != NULL);

    ppBuffer = *pppExtensionNames;
    *pppExtensionNames = (const char **)
        DK_ALLOCATE(pAllocator, ((*pExtensionCount) + 1) * sizeof(char *));
    if (ppBuffer != NULL)
        memcpy(*pppExtensionNames, ppBuffer,
               (*pExtensionCount) * sizeof(char *));

    (*pppExtensionNames)[*pExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    *pExtensionCount += 1;
}


static void
destroyDebugExtensionNames(const char **ppExtensionNames,
                           const DkAllocator *pAllocator)
{
    DK_FREE(pAllocator, ppExtensionNames);
}


static DkResult
checkValidationLayersSupport(const DkAllocator *pAllocator,
                             DkBool32 *pSupported)
{
    DkResult out;
    DkUint32 i;
    DkUint32 j;
    DkUint32 layerCount;
    VkLayerProperties *pLayers;

    DK_ASSERT(pAllocator != NULL);
    DK_ASSERT(pSupported != NULL);

    out = DK_SUCCESS;
    *pSupported = DK_FALSE;

    if (vkEnumerateInstanceLayerProperties((uint32_t *) &layerCount, NULL)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the number of layer properties "
                        "available\n");
        out = DK_ERROR;
        return out;
    }

    pLayers = (VkLayerProperties *)
        DK_ALLOCATE(pAllocator, layerCount * sizeof(VkLayerProperties));
    if (vkEnumerateInstanceLayerProperties((uint32_t *) &layerCount, pLayers)
        != VK_SUCCESS)
    {
        fprintf(stderr, "could not retrieve the layer properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    for (i = 0; i < validationLayerCount; ++i) {
        DkBool32 found;

        found = DK_FALSE;
        for (j = 0; j < layerCount; ++j) {
            if (strcmp(pLayers[j].layerName, ppValidationLayerNames[i]) == 0) {
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
createInstance(const char *pApplicationName,
               DkUint32 applicationMajorVersion,
               DkUint32 applicationMinorVersion,
               DkUint32 applicationPatchVersion,
               DkUint32 extensionCount,
               const char **ppExtensionNames,
               const VkAllocationCallbacks *pBackEndAllocator,
               const DkAllocator *pAllocator,
               VkInstance *pInstance)
{
#ifdef DK_ENABLE_VALIDATION_LAYERS
    DkBool32 validationLayersSupported;
    const char **ppExtensionNamesBuffer;
#endif
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo createInfo;

    DK_ASSERT(pApplicationName != NULL);

#ifdef DK_ENABLE_VALIDATION_LAYERS
    DK_ASSERT(pAllocator != NULL);
#else
    DK_UNUSED(pAllocator);
#endif

#ifdef DK_ENABLE_VALIDATION_LAYERS
    if (checkValidationLayersSupport(pAllocator, &validationLayersSupported)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    } else if (!validationLayersSupported) {
        fprintf(stderr, "one or more validation layers are not supported\n");
        return DK_ERROR;
    }

    ppExtensionNamesBuffer = ppExtensionNames;
    ppExtensionNames = (const char **)
        DK_ALLOCATE(pAllocator, (extensionCount + 1) * sizeof(char *));
    if (ppExtensionNamesBuffer != NULL)
        memcpy(ppExtensionNames, ppExtensionNamesBuffer,
               extensionCount * sizeof(char *));

    ppExtensionNames[extensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    extensionCount += 1;
#endif

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
#ifdef DK_ENABLE_VALIDATION_LAYERS
    createInfo.enabledLayerCount = validationLayerCount;
    createInfo.ppEnabledLayerNames = ppValidationLayerNames;
#else
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;
#endif
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;

    if (vkCreateInstance(&createInfo, pBackEndAllocator, pInstance)
        != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create the renderer instance\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


static void
destroyInstance(VkInstance instance,
                const VkAllocationCallbacks *pBackEndAllocator)
{
    vkDestroyInstance(instance, pBackEndAllocator);
}


DkResult
dkCreateRenderer(const DkRendererCreateInfo *pCreateInfo,
                 const DkAllocator *pAllocator,
                 DkRenderer **ppRenderer)
{
    DK_ASSERT(pCreateInfo != NULL);
    DK_ASSERT(ppRenderer != NULL);

    if (pAllocator == NULL)
        dkGetDefaultAllocator(&pAllocator);

    *ppRenderer = (DkRenderer *) DK_ALLOCATE(pAllocator, sizeof(DkRenderer));

    (*ppRenderer)->pAllocator = pAllocator;
    (*ppRenderer)->pBackEndAllocator = pCreateInfo->pBackEndAllocator;
    (*ppRenderer)->extensionCount = pCreateInfo->extensionCount;
    (*ppRenderer)->ppExtensionNames = pCreateInfo->ppExtensionNames;

#ifdef DK_ENABLE_VALIDATION_LAYERS
    createDebugExtensionNames((*ppRenderer)->pAllocator,
                              &(*ppRenderer)->extensionCount,
                              &(*ppRenderer)->ppExtensionNames);
#endif

    if (createInstance(pCreateInfo->pApplicationName,
                       pCreateInfo->applicationMajorVersion,
                       pCreateInfo->applicationMinorVersion,
                       pCreateInfo->applicationPatchVersion,
                       (*ppRenderer)->extensionCount,
                       (*ppRenderer)->ppExtensionNames,
                       (*ppRenderer)->pBackEndAllocator,
                       (*ppRenderer)->pAllocator,
                       &(*ppRenderer)->instance)
        != DK_SUCCESS)
    {
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


void
dkDestroyRenderer(DkRenderer *pRenderer)
{
    DK_ASSERT(pRenderer != NULL);

    destroyInstance(pRenderer->instance, pRenderer->pBackEndAllocator);

#ifdef DK_ENABLE_VALIDATION_LAYERS
    destroyDebugExtensionNames(pRenderer->ppExtensionNames,
                               pRenderer->pAllocator);
#endif

    DK_FREE(pRenderer->pAllocator, pRenderer);
}
