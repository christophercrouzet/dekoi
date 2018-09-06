#include "renderer.h"

#include "../common/private/allocator.h"
#include "../common/private/assert.h"
#include "../common/private/common.h"
#include "../common/private/logger.h"
#include "../common/allocator.h"
#include "../common/common.h"
#include "../common/logger.h"

#include <vulkan/vulkan.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef DK_RENDERER_DEBUG_REPORT
#define DKP_RENDERER_DEBUG_REPORT DKP_DEBUGGING
#elif DK_RENDERER_DEBUG_REPORT
#define DKP_RENDERER_DEBUG_REPORT 1
#else
#define DKP_RENDERER_DEBUG_REPORT 0
#endif /* DK_RENDERER_DEBUG_REPORT */

#ifndef DK_RENDERER_VALIDATION_LAYERS
#define DKP_RENDERER_VALIDATION_LAYERS DKP_DEBUGGING
#elif DK_RENDERER_VALIDATION_LAYERS
#define DKP_RENDERER_VALIDATION_LAYERS 1
#else
#define DKP_RENDERER_VALIDATION_LAYERS 0
#endif /* DK_RENDERER_VALIDATION_LAYERS */

#define DKP_CLAMP(x, low, high)                                                \
    (((x) > (high)) ? (high) : (x) < (low) ? (low) : (x))

enum DkpPresentSupport {
    DKP_PRESENT_SUPPORT_DISABLED = 0,
    DKP_PRESENT_SUPPORT_ENABLED = 1
};

enum DkpOldSwapChainPreservation {
    DKP_OLD_SWAP_CHAIN_PRESERVATION_DISABLED = 0,
    DKP_OLD_SWAP_CHAIN_PRESERVATION_ENABLED = 1
};

enum DkpQueueType {
    DKP_QUEUE_TYPE_GRAPHICS = 0,
    DKP_QUEUE_TYPE_COMPUTE = 1,
    DKP_QUEUE_TYPE_TRANSFER = 2,
    DKP_QUEUE_TYPE_PRESENT = 3,
    DKP_QUEUE_TYPE_ENUM_LAST = DKP_QUEUE_TYPE_PRESENT,
    DKP_QUEUE_TYPE_ENUM_COUNT = DKP_QUEUE_TYPE_ENUM_LAST + 1
};

enum DkpSemaphoreId {
    DKP_SEMAPHORE_ID_IMAGE_ACQUIRED = 0,
    DKP_SEMAPHORE_ID_PRESENT_COMPLETED = 1,
    DKP_SEMAPHORE_ID_ENUM_LAST = DKP_SEMAPHORE_ID_PRESENT_COMPLETED,
    DKP_SEMAPHORE_ID_ENUM_COUNT = DKP_SEMAPHORE_ID_ENUM_LAST + 1
};

enum DkpConstant {
    DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED = DKP_QUEUE_TYPE_ENUM_COUNT
};

struct DkpBackEndAllocationCallbacksData {
    const struct DkLoggingCallbacks *pLogger;
    const struct DkAllocationCallbacks *pAllocator;
};

struct DkpDebugReportCallbackData {
    const struct DkLoggingCallbacks *pLogger;
};

struct DkpQueues {
    VkQueue graphicsHandle;
    VkQueue computeHandle;
    VkQueue transferHandle;
    VkQueue presentHandle;
};

struct DkpDevice {
    uint32_t queueFamilyIndices[DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED];
    uint32_t filteredQueueFamilyCount;
    uint32_t filteredQueueFamilyIndices[DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED];
    VkPhysicalDevice physicalHandle;
    VkDevice logicalHandle;
};

struct DkpSwapChainProperties {
    uint32_t minImageCount;
    VkExtent2D imageExtent;
    VkImageUsageFlags imageUsage;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
};

struct DkpShader {
    VkShaderModule moduleHandle;
    VkShaderStageFlagBits stage;
    const char *pEntryPointName;
};

struct DkpBuffer {
    VkBuffer handle;
    VkDeviceMemory memoryHandle;
    VkDeviceSize offset;
};

struct DkpSwapChain {
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR format;
    VkExtent2D imageExtent;
    uint32_t imageCount;
    VkImage *pImageHandles;
    VkImageView *pImageViewHandles;
};

struct DkpCommandPools {
    VkCommandPool handles[DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED];
    VkCommandPool handleMap[DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED];
};

struct DkRenderer {
    const struct DkLoggingCallbacks *pLogger;
    const struct DkAllocationCallbacks *pAllocator;
    struct DkpBackEndAllocationCallbacksData backEndAllocatorData;
    VkAllocationCallbacks backEndAllocator;
    VkClearValue clearColor;
    uint32_t vertexBindingDescriptionCount;
    VkVertexInputBindingDescription *pVertexBindingDescriptions;
    uint32_t vertexAttributeDescriptionCount;
    VkVertexInputAttributeDescription *pVertexAttributeDescriptions;
    VkInstance instanceHandle;
    VkExtent2D surfaceExtent;
    VkSurfaceKHR surfaceHandle;
#if DKP_RENDERER_DEBUG_REPORT
    struct DkpDebugReportCallbackData debugReportCallbackData;
    VkDebugReportCallbackEXT debugReportCallbackHandle;
#endif /* DKP_RENDERER_DEBUG_REPORT */
    struct DkpDevice device;
    struct DkpQueues queues;
    VkSemaphore *pSemaphoreHandles;
    uint32_t shaderCount;
    struct DkpShader *pShaders;
    uint32_t vertexBufferCount;
    struct DkpBuffer *pVertexBuffers;
    struct DkpSwapChain swapChain;
    VkRenderPass renderPassHandle;
    VkPipelineLayout pipelineLayoutHandle;
    VkPipeline graphicsPipelineHandle;
    VkFramebuffer *pFramebufferHandles;
    struct DkpCommandPools commandPools;
    VkCommandBuffer *pGraphicsCommandBufferHandles;
    uint32_t vertexCount;
    uint32_t instanceCount;
};

static void
dkpGetSemaphoreIdDescription(const char **ppDescription,
                             enum DkpSemaphoreId semaphoreId)
{
    switch (semaphoreId) {
        case DKP_SEMAPHORE_ID_IMAGE_ACQUIRED:
            *ppDescription = "image acquired";
            return;
        case DKP_SEMAPHORE_ID_PRESENT_COMPLETED:
            *ppDescription = "present completed";
            return;
        default:
            DKP_ASSERT(0);
            *ppDescription = "invalid";
    }
}

static void
dkpValidateShaderStage(int *pValid, enum DkShaderStage shaderStage)
{
    switch (shaderStage) {
        case DK_SHADER_STAGE_VERTEX:
        case DK_SHADER_STAGE_TESSELLATION_CONTROL:
        case DK_SHADER_STAGE_TESSELLATION_EVALUATION:
        case DK_SHADER_STAGE_GEOMETRY:
        case DK_SHADER_STAGE_FRAGMENT:
        case DK_SHADER_STAGE_COMPUTE:
            *pValid = DKP_TRUE;
            return;
        default:
            *pValid = DKP_FALSE;
    }
}

static void
dkpTranslateShaderStageToBackEnd(VkShaderStageFlagBits *pBackEndShaderStage,
                                 enum DkShaderStage shaderStage)
{
    switch (shaderStage) {
        case DK_SHADER_STAGE_VERTEX:
            *pBackEndShaderStage = VK_SHADER_STAGE_VERTEX_BIT;
            return;
        case DK_SHADER_STAGE_TESSELLATION_CONTROL:
            *pBackEndShaderStage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            return;
        case DK_SHADER_STAGE_TESSELLATION_EVALUATION:
            *pBackEndShaderStage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            return;
        case DK_SHADER_STAGE_GEOMETRY:
            *pBackEndShaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
            return;
        case DK_SHADER_STAGE_FRAGMENT:
            *pBackEndShaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
            return;
        case DK_SHADER_STAGE_COMPUTE:
            *pBackEndShaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
            return;
        default:
            DKP_ASSERT(0);
            *pBackEndShaderStage = (VkShaderStageFlagBits)0;
    }
}

static void
dkpTranslateVertexInputRateToBackEnd(VkVertexInputRate *pBackEndInputRate,
                                     enum DkVertexInputRate inputRate)
{
    switch (inputRate) {
        case DK_VERTEX_INPUT_RATE_VERTEX:
            *pBackEndInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return;
        case DK_VERTEX_INPUT_RATE_INSTANCE:
            *pBackEndInputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
            return;
        default:
            DKP_ASSERT(0);
            *pBackEndInputRate = (VkVertexInputRate)0;
    }
}

static void
dkpTranslateFormatToBackEnd(VkFormat *pBackEndFormat, enum DkFormat format)
{
    switch (format) {
        case DK_FORMAT_R32G32_SFLOAT:
            *pBackEndFormat = VK_FORMAT_R32G32_SFLOAT;
            return;
        case DK_FORMAT_R32G32B32_SFLOAT:
            *pBackEndFormat = VK_FORMAT_R32G32B32_SFLOAT;
            return;
        default:
            DKP_ASSERT(0);
            *pBackEndFormat = (VkFormat)0;
    }
}

static void
dkpFilterQueueFamilyIndices(uint32_t *pFilteredQueueFamilyCount,
                            uint32_t *pFilteredQueueFamilyIndices,
                            const uint32_t *pQueueFamilyIndices)
{
    uint32_t i;
    uint32_t j;

    DKP_ASSERT(pFilteredQueueFamilyCount != NULL);
    DKP_ASSERT(pFilteredQueueFamilyIndices != NULL);
    DKP_ASSERT(pQueueFamilyIndices != NULL);

    *pFilteredQueueFamilyCount = 0;
    for (i = 0; i < DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED; ++i) {
        int found;

        if (pQueueFamilyIndices[i] == (uint32_t)-1) {
            continue;
        }

        found = DKP_FALSE;
        for (j = 0; j < *pFilteredQueueFamilyCount; ++j) {
            if (pFilteredQueueFamilyIndices[j] == pQueueFamilyIndices[i]) {
                found = DKP_TRUE;
                break;
            }
        }

        if (!found) {
            pFilteredQueueFamilyIndices[*pFilteredQueueFamilyCount]
                = pQueueFamilyIndices[i];
            ++*pFilteredQueueFamilyCount;
        }
    }
}

static enum DkStatus
dkpPickMemoryTypeIndex(uint32_t *pMemoryTypeIndex,
                       const struct DkpDevice *pDevice,
                       uint32_t typeFilter,
                       VkMemoryPropertyFlags properties,
                       const struct DkLoggingCallbacks *pLogger)
{
    uint32_t i;
    VkPhysicalDeviceMemoryProperties memoryProperties;

    DKP_ASSERT(pMemoryTypeIndex != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->physicalHandle != NULL);
    DKP_ASSERT(pLogger != NULL);

    vkGetPhysicalDeviceMemoryProperties(pDevice->physicalHandle,
                                        &memoryProperties);
    for (i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if (typeFilter & ((uint32_t)1 << i)
            && (memoryProperties.memoryTypes[i].propertyFlags & properties)
                   == properties) {
            *pMemoryTypeIndex = i;
            return DK_SUCCESS;
        }
    }

    DKP_LOG_TRACE(pLogger, "could not find a suitable memory type\n");
    return DK_ERROR;
}

static enum DkStatus
dkpCopyBuffer(const struct DkpDevice *pDevice,
              const struct DkpBuffer *pDestination,
              const struct DkpBuffer *pSource,
              VkDeviceSize size,
              VkCommandPool commandPoolHandle,
              const struct DkpQueues *pQueues,
              const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    VkCommandBufferAllocateInfo allocateInfo;
    VkCommandBuffer commandBuffer;
    VkCommandBufferBeginInfo beginInfo;
    VkBufferCopy copyRegion;
    VkSubmitInfo submitInfo;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pDestination != NULL);
    DKP_ASSERT(pSource != NULL);
    DKP_ASSERT(commandPoolHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pQueues != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = NULL;
    allocateInfo.commandPool = commandPoolHandle;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(
            pDevice->logicalHandle, &allocateInfo, &commandBuffer)
        != VK_SUCCESS) {
        out = DK_ERROR;
        DKP_LOG_TRACE(pLogger, "failed to allocate copy command buffers\n");
        goto exit;
    }

    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = NULL;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = NULL;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        out = DK_ERROR;
        DKP_LOG_TRACE(pLogger,
                      "could not begin the copy command buffer recording\n");
        goto allocate_command_buffers_cleanup;
    }

    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(
        commandBuffer, pSource->handle, pDestination->handle, 1, &copyRegion);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        out = DK_ERROR;
        DKP_LOG_TRACE(pLogger,
                      "could not end the copy command buffer recording\n");
        goto allocate_command_buffers_cleanup;
    }

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;

    if (vkQueueSubmit(pQueues->transferHandle, 1, &submitInfo, VK_NULL_HANDLE)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "could not submit the copy command buffer\n");
        out = DK_ERROR;
        goto allocate_command_buffers_cleanup;
    }

    vkQueueWaitIdle(pQueues->transferHandle);

allocate_command_buffers_cleanup:
    vkFreeCommandBuffers(
        pDevice->logicalHandle, commandPoolHandle, 1, &commandBuffer);

exit:
    return out;
}

static enum DkStatus
dkpInitializeBuffer(struct DkpBuffer *pBuffer,
                    const struct DkpDevice *pDevice,
                    VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags memoryProperties,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    VkBufferCreateInfo bufferInfo;
    VkMemoryRequirements memoryRequirements;
    VkMemoryAllocateInfo allocateInfo;

    DKP_ASSERT(pBuffer != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = NULL;
    bufferInfo.flags = 0;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = NULL;

    if (vkCreateBuffer(pDevice->logicalHandle,
                       &bufferInfo,
                       pBackEndAllocator,
                       &pBuffer->handle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to create the buffer\n");
        out = DK_ERROR;
        goto exit;
    }

    vkGetBufferMemoryRequirements(
        pDevice->logicalHandle, pBuffer->handle, &memoryRequirements);

    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = NULL;
    allocateInfo.allocationSize = memoryRequirements.size;

    out = dkpPickMemoryTypeIndex(&allocateInfo.memoryTypeIndex,
                                 pDevice,
                                 memoryRequirements.memoryTypeBits,
                                 memoryProperties,
                                 pLogger);
    if (out != DK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to pick a memory type index\n");
        goto buffer_undo;
    }

    if (vkAllocateMemory(pDevice->logicalHandle,
                         &allocateInfo,
                         pBackEndAllocator,
                         &pBuffer->memoryHandle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the buffer memory\n");
        out = DK_ERROR;
        goto buffer_undo;
    }

    if (vkBindBufferMemory(
            pDevice->logicalHandle, pBuffer->handle, pBuffer->memoryHandle, 0)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to bind the buffer memory\n");
        out = DK_ERROR;
        goto allocate_memory_undo;
    }

    goto exit;

allocate_memory_undo:
    vkFreeMemory(
        pDevice->logicalHandle, pBuffer->memoryHandle, pBackEndAllocator);

buffer_undo:
    vkDestroyBuffer(pDevice->logicalHandle, pBuffer->handle, pBackEndAllocator);

exit:
    return out;
}

static void
dkpTerminateBuffer(const struct DkpDevice *pDevice,
                   struct DkpBuffer *pBuffer,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBuffer != NULL);
    DKP_ASSERT(pBuffer->handle != VK_NULL_HANDLE);
    DKP_ASSERT(pBuffer->memoryHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkFreeMemory(
        pDevice->logicalHandle, pBuffer->memoryHandle, pBackEndAllocator);
    vkDestroyBuffer(pDevice->logicalHandle, pBuffer->handle, pBackEndAllocator);
}

static enum DkStatus
dkpCreateInstanceLayerNames(uint32_t *pLayerCount,
                            const char ***pppLayerNames,
                            const struct DkAllocationCallbacks *pAllocator,
                            const struct DkLoggingCallbacks *pLogger)
{
    DKP_ASSERT(pLayerCount != NULL);
    DKP_ASSERT(pppLayerNames != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (DKP_RENDERER_VALIDATION_LAYERS) {
        *pLayerCount = 1;
        *pppLayerNames = (const char **)DKP_ALLOCATE(
            pAllocator, sizeof **pppLayerNames * *pLayerCount);
        if (*pppLayerNames == NULL) {
            DKP_LOG_TRACE(pLogger,
                          "failed to allocate the instance layer names\n");
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
                             const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pAllocator != NULL);

    if (DKP_RENDERER_VALIDATION_LAYERS) {
        DKP_ASSERT(ppLayerNames != NULL);
        DKP_FREE(pAllocator, ppLayerNames);
    }
}

static enum DkStatus
dkpCheckInstanceLayersSupport(int *pSupported,
                              uint32_t requiredLayerCount,
                              const char *const *ppRequiredLayerNames,
                              const struct DkAllocationCallbacks *pAllocator,
                              const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t j;
    uint32_t layerCount;
    VkLayerProperties *pLayers;

    DKP_ASSERT(pSupported != NULL);
    DKP_ASSERT(ppRequiredLayerNames != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateInstanceLayerProperties(&layerCount, NULL) != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not retrieve the number of instance layer "
                      "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    if (layerCount == 0) {
        *pSupported = requiredLayerCount == 0 ? DKP_TRUE : DKP_FALSE;
        goto exit;
    }

    pLayers = (VkLayerProperties *)DKP_ALLOCATE(pAllocator,
                                                sizeof *pLayers * layerCount);
    if (pLayers == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the instance layer properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateInstanceLayerProperties(&layerCount, pLayers)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(
            pLogger,
            "could not enumerate the instance layer properties available\n");
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

        if (!found) {
            goto layers_cleanup;
        }
    }

    *pSupported = DKP_TRUE;

layers_cleanup:
    DKP_FREE(pAllocator, pLayers);

exit:
    return out;
}

static enum DkStatus
dkpCreateInstanceExtensionNames(
    uint32_t *pExtensionCount,
    const char ***pppExtensionNames,
    const struct DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    const char **ppBuffer;

    DKP_ASSERT(pExtensionCount != NULL);
    DKP_ASSERT(pppExtensionNames != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (pWindowSystemIntegrator == NULL) {
        *pExtensionCount = 0;
        *pppExtensionNames = NULL;
    } else if (pWindowSystemIntegrator->pfnCreateInstanceExtensionNames(
                   (DkUint32 *)pExtensionCount,
                   pppExtensionNames,
                   pWindowSystemIntegrator->pData,
                   pLogger)
               != DK_SUCCESS) {
        return DK_ERROR;
    }

    if (DKP_RENDERER_DEBUG_REPORT) {
        ppBuffer = (const char **)DKP_ALLOCATE(
            pAllocator, sizeof *ppBuffer * (*pExtensionCount + 1));
        if (ppBuffer == NULL) {
            DKP_LOG_TRACE(pLogger,
                          "failed to allocate the instance extension names\n");
            return DK_ERROR_ALLOCATION;
        }

        if (*pppExtensionNames != NULL) {
            memcpy(ppBuffer,
                   *pppExtensionNames,
                   sizeof *ppBuffer * *pExtensionCount);
        }

        ppBuffer[*pExtensionCount] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

        if (pWindowSystemIntegrator != NULL) {
            pWindowSystemIntegrator->pfnDestroyInstanceExtensionNames(
                pWindowSystemIntegrator->pData, pLogger, *pppExtensionNames);
        }

        *pExtensionCount += 1;
        *pppExtensionNames = ppBuffer;
    }

    return DK_SUCCESS;
}

static void
dkpDestroyInstanceExtensionNames(
    const char **ppExtensionNames,
    const struct DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (DKP_RENDERER_DEBUG_REPORT) {
        DKP_ASSERT(ppExtensionNames != NULL);
        DKP_FREE(pAllocator, ppExtensionNames);
    } else if (pWindowSystemIntegrator != NULL) {
        pWindowSystemIntegrator->pfnDestroyInstanceExtensionNames(
            pWindowSystemIntegrator->pData, pLogger, ppExtensionNames);
    }
}

static enum DkStatus
dkpCheckInstanceExtensionsSupport(
    int *pSupported,
    uint32_t requiredExtensionCount,
    const char *const *ppRequiredExtensionNames,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t j;
    uint32_t extensionCount;
    VkExtensionProperties *pExtensions;

    DKP_ASSERT(pSupported != NULL);
    DKP_ASSERT(ppRequiredExtensionNames != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not retrieve the number of instance extension "
                      "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    if (extensionCount == 0) {
        *pSupported = requiredExtensionCount == 0 ? DKP_TRUE : DKP_FALSE;
        goto exit;
    }

    pExtensions = (VkExtensionProperties *)DKP_ALLOCATE(
        pAllocator, sizeof *pExtensions * extensionCount);
    if (pExtensions == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the instance extension properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateInstanceExtensionProperties(
            NULL, &extensionCount, pExtensions)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not enumerate the instance extension properties "
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
                == 0) {
                found = DKP_TRUE;
                break;
            }
        }

        if (!found) {
            goto extensions_cleanup;
        }
    }

    *pSupported = DKP_TRUE;

extensions_cleanup:
    DKP_FREE(pAllocator, pExtensions);

exit:
    return out;
}

static enum DkStatus
dkpCreateInstance(
    VkInstance *pInstanceHandle,
    const char *pApplicationName,
    unsigned int applicationMajorVersion,
    unsigned int applicationMinorVersion,
    unsigned int applicationPatchVersion,
    const struct DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator,
    const VkAllocationCallbacks *pBackEndAllocator,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t layerCount;
    const char **ppLayerNames;
    int layersSupported;
    uint32_t extensionCount;
    const char **ppExtensionNames;
    int extensionsSupported;
    VkApplicationInfo applicationInfo;
    VkInstanceCreateInfo createInfo;

    DKP_ASSERT(pInstanceHandle != NULL);
    DKP_ASSERT(pApplicationName != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    out = dkpCreateInstanceLayerNames(
        &layerCount, &ppLayerNames, pAllocator, pLogger);
    if (out != DK_SUCCESS) {
        goto exit;
    }

    out = dkpCheckInstanceLayersSupport(
        &layersSupported, layerCount, ppLayerNames, pAllocator, pLogger);
    if (out != DK_SUCCESS) {
        goto layer_names_cleanup;
    }

    if (!layersSupported) {
        DKP_LOG_TRACE(pLogger,
                      "one or more instance layers are not supported\n");
        out = DK_ERROR;
        goto layer_names_cleanup;
    }

    out = dkpCreateInstanceExtensionNames(&extensionCount,
                                          &ppExtensionNames,
                                          pWindowSystemIntegrator,
                                          pAllocator,
                                          pLogger);
    if (out != DK_SUCCESS) {
        goto layer_names_cleanup;
    }

    out = dkpCheckInstanceExtensionsSupport(&extensionsSupported,
                                            extensionCount,
                                            ppExtensionNames,
                                            pAllocator,
                                            pLogger);
    if (out != DK_SUCCESS) {
        goto extension_names_cleanup;
    }

    if (!extensionsSupported) {
        DKP_LOG_TRACE(pLogger,
                      "one or more instance extensions are not supported\n");
        out = DK_ERROR;
        goto extension_names_cleanup;
    }

    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = NULL;
    applicationInfo.pApplicationName = pApplicationName;
    applicationInfo.applicationVersion
        = VK_MAKE_VERSION(applicationMajorVersion,
                          applicationMinorVersion,
                          applicationPatchVersion);
    applicationInfo.pEngineName = DK_NAME;
    applicationInfo.engineVersion
        = VK_MAKE_VERSION(DK_MAJOR_VERSION, DK_MINOR_VERSION, DK_PATCH_VERSION);
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
            DKP_LOG_TRACE(pLogger,
                          "some requested instance layers are not supported\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            DKP_LOG_TRACE(
                pLogger,
                "some requested instance extensions are not supported\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            DKP_LOG_TRACE(pLogger, "the driver is incompatible\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        case VK_ERROR_INITIALIZATION_FAILED:
            DKP_LOG_TRACE(pLogger, "failed to initialize the instance\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
        default:
            DKP_LOG_TRACE(pLogger, "failed to create the instance\n");
            out = DK_ERROR;
            goto extension_names_cleanup;
    }

extension_names_cleanup:
    dkpDestroyInstanceExtensionNames(
        ppExtensionNames, pWindowSystemIntegrator, pAllocator, pLogger);

layer_names_cleanup:
    dkpDestroyInstanceLayerNames(ppLayerNames, pAllocator);

exit:
    return out;
}

static void
dkpDestroyInstance(VkInstance instanceHandle,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroyInstance(instanceHandle, pBackEndAllocator);
}

#if DKP_RENDERER_DEBUG_REPORT
static VkBool32
dkpHandleDebugReport(VkDebugReportFlagsEXT flags,
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

    DKP_LOG_TRACE(((struct DkpDebugReportCallbackData *)pUserData)->pLogger,
                  "validation layer: %s\n",
                  pMessage);
    return VK_FALSE;
}

static enum DkStatus
dkpCreateDebugReportCallback(VkDebugReportCallbackEXT *pCallbackHandle,
                             VkInstance instanceHandle,
                             struct DkpDebugReportCallbackData *pData,
                             const VkAllocationCallbacks *pBackEndAllocator,
                             const struct DkLoggingCallbacks *pLogger)
{
    VkDebugReportCallbackCreateInfoEXT createInfo;
    PFN_vkCreateDebugReportCallbackEXT function;

    DKP_ASSERT(pCallbackHandle != NULL);
    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.pNext = NULL;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
                       | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = dkpHandleDebugReport;
    createInfo.pUserData = pData;

    function = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
        instanceHandle, "vkCreateDebugReportCallbackEXT");
    if (function == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "could not retrieve the 'vkCreateDebugReportCallbackEXT' "
                      "function\n");
        return DK_ERROR;
    }

    if (function(
            instanceHandle, &createInfo, pBackEndAllocator, pCallbackHandle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to create the debug report callback\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
dkpDestroyDebugReportCallback(VkInstance instanceHandle,
                              VkDebugReportCallbackEXT callbackHandle,
                              const VkAllocationCallbacks *pBackEndAllocator,
                              const struct DkLoggingCallbacks *pLogger)
{
    PFN_vkDestroyDebugReportCallbackEXT function;

    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(callbackHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    function = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
        instanceHandle, "vkDestroyDebugReportCallbackEXT");
    if (function == NULL) {
        DKP_LOG_TRACE(
            pLogger,
            "could not retrieve the 'vkDestroyDebugReportCallbackEXT' "
            "function\n");
        return;
    }

    function(instanceHandle, callbackHandle, pBackEndAllocator);
}
#endif /* DKP_RENDERER_DEBUG_REPORT */

static enum DkStatus
dkpCreateSurface(
    VkSurfaceKHR *pSurfaceHandle,
    VkInstance instanceHandle,
    const struct DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator,
    const VkAllocationCallbacks *pBackEndAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    DKP_ASSERT(pSurfaceHandle != NULL);
    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(pWindowSystemIntegrator != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (pWindowSystemIntegrator->pfnCreateSurface(
            pSurfaceHandle,
            pWindowSystemIntegrator->pData,
            instanceHandle,
            pBackEndAllocator,
            pLogger)
        != DK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "the window system integrator's 'createSurface' callback "
                      "returned an error\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
dkpDestroySurface(VkInstance instanceHandle,
                  VkSurfaceKHR surfaceHandle,
                  const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(surfaceHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroySurfaceKHR(instanceHandle, surfaceHandle, pBackEndAllocator);
}

static enum DkStatus
dkpCreateDeviceExtensionNames(uint32_t *pExtensionCount,
                              const char ***pppExtensionNames,
                              enum DkpPresentSupport presentSupport,
                              const struct DkAllocationCallbacks *pAllocator,
                              const struct DkLoggingCallbacks *pLogger)
{
    DKP_ASSERT(pExtensionCount != NULL);
    DKP_ASSERT(pppExtensionNames != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (presentSupport == DKP_PRESENT_SUPPORT_DISABLED) {
        *pExtensionCount = 0;
        *pppExtensionNames = NULL;
        return DK_SUCCESS;
    }

    *pExtensionCount = 1;
    *pppExtensionNames = (const char **)DKP_ALLOCATE(
        pAllocator, sizeof **pppExtensionNames * *pExtensionCount);
    if (*pppExtensionNames == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the device extension names\n");
        return DK_ERROR_ALLOCATION;
    }

    (*pppExtensionNames)[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    return DK_SUCCESS;
}

static void
dkpDestroyDeviceExtensionNames(const char **ppExtensionNames,
                               const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pAllocator != NULL);

    if (ppExtensionNames != NULL) {
        DKP_FREE(pAllocator, ppExtensionNames);
    }
}

static enum DkStatus
dkpCheckDeviceExtensionsSupport(int *pSupported,
                                VkPhysicalDevice physicalDeviceHandle,
                                uint32_t requiredExtensionCount,
                                const char *const *ppRequiredExtensionNames,
                                const struct DkAllocationCallbacks *pAllocator,
                                const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t j;
    uint32_t extensionCount;
    VkExtensionProperties *pExtensions;

    DKP_ASSERT(pSupported != NULL);
    DKP_ASSERT(physicalDeviceHandle != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vkEnumerateDeviceExtensionProperties(
            physicalDeviceHandle, NULL, &extensionCount, NULL)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not retrieve the number of device extension "
                      "properties available\n");
        out = DK_ERROR;
        goto exit;
    }

    if (extensionCount == 0) {
        *pSupported = requiredExtensionCount == 0 ? DKP_TRUE : DKP_FALSE;
        goto exit;
    }

    pExtensions = (VkExtensionProperties *)DKP_ALLOCATE(
        pAllocator, sizeof *pExtensions * extensionCount);
    if (pExtensions == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the device extension properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumerateDeviceExtensionProperties(
            physicalDeviceHandle, NULL, &extensionCount, pExtensions)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(
            pLogger,
            "could not enumerate the device extension properties available\n");
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
                == 0) {
                found = DKP_TRUE;
                break;
            }
        }

        if (!found) {
            goto extensions_cleanup;
        }
    }

    *pSupported = DKP_TRUE;

extensions_cleanup:
    DKP_FREE(pAllocator, pExtensions);

exit:
    return out;
}

static enum DkStatus
dkpPickDeviceQueueFamilies(uint32_t *pQueueFamilyIndices,
                           VkPhysicalDevice physicalDeviceHandle,
                           VkSurfaceKHR surfaceHandle,
                           const struct DkAllocationCallbacks *pAllocator,
                           const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t propertyCount;
    VkQueueFamilyProperties *pProperties;

    DKP_ASSERT(pQueueFamilyIndices != NULL);
    DKP_ASSERT(physicalDeviceHandle != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    for (i = 0; i < DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED; ++i) {
        pQueueFamilyIndices[i] = (uint32_t)-1;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDeviceHandle, &propertyCount, NULL);
    if (propertyCount == 0) {
        goto exit;
    }

    pProperties = (VkQueueFamilyProperties *)DKP_ALLOCATE(
        pAllocator, sizeof *pProperties * propertyCount);
    if (pProperties == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the queue family properties\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDeviceHandle, &propertyCount, pProperties);

    /*
       Pick a graphics and a present queue, preferably sharing the same index.
    */
    for (i = 0; i < propertyCount; ++i) {
        int graphicsSupported;

        if (pProperties[i].queueCount == 0) {
            continue;
        }

        graphicsSupported = pProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

        if (pQueueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS] == (uint32_t)-1
            && graphicsSupported) {
            pQueueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS] = i;
        }

        if (surfaceHandle != VK_NULL_HANDLE) {
            VkBool32 presentSupported;

            if (vkGetPhysicalDeviceSurfaceSupportKHR(
                    physicalDeviceHandle, i, surfaceHandle, &presentSupported)
                != VK_SUCCESS) {
                DKP_LOG_TRACE(pLogger,
                              "could not determine support for presentation\n");
                out = DK_ERROR;
                goto properties_cleanup;
            }

            if (pQueueFamilyIndices[DKP_QUEUE_TYPE_PRESENT] == (uint32_t)-1
                && presentSupported) {
                pQueueFamilyIndices[DKP_QUEUE_TYPE_PRESENT] = i;
            }

            if (graphicsSupported && presentSupported
                && pQueueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS]
                       != pQueueFamilyIndices[DKP_QUEUE_TYPE_PRESENT]) {
                pQueueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS] = i;
                pQueueFamilyIndices[DKP_QUEUE_TYPE_PRESENT] = i;
                break;
            }
        }
    }

    /*
       Pick a compute and a transfer queue that have not been picked already.
    */
    for (i = 0; i < propertyCount; ++i) {
        int computeSupported;
        int transferSupported;

        if (pProperties[i].queueCount == 0) {
            continue;
        }

        computeSupported = pProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        transferSupported = pProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;

        if (pQueueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS] == i
            || pQueueFamilyIndices[DKP_QUEUE_TYPE_PRESENT] == i) {
            continue;
        }

        if (pQueueFamilyIndices[DKP_QUEUE_TYPE_COMPUTE] == (uint32_t)-1
            && computeSupported) {
            pQueueFamilyIndices[DKP_QUEUE_TYPE_COMPUTE] = i;
        } else if (pQueueFamilyIndices[DKP_QUEUE_TYPE_TRANSFER] == (uint32_t)-1
                   && transferSupported) {
            pQueueFamilyIndices[DKP_QUEUE_TYPE_TRANSFER] = i;
        }
    }

    if (pQueueFamilyIndices[DKP_QUEUE_TYPE_COMPUTE] == (uint32_t)-1) {
        /* Fallback to picking the first compute queue available. */
        for (i = 0; i < propertyCount; ++i) {
            int computeSupported;

            if (pProperties[i].queueCount == 0) {
                continue;
            }

            computeSupported = pProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;

            if (computeSupported) {
                pQueueFamilyIndices[DKP_QUEUE_TYPE_COMPUTE] = i;
                break;
            }
        }
    }

    if (pQueueFamilyIndices[DKP_QUEUE_TYPE_TRANSFER] == (uint32_t)-1) {
        /* Fallback to picking the first transfer queue available. */
        for (i = 0; i < propertyCount; ++i) {
            int graphicsSupported;
            int computeSupported;
            int transferSupported;

            if (pProperties[i].queueCount == 0) {
                continue;
            }

            graphicsSupported = pProperties[i].queueFlags
                                & VK_QUEUE_GRAPHICS_BIT;
            computeSupported = pProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
            transferSupported = pProperties[i].queueFlags
                                & VK_QUEUE_TRANSFER_BIT;

            /* Graphics and compute queues also support transfer operations. */
            if (graphicsSupported || computeSupported || transferSupported) {
                pQueueFamilyIndices[DKP_QUEUE_TYPE_TRANSFER] = i;
                break;
            }
        }
    }

properties_cleanup:
    DKP_FREE(pAllocator, pProperties);

exit:
    return out;
}

static enum DkStatus
dkpPickSwapChainPresentMode(VkPresentModeKHR *pPresentMode,
                            uint32_t presentModeCount,
                            const VkPresentModeKHR *pPresentModes)
{
    uint32_t i;

    DKP_ASSERT(pPresentMode != NULL);
    DKP_ASSERT(pPresentModes != NULL);

    for (i = 0; i < presentModeCount; ++i) {
        if (pPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            *pPresentMode = pPresentModes[i];
            return DK_SUCCESS;
        }
    }

    for (i = 0; i < presentModeCount; ++i) {
        if (pPresentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            *pPresentMode = pPresentModes[i];
            return DK_SUCCESS;
        }
    }

    for (i = 0; i < presentModeCount; ++i) {
        if (pPresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            *pPresentMode = pPresentModes[i];
            return DK_SUCCESS;
        }
    }

    return DK_ERROR_NOT_AVAILABLE;
}

static enum DkStatus
dkpPickSwapChainImageUsage(VkImageUsageFlags *pImageUsage,
                           VkSurfaceCapabilitiesKHR capabilities)
{
    DKP_ASSERT(pImageUsage != NULL);

    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        return DK_ERROR_NOT_AVAILABLE;
    }

    *pImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                   | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    return DK_SUCCESS;
}

static void
dkpPickSwapChainFormat(VkSurfaceFormatKHR *pFormat,
                       uint32_t formatCount,
                       const VkSurfaceFormatKHR *pFormats)
{
    uint32_t i;

    DKP_ASSERT(pFormat != NULL);
    DKP_ASSERT(pFormats != NULL);

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
dkpPickSwapChainMinImageCount(uint32_t *pMinImageCount,
                              VkSurfaceCapabilitiesKHR capabilities,
                              VkPresentModeKHR presentMode)
{
    DKP_ASSERT(pMinImageCount != NULL);

    *pMinImageCount = capabilities.minImageCount;

    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        ++(*pMinImageCount);
    }

    if (capabilities.maxImageCount > 0
        && *pMinImageCount > capabilities.maxImageCount) {
        *pMinImageCount = capabilities.maxImageCount;
    }
}

static void
dkpPickSwapChainImageExtent(VkExtent2D *pImageExtent,
                            VkSurfaceCapabilitiesKHR capabilities,
                            const VkExtent2D *pDesiredImageExtent)
{
    DKP_ASSERT(pImageExtent != NULL);
    DKP_ASSERT(pDesiredImageExtent != NULL);

    if (capabilities.currentExtent.width == (uint32_t)-1
        || capabilities.currentExtent.height == (uint32_t)-1) {
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
dkpPickSwapChainPreTransform(VkSurfaceTransformFlagBitsKHR *pPreTransform,
                             VkSurfaceCapabilitiesKHR capabilities)
{
    DKP_ASSERT(pPreTransform != NULL);

    *pPreTransform = capabilities.currentTransform;
}

static enum DkStatus
dkpPickSwapChainProperties(struct DkpSwapChainProperties *pSwapChainProperties,
                           VkPhysicalDevice physicalDeviceHandle,
                           VkSurfaceKHR surfaceHandle,
                           const VkExtent2D *pDesiredImageExtent,
                           const struct DkAllocationCallbacks *pAllocator,
                           const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount;
    VkSurfaceFormatKHR *pFormats;
    uint32_t presentModeCount;
    VkPresentModeKHR *pPresentModes;

    DKP_ASSERT(pSwapChainProperties != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDeviceHandle, surfaceHandle, &capabilities)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "could not retrieve the surface capabilities\n");
        out = DK_ERROR;
        goto exit;
    }

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDeviceHandle, surfaceHandle, &formatCount, NULL)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not count the number of the surface formats\n");
        out = DK_ERROR;
        goto exit;
    }

    if (formatCount == 0) {
        DKP_LOG_TRACE(pLogger, "could not find a suitable surface format\n");
        out = DK_ERROR_NOT_AVAILABLE;
        goto exit;
    }

    pFormats = (VkSurfaceFormatKHR *)DKP_ALLOCATE(
        pAllocator, sizeof *pFormats * formatCount);
    if (pFormats == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the surface formats\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDeviceHandle, surfaceHandle, &formatCount, pFormats)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "could not enumerate the surface formats\n");
        out = DK_ERROR;
        goto formats_cleanup;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDeviceHandle, surfaceHandle, &presentModeCount, NULL)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(
            pLogger,
            "could not count the number of the surface present modes\n");
        out = DK_ERROR;
        goto formats_cleanup;
    }

    if (presentModeCount == 0) {
        DKP_LOG_TRACE(pLogger,
                      "could not find a suitable surface present mode\n");
        out = DK_ERROR_NOT_AVAILABLE;
        goto formats_cleanup;
    }

    pPresentModes = (VkPresentModeKHR *)DKP_ALLOCATE(
        pAllocator, sizeof *pPresentModes * presentModeCount);
    if (pPresentModes == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the surface present modes\n");
        out = DK_ERROR_ALLOCATION;
        goto formats_cleanup;
    }

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceHandle,
                                                  surfaceHandle,
                                                  &presentModeCount,
                                                  pPresentModes)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not enumerate the surface present modes\n");
        out = DK_ERROR;
        goto present_modes_cleanup;
    }

    out = dkpPickSwapChainPresentMode(
        &pSwapChainProperties->presentMode, presentModeCount, pPresentModes);
    if (out != DK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "could not find a suitable present mode\n");
        goto present_modes_cleanup;
    }

    out = dkpPickSwapChainImageUsage(&pSwapChainProperties->imageUsage,
                                     capabilities);
    if (out != DK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "one or more image usage flags are not supported\n");
        goto present_modes_cleanup;
    }

    dkpPickSwapChainFormat(
        &pSwapChainProperties->format, formatCount, pFormats);
    dkpPickSwapChainMinImageCount(&pSwapChainProperties->minImageCount,
                                  capabilities,
                                  pSwapChainProperties->presentMode);
    dkpPickSwapChainImageExtent(
        &pSwapChainProperties->imageExtent, capabilities, pDesiredImageExtent);
    dkpPickSwapChainPreTransform(&pSwapChainProperties->preTransform,
                                 capabilities);

present_modes_cleanup:
    DKP_FREE(pAllocator, pPresentModes);

formats_cleanup:
    DKP_FREE(pAllocator, pFormats);

exit:
    return out;
}

static enum DkStatus
dkpCheckSwapChainSupport(int *pSupported,
                         VkPhysicalDevice physicalDeviceHandle,
                         VkSurfaceKHR surfaceHandle,
                         const struct DkAllocationCallbacks *pAllocator,
                         const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus status;
    VkExtent2D imageExtent;
    struct DkpSwapChainProperties swapChainProperties;

    DKP_ASSERT(pSupported != NULL);
    DKP_ASSERT(physicalDeviceHandle != NULL);
    DKP_ASSERT(surfaceHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    *pSupported = DKP_FALSE;

    /*
       Use dummy image extent values here as we're only interested in checking
       swap chain support rather than actually creating a valid swap chain.
    */
    imageExtent.width = 0;
    imageExtent.height = 0;
    status = dkpPickSwapChainProperties(&swapChainProperties,
                                        physicalDeviceHandle,
                                        surfaceHandle,
                                        &imageExtent,
                                        pAllocator,
                                        pLogger);
    if (status == DK_SUCCESS) {
        *pSupported = DKP_TRUE;
        return DK_SUCCESS;
    }

    if (status == DK_ERROR_NOT_AVAILABLE) {
        return DK_SUCCESS;
    }

    return DK_ERROR;
}

static enum DkStatus
dkpInspectPhysicalDevice(int *pSuitable,
                         uint32_t *pQueueFamilyIndices,
                         VkPhysicalDevice physicalDeviceHandle,
                         VkSurfaceKHR surfaceHandle,
                         uint32_t extensionCount,
                         const char *const *ppExtensionNames,
                         const struct DkAllocationCallbacks *pAllocator,
                         const struct DkLoggingCallbacks *pLogger)
{
    VkPhysicalDeviceProperties properties;
    int extensionsSupported;
    int swapChainSupported;

    DKP_ASSERT(pSuitable != NULL);
    DKP_ASSERT(pQueueFamilyIndices != NULL);
    DKP_ASSERT(physicalDeviceHandle != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    *pSuitable = DKP_FALSE;

    vkGetPhysicalDeviceProperties(physicalDeviceHandle, &properties);
    if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        return DK_SUCCESS;
    }

    if (dkpCheckDeviceExtensionsSupport(&extensionsSupported,
                                        physicalDeviceHandle,
                                        extensionCount,
                                        ppExtensionNames,
                                        pAllocator,
                                        pLogger)
        != DK_SUCCESS) {
        return DK_ERROR;
    }

    if (!extensionsSupported) {
        return DK_SUCCESS;
    }

    if (dkpPickDeviceQueueFamilies(pQueueFamilyIndices,
                                   physicalDeviceHandle,
                                   surfaceHandle,
                                   pAllocator,
                                   pLogger)
        != DK_SUCCESS) {
        return DK_ERROR;
    }

    if (pQueueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS] == (uint32_t)-1
        || pQueueFamilyIndices[DKP_QUEUE_TYPE_COMPUTE] == (uint32_t)-1
        || pQueueFamilyIndices[DKP_QUEUE_TYPE_TRANSFER] == (uint32_t)-1
        || (surfaceHandle != VK_NULL_HANDLE
            && pQueueFamilyIndices[DKP_QUEUE_TYPE_PRESENT] == (uint32_t)-1)) {
        return DK_SUCCESS;
    }

    if (surfaceHandle != VK_NULL_HANDLE) {
        if (dkpCheckSwapChainSupport(&swapChainSupported,
                                     physicalDeviceHandle,
                                     surfaceHandle,
                                     pAllocator,
                                     pLogger)
            != DK_SUCCESS) {
            return DK_ERROR;
        }

        if (!swapChainSupported) {
            return DK_SUCCESS;
        }
    }

    *pSuitable = DKP_TRUE;
    return DK_SUCCESS;
}

static enum DkStatus
dkpPickPhysicalDevice(VkPhysicalDevice *pPhysicalDeviceHandle,
                      uint32_t *pQueueFamilyIndices,
                      VkInstance instanceHandle,
                      VkSurfaceKHR surfaceHandle,
                      uint32_t extensionCount,
                      const char *const *ppExtensionNames,
                      const struct DkAllocationCallbacks *pAllocator,
                      const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t physicalDeviceCount;
    VkPhysicalDevice *pPhysicalDeviceHandles;

    DKP_ASSERT(pPhysicalDeviceHandle != NULL);
    DKP_ASSERT(pQueueFamilyIndices != NULL);
    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount, NULL)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not count the number of the physical devices\n");
        out = DK_ERROR;
        goto exit;
    }

    if (physicalDeviceCount == 0) {
        DKP_LOG_TRACE(pLogger, "could not fine a suitable physical device\n");
        out = DK_ERROR;
        goto exit;
    }

    pPhysicalDeviceHandles = (VkPhysicalDevice *)DKP_ALLOCATE(
        pAllocator, sizeof *pPhysicalDeviceHandles * physicalDeviceCount);
    if (pPhysicalDeviceHandles == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the physical devices\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkEnumeratePhysicalDevices(
            instanceHandle, &physicalDeviceCount, pPhysicalDeviceHandles)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "could not enumerate the physical devices\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

    *pPhysicalDeviceHandle = NULL;
    for (i = 0; i < physicalDeviceCount; ++i) {
        int suitable;

        out = dkpInspectPhysicalDevice(&suitable,
                                       pQueueFamilyIndices,
                                       pPhysicalDeviceHandles[i],
                                       surfaceHandle,
                                       extensionCount,
                                       ppExtensionNames,
                                       pAllocator,
                                       pLogger);
        if (out != DK_SUCCESS) {
            goto physical_devices_cleanup;
        }

        if (suitable) {
            *pPhysicalDeviceHandle = pPhysicalDeviceHandles[i];
            break;
        }
    }

    if (*pPhysicalDeviceHandle == NULL) {
        DKP_LOG_TRACE(pLogger, "could not find a suitable physical device\n");
        out = DK_ERROR;
        goto physical_devices_cleanup;
    }

physical_devices_cleanup:
    DKP_FREE(pAllocator, pPhysicalDeviceHandles);

exit:
    return out;
}

static enum DkStatus
dkpInitializeDevice(struct DkpDevice *pDevice,
                    VkInstance instanceHandle,
                    VkSurfaceKHR surfaceHandle,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const struct DkAllocationCallbacks *pAllocator,
                    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    enum DkpPresentSupport presentSupport;
    uint32_t extensionCount;
    const char **ppExtensionNames;
    uint32_t queueCount;
    float *pQueuePriorities;
    VkDeviceQueueCreateInfo *pQueueInfos;
    VkDeviceCreateInfo createInfo;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(instanceHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    presentSupport = surfaceHandle == VK_NULL_HANDLE
                         ? DKP_PRESENT_SUPPORT_DISABLED
                         : DKP_PRESENT_SUPPORT_ENABLED;

    out = dkpCreateDeviceExtensionNames(&extensionCount,
                                        &ppExtensionNames,
                                        presentSupport,
                                        pAllocator,
                                        pLogger);
    if (out != DK_SUCCESS) {
        goto exit;
    }

    out = dkpPickPhysicalDevice(&pDevice->physicalHandle,
                                pDevice->queueFamilyIndices,
                                instanceHandle,
                                surfaceHandle,
                                extensionCount,
                                ppExtensionNames,
                                pAllocator,
                                pLogger);
    if (out != DK_SUCCESS) {
        goto extension_names_cleanup;
    }

    queueCount = 1;
    pQueuePriorities = (float *)DKP_ALLOCATE(
        pAllocator, sizeof *pQueuePriorities * queueCount);
    if (pQueuePriorities == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the device queue priorities\n");
        out = DK_ERROR_ALLOCATION;
        goto extension_names_cleanup;
    }

    for (i = 0; i < queueCount; ++i) {
        pQueuePriorities[i] = 1.0f;
    }

    dkpFilterQueueFamilyIndices(&pDevice->filteredQueueFamilyCount,
                                pDevice->filteredQueueFamilyIndices,
                                pDevice->queueFamilyIndices);

    pQueueInfos = (VkDeviceQueueCreateInfo *)DKP_ALLOCATE(
        pAllocator, sizeof *pQueueInfos * pDevice->filteredQueueFamilyCount);
    if (pQueueInfos == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the device queue infos\n");
        out = DK_ERROR_ALLOCATION;
        goto queue_priorities_cleanup;
    }

    for (i = 0; i < pDevice->filteredQueueFamilyCount; ++i) {
        pQueueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        pQueueInfos[i].pNext = NULL;
        pQueueInfos[i].flags = 0;
        pQueueInfos[i].queueFamilyIndex
            = pDevice->filteredQueueFamilyIndices[i];
        pQueueInfos[i].queueCount = queueCount;
        pQueueInfos[i].pQueuePriorities = pQueuePriorities;
    }

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.queueCreateInfoCount = pDevice->filteredQueueFamilyCount;
    createInfo.pQueueCreateInfos = pQueueInfos;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;
    createInfo.pEnabledFeatures = NULL;

    switch (vkCreateDevice(pDevice->physicalHandle,
                           &createInfo,
                           pBackEndAllocator,
                           &pDevice->logicalHandle)) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            DKP_LOG_TRACE(
                pLogger,
                "some requested device extensions are not supported\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            DKP_LOG_TRACE(pLogger,
                          "some requested device features are not supported\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        case VK_ERROR_TOO_MANY_OBJECTS:
            DKP_LOG_TRACE(pLogger,
                          "too many devices have already been created\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        case VK_ERROR_DEVICE_LOST:
            DKP_LOG_TRACE(pLogger, "the device has been lost\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
        default:
            DKP_LOG_TRACE(pLogger, "failed to create the device\n");
            out = DK_ERROR;
            goto queue_infos_cleanup;
    }

queue_infos_cleanup:
    DKP_FREE(pAllocator, pQueueInfos);

queue_priorities_cleanup:
    DKP_FREE(pAllocator, pQueuePriorities);

extension_names_cleanup:
    dkpDestroyDeviceExtensionNames(ppExtensionNames, pAllocator);

exit:
    return out;
}

static void
dkpTerminateDevice(struct DkpDevice *pDevice,
                   const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroyDevice(pDevice->logicalHandle, pBackEndAllocator);
}

static enum DkStatus
dkpGetDeviceQueues(struct DkpQueues *pQueues, const struct DkpDevice *pDevice)
{
    uint32_t i;

    DKP_ASSERT(pQueues != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);

    pQueues->graphicsHandle = NULL;
    pQueues->computeHandle = NULL;
    pQueues->transferHandle = NULL;
    pQueues->presentHandle = NULL;

    for (i = 0; i < DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED; ++i) {
        VkQueue *pQueue;

        if (pDevice->queueFamilyIndices[i] == (uint32_t)-1) {
            continue;
        }

        switch (i) {
            case DKP_QUEUE_TYPE_GRAPHICS:
                pQueue = &pQueues->graphicsHandle;
                break;
            case DKP_QUEUE_TYPE_COMPUTE:
                pQueue = &pQueues->computeHandle;
                break;
            case DKP_QUEUE_TYPE_TRANSFER:
                pQueue = &pQueues->transferHandle;
                break;
            case DKP_QUEUE_TYPE_PRESENT:
                pQueue = &pQueues->presentHandle;
                break;
            default:
                DKP_ASSERT(0);
                return DK_ERROR;
        }

        vkGetDeviceQueue(
            pDevice->logicalHandle, pDevice->queueFamilyIndices[i], 0, pQueue);
    }

    return DK_SUCCESS;
}

static enum DkStatus
dkpCreateSemaphores(VkSemaphore **ppSemaphoreHandles,
                    const struct DkpDevice *pDevice,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const struct DkAllocationCallbacks *pAllocator,
                    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    unsigned int i;
    VkSemaphoreCreateInfo createInfo;

    DKP_ASSERT(ppSemaphoreHandles != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    DKP_ASSERT(DKP_SEMAPHORE_ID_ENUM_COUNT > 0);
    *ppSemaphoreHandles = (VkSemaphore *)DKP_ALLOCATE(
        pAllocator, sizeof **ppSemaphoreHandles * DKP_SEMAPHORE_ID_ENUM_COUNT);
    if (*ppSemaphoreHandles == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the semaphores\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        (*ppSemaphoreHandles)[i] = VK_NULL_HANDLE;
    }

    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        if (vkCreateSemaphore(pDevice->logicalHandle,
                              &createInfo,
                              pBackEndAllocator,
                              &(*ppSemaphoreHandles)[i])
            != VK_SUCCESS) {
            const char *pSemaphoreDescription;

            dkpGetSemaphoreIdDescription(&pSemaphoreDescription,
                                         (enum DkpSemaphoreId)i);

            DKP_LOG_TRACE(pLogger,
                          "failed to create a '%s' semaphore\n",
                          pSemaphoreDescription);
            out = DK_ERROR;
            goto semaphores_undo;
        }
    }

    goto exit;

semaphores_undo:
    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        if ((*ppSemaphoreHandles)[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(pDevice->logicalHandle,
                               (*ppSemaphoreHandles)[i],
                               pBackEndAllocator);
        }
    }

    DKP_FREE(pAllocator, *ppSemaphoreHandles);

exit:
    return out;
}

static void
dkpDestroySemaphores(const struct DkpDevice *pDevice,
                     VkSemaphore *pSemaphoreHandles,
                     const VkAllocationCallbacks *pBackEndAllocator,
                     const struct DkAllocationCallbacks *pAllocator)
{
    unsigned int i;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSemaphoreHandles != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);

    for (i = 0; i < DKP_SEMAPHORE_ID_ENUM_COUNT; ++i) {
        DKP_ASSERT(pSemaphoreHandles[i] != VK_NULL_HANDLE);
        vkDestroySemaphore(
            pDevice->logicalHandle, pSemaphoreHandles[i], pBackEndAllocator);
    }

    DKP_FREE(pAllocator, pSemaphoreHandles);
}

static enum DkStatus
dkpCreateShaderModule(VkShaderModule *pShaderModuleHandle,
                      const struct DkpDevice *pDevice,
                      size_t shaderCodeSize,
                      const uint32_t *pShaderCode,
                      const VkAllocationCallbacks *pBackEndAllocator,
                      const struct DkLoggingCallbacks *pLogger)
{
    VkShaderModuleCreateInfo createInfo;

    DKP_ASSERT(pShaderModuleHandle != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pShaderCode != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.codeSize = shaderCodeSize;
    createInfo.pCode = pShaderCode;

    if (vkCreateShaderModule(pDevice->logicalHandle,
                             &createInfo,
                             pBackEndAllocator,
                             pShaderModuleHandle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to create the shader module\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
dkpDestroyShaderModule(const struct DkpDevice *pDevice,
                       VkShaderModule shaderModuleHandle,
                       const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(shaderModuleHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroyShaderModule(
        pDevice->logicalHandle, shaderModuleHandle, pBackEndAllocator);
}

static enum DkStatus
dkpCreateShaders(struct DkpShader **ppShaders,
                 const struct DkpDevice *pDevice,
                 uint32_t shaderCount,
                 const struct DkShaderCreateInfo *pShaderInfos,
                 const VkAllocationCallbacks *pBackEndAllocator,
                 const struct DkAllocationCallbacks *pAllocator,
                 const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;

    DKP_ASSERT(ppShaders != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(shaderCount > 0);
    DKP_ASSERT(pShaderInfos != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    *ppShaders = (struct DkpShader *)DKP_ALLOCATE(
        pAllocator, sizeof **ppShaders * shaderCount);
    if (*ppShaders == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the shaders\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < shaderCount; ++i) {
        (*ppShaders)[i].moduleHandle = VK_NULL_HANDLE;
    }

    for (i = 0; i < shaderCount; ++i) {
        VkShaderStageFlagBits backEndShaderStage;

        out = dkpCreateShaderModule(&(*ppShaders)[i].moduleHandle,
                                    pDevice,
                                    (size_t)pShaderInfos[i].codeSize,
                                    (uint32_t *)pShaderInfos[i].pCode,
                                    pBackEndAllocator,
                                    pLogger);
        if (out != DK_SUCCESS) {
            goto shaders_undo;
        }

        dkpTranslateShaderStageToBackEnd(&backEndShaderStage,
                                         pShaderInfos[i].stage);

        (*ppShaders)[i].stage = backEndShaderStage;
        (*ppShaders)[i].pEntryPointName = pShaderInfos[i].pEntryPointName;
    }

    goto exit;

shaders_undo:
    for (i = 0; i < shaderCount; ++i) {
        if ((*ppShaders)[i].moduleHandle != VK_NULL_HANDLE) {
            dkpDestroyShaderModule(
                pDevice, (*ppShaders)[i].moduleHandle, pBackEndAllocator);
        }
    }

    DKP_FREE(pAllocator, *ppShaders);

exit:
    return out;
}

static void
dkpDestroyShaders(const struct DkpDevice *pDevice,
                  uint32_t shaderCount,
                  struct DkpShader *pShaders,
                  const VkAllocationCallbacks *pBackEndAllocator,
                  const struct DkAllocationCallbacks *pAllocator)
{
    uint32_t i;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pShaders != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);

    for (i = 0; i < shaderCount; ++i) {
        DKP_ASSERT(pShaders[i].moduleHandle != NULL);
        dkpDestroyShaderModule(
            pDevice, pShaders[i].moduleHandle, pBackEndAllocator);
    }

    DKP_FREE(pAllocator, pShaders);
}

static enum DkStatus
dkpCreateVertexBuffers(
    struct DkpBuffer **ppVertexBuffers,
    const struct DkpDevice *pDevice,
    uint32_t vertexBufferCount,
    const struct DkVertexBufferCreateInfo *pVertexBufferInfos,
    VkCommandPool commandPoolHandle,
    const struct DkpQueues *pQueues,
    const VkAllocationCallbacks *pBackEndAllocator,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    struct DkpBuffer stagingBuffer;

    DKP_ASSERT(ppVertexBuffers != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(commandPoolHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pQueues != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vertexBufferCount == 0) {
        *ppVertexBuffers = NULL;
        goto exit;
    }

    *ppVertexBuffers = (struct DkpBuffer *)DKP_ALLOCATE(
        pAllocator, sizeof **ppVertexBuffers * vertexBufferCount);
    if (*ppVertexBuffers == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the vertex buffers\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < vertexBufferCount; ++i) {
        (*ppVertexBuffers)[i].handle = VK_NULL_HANDLE;
        (*ppVertexBuffers)[i].memoryHandle = VK_NULL_HANDLE;
    }

    for (i = 0; i < vertexBufferCount; ++i) {
        void *pData;

        out = dkpInitializeBuffer(&stagingBuffer,
                                  pDevice,
                                  (VkDeviceSize)pVertexBufferInfos[i].size,
                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  pBackEndAllocator,
                                  pLogger);
        if (out != DK_SUCCESS) {
            stagingBuffer.handle = VK_NULL_HANDLE;
            stagingBuffer.memoryHandle = VK_NULL_HANDLE;
            goto vertex_buffers_undo;
        }

        if (vkMapMemory(pDevice->logicalHandle,
                        stagingBuffer.memoryHandle,
                        (VkDeviceSize)pVertexBufferInfos[i].offset,
                        (VkDeviceSize)pVertexBufferInfos[i].size,
                        0,
                        &pData)
            != VK_SUCCESS) {
            DKP_LOG_TRACE(pLogger,
                          "failed to map a vertex staging buffer memory\n");
            out = DK_ERROR;
            goto vertex_buffers_undo;
        }

        memcpy(pData,
               pVertexBufferInfos[i].pData,
               (size_t)pVertexBufferInfos[i].size);
        vkUnmapMemory(pDevice->logicalHandle, stagingBuffer.memoryHandle);

        out = dkpInitializeBuffer(&(*ppVertexBuffers)[i],
                                  pDevice,
                                  (VkDeviceSize)pVertexBufferInfos[i].size,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                      | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  pBackEndAllocator,
                                  pLogger);
        if (out != DK_SUCCESS) {
            (*ppVertexBuffers)[i].handle = VK_NULL_HANDLE;
            (*ppVertexBuffers)[i].memoryHandle = VK_NULL_HANDLE;
            goto vertex_buffers_undo;
        }

        (*ppVertexBuffers)[i].offset
            = (VkDeviceSize)pVertexBufferInfos[i].offset;

        dkpCopyBuffer(pDevice,
                      &(*ppVertexBuffers)[i],
                      &stagingBuffer,
                      (VkDeviceSize)pVertexBufferInfos[i].size,
                      commandPoolHandle,
                      pQueues,
                      pLogger);

        dkpTerminateBuffer(pDevice, &stagingBuffer, pBackEndAllocator);
        stagingBuffer.handle = VK_NULL_HANDLE;
        stagingBuffer.memoryHandle = VK_NULL_HANDLE;
    }

    if (stagingBuffer.handle != VK_NULL_HANDLE
        || stagingBuffer.memoryHandle != VK_NULL_HANDLE) {
        dkpTerminateBuffer(pDevice, &stagingBuffer, pBackEndAllocator);
    }

    goto exit;

vertex_buffers_undo:
    for (i = 0; i < vertexBufferCount; ++i) {
        if ((*ppVertexBuffers)[i].handle != VK_NULL_HANDLE
            || (*ppVertexBuffers)[i].memoryHandle != VK_NULL_HANDLE) {
            dkpTerminateBuffer(
                pDevice, &(*ppVertexBuffers)[i], pBackEndAllocator);
        }
    }

    DKP_FREE(pAllocator, *ppVertexBuffers);

exit:
    return out;
}

static void
dkpDestroyVertexBuffers(const struct DkpDevice *pDevice,
                        uint32_t vertexBufferCount,
                        struct DkpBuffer *pVertexBuffers,
                        const VkAllocationCallbacks *pBackEndAllocator,
                        const struct DkAllocationCallbacks *pAllocator)
{
    uint32_t i;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);

    for (i = 0; i < vertexBufferCount; ++i) {
        DKP_ASSERT(pVertexBuffers[i].handle != VK_NULL_HANDLE);
        DKP_ASSERT(pVertexBuffers[i].memoryHandle != VK_NULL_HANDLE);
        dkpTerminateBuffer(pDevice, &pVertexBuffers[i], pBackEndAllocator);
    }

    if (pVertexBuffers != NULL) {
        DKP_FREE(pAllocator, pVertexBuffers);
    }
}

static enum DkStatus
dkpCreateSwapChainImages(uint32_t *pImageCount,
                         VkImage **ppImageHandles,
                         const struct DkpDevice *pDevice,
                         VkSwapchainKHR swapChainHandle,
                         const struct DkAllocationCallbacks *pAllocator,
                         const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;

    DKP_ASSERT(pImageCount != NULL);
    DKP_ASSERT(ppImageHandles != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(swapChainHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vkGetSwapchainImagesKHR(
            pDevice->logicalHandle, swapChainHandle, pImageCount, NULL)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "could not retrieve the number of swap chain images\n");
        out = DK_ERROR;
        goto exit;
    }

    if (*pImageCount == 0) {
        DKP_LOG_TRACE(pLogger, "could not find a suitable swap chain image\n");
        out = DK_ERROR;
        goto exit;
    }

    *ppImageHandles = (VkImage *)DKP_ALLOCATE(
        pAllocator, sizeof **ppImageHandles * *pImageCount);
    if (*ppImageHandles == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the swap chain images\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkGetSwapchainImagesKHR(pDevice->logicalHandle,
                                swapChainHandle,
                                pImageCount,
                                *ppImageHandles)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "could not retrieve the swap chain images\n");
        out = DK_ERROR;
        goto images_undo;
    }

    goto exit;

images_undo:
    DKP_FREE(pAllocator, *ppImageHandles);

exit:
    return out;
}

static void
dkpDestroySwapChainImages(VkImage *pImageHandles,
                          const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pImageHandles != NULL);
    DKP_ASSERT(pAllocator != NULL);

    DKP_FREE(pAllocator, pImageHandles);
}

static enum DkStatus
dkpCreateSwapChainImageViews(VkImageView **ppImageViewHandles,
                             const struct DkpDevice *pDevice,
                             uint32_t imageCount,
                             const VkImage *pImageHandles,
                             VkFormat format,
                             const VkAllocationCallbacks *pBackEndAllocator,
                             const struct DkAllocationCallbacks *pAllocator,
                             const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    VkImageViewCreateInfo createInfo;

    DKP_ASSERT(ppImageViewHandles != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(imageCount > 0);
    DKP_ASSERT(pImageHandles != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    *ppImageViewHandles = (VkImageView *)DKP_ALLOCATE(
        pAllocator, sizeof **ppImageViewHandles * imageCount);
    if (*ppImageViewHandles == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the swap chain image views\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < imageCount; ++i) {
        (*ppImageViewHandles)[i] = VK_NULL_HANDLE;
    }

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

        if (vkCreateImageView(pDevice->logicalHandle,
                              &createInfo,
                              pBackEndAllocator,
                              &(*ppImageViewHandles)[i])
            != VK_SUCCESS) {
            DKP_LOG_TRACE(pLogger, "failed to create an image view\n");
            out = DK_ERROR;
            goto image_views_undo;
        }
    }

    goto exit;

image_views_undo:
    for (i = 0; i < imageCount; ++i) {
        if ((*ppImageViewHandles)[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(pDevice->logicalHandle,
                               (*ppImageViewHandles)[i],
                               pBackEndAllocator);
        }
    }

    DKP_FREE(pAllocator, *ppImageViewHandles);

exit:
    return out;
}

static void
dkpDestroySwapChainImageViews(const struct DkpDevice *pDevice,
                              uint32_t imageCount,
                              VkImageView *pImageViewHandles,
                              const VkAllocationCallbacks *pBackEndAllocator,
                              const struct DkAllocationCallbacks *pAllocator)
{
    uint32_t i;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pImageViewHandles != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);

    for (i = 0; i < imageCount; ++i) {
        DKP_ASSERT(pImageViewHandles[i] != VK_NULL_HANDLE);
        vkDestroyImageView(
            pDevice->logicalHandle, pImageViewHandles[i], pBackEndAllocator);
    }

    DKP_FREE(pAllocator, pImageViewHandles);
}

static enum DkStatus
dkpInitializeSwapChain(struct DkpSwapChain *pSwapChain,
                       const struct DkpDevice *pDevice,
                       VkSurfaceKHR surfaceHandle,
                       const VkExtent2D *pDesiredImageExtent,
                       VkSwapchainKHR oldSwapChainHandle,
                       const VkAllocationCallbacks *pBackEndAllocator,
                       const struct DkAllocationCallbacks *pAllocator,
                       const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    struct DkpSwapChainProperties swapChainProperties;
    VkSharingMode imageSharingMode;
    uint32_t queueFamilyIndexCount;
    uint32_t *pQueueFamilyIndices;
    VkSwapchainCreateInfoKHR createInfo;

    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->physicalHandle != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    out = dkpPickSwapChainProperties(&swapChainProperties,
                                     pDevice->physicalHandle,
                                     surfaceHandle,
                                     pDesiredImageExtent,
                                     pAllocator,
                                     pLogger);
    if (out != DK_SUCCESS) {
        goto exit;
    }

    if (pDevice->queueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS]
        != pDevice->queueFamilyIndices[DKP_QUEUE_TYPE_PRESENT]) {
        imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        queueFamilyIndexCount = 2;
        pQueueFamilyIndices = (uint32_t *)DKP_ALLOCATE(
            pAllocator, sizeof *pQueueFamilyIndices * queueFamilyIndexCount);
        if (pQueueFamilyIndices == NULL) {
            DKP_LOG_TRACE(
                pLogger,
                "failed to allocate the swap chain's queue family indices\n");
            out = DK_ERROR_ALLOCATION;
            goto exit;
        }

        pQueueFamilyIndices[0]
            = pDevice->queueFamilyIndices[DKP_QUEUE_TYPE_GRAPHICS];
        pQueueFamilyIndices[1]
            = pDevice->queueFamilyIndices[DKP_QUEUE_TYPE_PRESENT];
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

    switch (vkCreateSwapchainKHR(pDevice->logicalHandle,
                                 &createInfo,
                                 pBackEndAllocator,
                                 &pSwapChain->handle)) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            DKP_LOG_TRACE(pLogger,
                          "the swap chain's surface is already in use\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
        case VK_ERROR_DEVICE_LOST:
            DKP_LOG_TRACE(pLogger, "the swap chain's device has been lost\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
        case VK_ERROR_SURFACE_LOST_KHR:
            DKP_LOG_TRACE(pLogger, "the swap chain's surface has been lost\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
        default:
            DKP_LOG_TRACE(pLogger, "failed to create the swap chain\n");
            out = DK_ERROR;
            goto queue_family_indices_cleanup;
    }

    pSwapChain->imageExtent = swapChainProperties.imageExtent;

    out = dkpCreateSwapChainImages(&pSwapChain->imageCount,
                                   &pSwapChain->pImageHandles,
                                   pDevice,
                                   pSwapChain->handle,
                                   pAllocator,
                                   pLogger);
    if (out != DK_SUCCESS) {
        goto queue_family_indices_cleanup;
    }

    out = dkpCreateSwapChainImageViews(&pSwapChain->pImageViewHandles,
                                       pDevice,
                                       pSwapChain->imageCount,
                                       pSwapChain->pImageHandles,
                                       swapChainProperties.format.format,
                                       pBackEndAllocator,
                                       pAllocator,
                                       pLogger);
    if (out != DK_SUCCESS) {
        goto images_undo;
    }

    pSwapChain->format = swapChainProperties.format;
    goto cleanup;

images_undo:
    dkpDestroySwapChainImages(pSwapChain->pImageHandles, pAllocator);

cleanup:;

queue_family_indices_cleanup:
    if (pQueueFamilyIndices != NULL) {
        DKP_FREE(pAllocator, pQueueFamilyIndices);
    }

exit:
    if (oldSwapChainHandle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            pDevice->logicalHandle, oldSwapChainHandle, pBackEndAllocator);
    }

    return out;
}

static void
dkpTerminateSwapChain(const struct DkpDevice *pDevice,
                      struct DkpSwapChain *pSwapChain,
                      enum DkpOldSwapChainPreservation oldSwapChainPreservation,
                      const VkAllocationCallbacks *pBackEndAllocator,
                      const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pSwapChain->handle != VK_NULL_HANDLE);
    DKP_ASSERT(pSwapChain->pImageHandles != NULL);
    DKP_ASSERT(pSwapChain->pImageViewHandles != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);

    dkpDestroySwapChainImageViews(pDevice,
                                  pSwapChain->imageCount,
                                  pSwapChain->pImageViewHandles,
                                  pBackEndAllocator,
                                  pAllocator);
    dkpDestroySwapChainImages(pSwapChain->pImageHandles, pAllocator);

    if (oldSwapChainPreservation == DKP_OLD_SWAP_CHAIN_PRESERVATION_DISABLED) {
        vkDestroySwapchainKHR(
            pDevice->logicalHandle, pSwapChain->handle, pBackEndAllocator);
    }
}

static enum DkStatus
dkpCreateRenderPass(VkRenderPass *pRenderPassHandle,
                    const struct DkpDevice *pDevice,
                    const struct DkpSwapChain *pSwapChain,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    const struct DkAllocationCallbacks *pAllocator,
                    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t colorAttachmentCount;
    VkAttachmentDescription *pColorAttachments;
    VkAttachmentReference *pColorAttachmentReferences;
    uint32_t subpassCount;
    VkSubpassDescription *pSubpasses;
    uint32_t subpassDependencyCount;
    VkSubpassDependency *pSubpassDependencies;
    VkRenderPassCreateInfo renderPassInfo;

    DKP_ASSERT(pRenderPassHandle != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    colorAttachmentCount = 1;
    pColorAttachments = (VkAttachmentDescription *)DKP_ALLOCATE(
        pAllocator, sizeof *pColorAttachments * colorAttachmentCount);
    if (pColorAttachments == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the color attachments\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    pColorAttachmentReferences = (VkAttachmentReference *)DKP_ALLOCATE(
        pAllocator, sizeof *pColorAttachmentReferences * colorAttachmentCount);
    if (pColorAttachmentReferences == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the color attachment references\n");
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
        pColorAttachmentReferences[i].layout
            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    subpassCount = 1;
    pSubpasses = (VkSubpassDescription *)DKP_ALLOCATE(
        pAllocator, sizeof *pSubpasses * subpassCount);
    if (pSubpasses == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the subpasses\n");
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

    subpassDependencyCount = 1;
    pSubpassDependencies = (VkSubpassDependency *)DKP_ALLOCATE(
        pAllocator, sizeof *pSubpassDependencies * subpassDependencyCount);
    if (pSubpassDependencies == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the subpass dependencies\n");
        out = DK_ERROR_ALLOCATION;
        goto subpasses_cleanup;
    }

    pSubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    pSubpassDependencies[0].dstSubpass = 0;
    pSubpassDependencies[0].srcStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    pSubpassDependencies[0].dstStageMask
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    pSubpassDependencies[0].srcAccessMask = 0;
    pSubpassDependencies[0].dstAccessMask
        = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
          | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    pSubpassDependencies[0].dependencyFlags = 0;

    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = NULL;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = colorAttachmentCount;
    renderPassInfo.pAttachments = pColorAttachments;
    renderPassInfo.subpassCount = subpassCount;
    renderPassInfo.pSubpasses = pSubpasses;
    renderPassInfo.dependencyCount = subpassDependencyCount;
    renderPassInfo.pDependencies = pSubpassDependencies;

    if (vkCreateRenderPass(pDevice->logicalHandle,
                           &renderPassInfo,
                           pBackEndAllocator,
                           pRenderPassHandle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to created the render pass\n");
        out = DK_ERROR;
        goto subpass_dependencies_cleanup;
    }

subpass_dependencies_cleanup:
    DKP_FREE(pAllocator, pSubpassDependencies);

subpasses_cleanup:
    DKP_FREE(pAllocator, pSubpasses);

color_attachment_references_cleanup:
    DKP_FREE(pAllocator, pColorAttachmentReferences);

color_attachments_cleanup:
    DKP_FREE(pAllocator, pColorAttachments);

exit:
    return out;
}

static void
dkpDestroyRenderPass(const struct DkpDevice *pDevice,
                     VkRenderPass renderPassHandle,
                     const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(renderPassHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroyRenderPass(
        pDevice->logicalHandle, renderPassHandle, pBackEndAllocator);
}

static enum DkStatus
dkpCreatePipelineLayout(VkPipelineLayout *pPipelineLayoutHandle,
                        const struct DkpDevice *pDevice,
                        const VkAllocationCallbacks *pBackEndAllocator,
                        const struct DkLoggingCallbacks *pLogger)
{
    VkPipelineLayoutCreateInfo layoutInfo;

    DKP_ASSERT(pPipelineLayoutHandle != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = NULL;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = NULL;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = 0;

    if (vkCreatePipelineLayout(pDevice->logicalHandle,
                               &layoutInfo,
                               pBackEndAllocator,
                               pPipelineLayoutHandle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to create the pipeline layout\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
dkpDestroyPipelineLayout(const struct DkpDevice *pDevice,
                         VkPipelineLayout pipelineLayoutHandle,
                         const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pipelineLayoutHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroyPipelineLayout(
        pDevice->logicalHandle, pipelineLayoutHandle, pBackEndAllocator);
}

static enum DkStatus
dkpCreateGraphicsPipeline(
    VkPipeline *pPipelineHandle,
    const struct DkpDevice *pDevice,
    VkPipelineLayout pipelineLayoutHandle,
    VkRenderPass renderPassHandle,
    uint32_t shaderCount,
    const struct DkpShader *pShaders,
    const VkExtent2D *pImageExtent,
    uint32_t vertexBindingDescriptionCount,
    const VkVertexInputBindingDescription *pVertexBindingDescriptions,
    uint32_t vertexAttributeDescriptionCount,
    const VkVertexInputAttributeDescription *pVertexAttributeDescriptions,
    const VkAllocationCallbacks *pBackEndAllocator,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
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

    DKP_ASSERT(pPipelineHandle != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pipelineLayoutHandle != VK_NULL_HANDLE);
    DKP_ASSERT(renderPassHandle != VK_NULL_HANDLE);
    DKP_ASSERT(shaderCount > 0);
    DKP_ASSERT(pShaders != NULL);
    DKP_ASSERT(pImageExtent != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    pShaderStageInfos = (VkPipelineShaderStageCreateInfo *)DKP_ALLOCATE(
        pAllocator, sizeof *pShaderStageInfos * shaderCount);
    if (pShaderStageInfos == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the shader stage infos\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < shaderCount; ++i) {
        pShaderStageInfos[i].sType
            = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pShaderStageInfos[i].pNext = NULL;
        pShaderStageInfos[i].flags = 0;
        pShaderStageInfos[i].stage = pShaders[i].stage;
        pShaderStageInfos[i].module = pShaders[i].moduleHandle;
        pShaderStageInfos[i].pName = pShaders[i].pEntryPointName;
        pShaderStageInfos[i].pSpecializationInfo = NULL;
    }

    viewportCount = 1;
    pViewports = (VkViewport *)DKP_ALLOCATE(pAllocator,
                                            sizeof *pViewports * viewportCount);
    if (pViewports == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the viewports\n");
        out = DK_ERROR_ALLOCATION;
        goto shader_stage_infos_cleanup;
    }

    pViewports[0].x = 0.0f;
    pViewports[0].y = 0.0f;
    pViewports[0].width = (float)pImageExtent->width;
    pViewports[0].height = (float)pImageExtent->height;
    pViewports[0].minDepth = 0.0f;
    pViewports[0].maxDepth = 1.0f;

    scissorCount = 1;
    pScissors = (VkRect2D *)DKP_ALLOCATE(pAllocator,
                                         sizeof *pScissors * scissorCount);
    if (pScissors == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the scissors\n");
        out = DK_ERROR_ALLOCATION;
        goto viewports_cleanup;
    }

    pScissors[0].offset.x = 0;
    pScissors[0].offset.y = 0;
    pScissors[0].extent = *pImageExtent;

    colorBlendAttachmentStateCount = 1;
    pColorBlendAttachmentStates
        = (VkPipelineColorBlendAttachmentState *)DKP_ALLOCATE(
            pAllocator,
            (sizeof *pColorBlendAttachmentStates
             * colorBlendAttachmentStateCount));
    if (pColorBlendAttachmentStates == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the color blend attachment states\n");
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
    pColorBlendAttachmentStates[0].colorWriteMask
        = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    vertexInputStateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfo.pNext = NULL;
    vertexInputStateInfo.flags = 0;
    vertexInputStateInfo.vertexBindingDescriptionCount
        = vertexBindingDescriptionCount;
    vertexInputStateInfo.pVertexBindingDescriptions
        = pVertexBindingDescriptions;
    vertexInputStateInfo.vertexAttributeDescriptionCount
        = vertexAttributeDescriptionCount;
    vertexInputStateInfo.pVertexAttributeDescriptions
        = pVertexAttributeDescriptions;

    inputAssemblyStateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.pNext = NULL;
    inputAssemblyStateInfo.flags = 0;
    inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;

    viewportStateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.pNext = NULL;
    viewportStateInfo.flags = 0;
    viewportStateInfo.viewportCount = viewportCount;
    viewportStateInfo.pViewports = pViewports;
    viewportStateInfo.scissorCount = scissorCount;
    viewportStateInfo.pScissors = pScissors;

    rasterizationStateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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

    multisampleStateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.pNext = NULL;
    multisampleStateInfo.flags = 0;
    multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateInfo.minSampleShading = 1.0;
    multisampleStateInfo.pSampleMask = NULL;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;

    colorBlendStateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
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
    pCreateInfos = (VkGraphicsPipelineCreateInfo *)DKP_ALLOCATE(
        pAllocator, sizeof *pCreateInfos * createInfoCount);
    if (pCreateInfos == NULL) {
        DKP_LOG_TRACE(
            pLogger, "failed to allocate the graphics pipeline create infos\n");
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

    if (vkCreateGraphicsPipelines(pDevice->logicalHandle,
                                  VK_NULL_HANDLE,
                                  createInfoCount,
                                  pCreateInfos,
                                  pBackEndAllocator,
                                  pPipelineHandle)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger, "failed to create the graphics pipelines\n");
        out = DK_ERROR;
        goto create_infos_cleanup;
    }

create_infos_cleanup:
    DKP_FREE(pAllocator, pCreateInfos);

color_blend_attachment_states_cleanup:
    DKP_FREE(pAllocator, pColorBlendAttachmentStates);

scissors_cleanup:
    DKP_FREE(pAllocator, pScissors);

viewports_cleanup:
    DKP_FREE(pAllocator, pViewports);

shader_stage_infos_cleanup:
    DKP_FREE(pAllocator, pShaderStageInfos);

exit:
    return out;
}

static void
dkpDestroyGraphicsPipeline(const struct DkpDevice *pDevice,
                           VkPipeline pipelineHandle,
                           const VkAllocationCallbacks *pBackEndAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pipelineHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pBackEndAllocator != NULL);

    vkDestroyPipeline(
        pDevice->logicalHandle, pipelineHandle, pBackEndAllocator);
}

static enum DkStatus
dkpCreateFramebuffers(VkFramebuffer **ppFramebufferHandles,
                      const struct DkpDevice *pDevice,
                      const struct DkpSwapChain *pSwapChain,
                      VkRenderPass renderPassHandle,
                      const VkExtent2D *pImageExtent,
                      const VkAllocationCallbacks *pBackEndAllocator,
                      const struct DkAllocationCallbacks *pAllocator,
                      const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;

    DKP_ASSERT(ppFramebufferHandles != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pSwapChain->imageCount > 0);
    DKP_ASSERT(pSwapChain->pImageViewHandles != NULL);
    DKP_ASSERT(renderPassHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pImageExtent != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    *ppFramebufferHandles = (VkFramebuffer *)DKP_ALLOCATE(
        pAllocator, sizeof **ppFramebufferHandles * pSwapChain->imageCount);
    if (*ppFramebufferHandles == NULL) {
        DKP_LOG_TRACE(pLogger, "failed to allocate the frame buffers\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    for (i = 0; i < pSwapChain->imageCount; ++i) {
        (*ppFramebufferHandles)[i] = VK_NULL_HANDLE;
    }

    for (i = 0; i < pSwapChain->imageCount; ++i) {
        VkImageView attachments[1];
        VkFramebufferCreateInfo framebufferInfo;

        attachments[0] = pSwapChain->pImageViewHandles[i];

        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.pNext = NULL;
        framebufferInfo.flags = 0;
        framebufferInfo.renderPass = renderPassHandle;
        framebufferInfo.attachmentCount
            = (uint32_t)DKP_GET_ARRAY_SIZE(attachments);
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = pImageExtent->width;
        framebufferInfo.height = pImageExtent->height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(pDevice->logicalHandle,
                                &framebufferInfo,
                                pBackEndAllocator,
                                &(*ppFramebufferHandles)[i])
            != VK_SUCCESS) {
            DKP_LOG_TRACE(pLogger, "failed to create a frame buffer\n");
            out = DK_ERROR;
            goto frame_buffers_undo;
        }
    }

    goto exit;

frame_buffers_undo:
    for (i = 0; i < pSwapChain->imageCount; ++i) {
        if ((*ppFramebufferHandles)[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(pDevice->logicalHandle,
                                 (*ppFramebufferHandles)[i],
                                 pBackEndAllocator);
        }
    }

    DKP_FREE(pAllocator, *ppFramebufferHandles);

exit:
    return out;
}

static void
dkpDestroyFramebuffers(const struct DkpDevice *pDevice,
                       const struct DkpSwapChain *pSwapChain,
                       VkFramebuffer *pFramebufferHandles,
                       const VkAllocationCallbacks *pBackEndAllocator,
                       const struct DkAllocationCallbacks *pAllocator)
{
    uint32_t i;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pSwapChain->imageCount > 0);
    DKP_ASSERT(pFramebufferHandles != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pAllocator != NULL);

    for (i = 0; i < pSwapChain->imageCount; ++i) {
        DKP_ASSERT(pFramebufferHandles[i] != VK_NULL_HANDLE);
        vkDestroyFramebuffer(
            pDevice->logicalHandle, pFramebufferHandles[i], pBackEndAllocator);
    }

    DKP_FREE(pAllocator, pFramebufferHandles);
}

static enum DkStatus
dkpInitializeCommandPools(struct DkpCommandPools *pCommandPools,
                          const struct DkpDevice *pDevice,
                          const VkAllocationCallbacks *pBackEndAllocator,
                          const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    uint32_t j;
    VkCommandPoolCreateInfo createInfo;

    DKP_ASSERT(pCommandPools != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    DKP_ASSERT(pDevice->filteredQueueFamilyCount
               <= sizeof DKP_GET_ARRAY_SIZE(pCommandPools->handles));

    for (i = 0; i < pDevice->filteredQueueFamilyCount; ++i) {
        pCommandPools->handles[i] = VK_NULL_HANDLE;
    }

    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    for (i = 0; i < pDevice->filteredQueueFamilyCount; ++i) {
        createInfo.queueFamilyIndex = pDevice->filteredQueueFamilyIndices[i];

        if (vkCreateCommandPool(pDevice->logicalHandle,
                                &createInfo,
                                pBackEndAllocator,
                                &pCommandPools->handles[i])
            != VK_SUCCESS) {
            DKP_LOG_TRACE(pLogger, "failed to create a command pool\n");
            out = DK_ERROR;
            goto command_pools_undo;
        }

        for (j = 0; j < DKP_CONSTANT_MAX_QUEUE_FAMILIES_USED; ++j) {
            if (pDevice->queueFamilyIndices[j]
                == pDevice->filteredQueueFamilyIndices[i]) {
                pCommandPools->handleMap[j] = pCommandPools->handles[i];
            }
        }
    }

    goto exit;

command_pools_undo:
    for (i = 0; i < pDevice->filteredQueueFamilyCount; ++i) {
        if (pCommandPools->handles[i] != VK_NULL_HANDLE) {
            vkDestroyCommandPool(pDevice->logicalHandle,
                                 pCommandPools->handles[i],
                                 pBackEndAllocator);
        }
    }

exit:
    return out;
}

static void
dkpTerminateCommandPools(const struct DkpDevice *pDevice,
                         struct DkpCommandPools *pCommandPools,
                         const VkAllocationCallbacks *pBackEndAllocator)
{
    uint32_t i;

    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pCommandPools != NULL);
    DKP_ASSERT(pBackEndAllocator != NULL);

    for (i = 0; i < pDevice->filteredQueueFamilyCount; ++i) {
        DKP_ASSERT(pCommandPools->handles[i] != VK_NULL_HANDLE);
        vkDestroyCommandPool(pDevice->logicalHandle,
                             pCommandPools->handles[i],
                             pBackEndAllocator);
    }
}

static enum DkStatus
dkpCreateGraphicsCommandBuffers(VkCommandBuffer **ppCommandBufferHandles,
                                const struct DkpDevice *pDevice,
                                const struct DkpSwapChain *pSwapChain,
                                VkCommandPool commandPoolHandle,
                                const struct DkAllocationCallbacks *pAllocator,
                                const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    VkCommandBufferAllocateInfo allocateInfo;

    DKP_ASSERT(ppCommandBufferHandles != NULL);
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pSwapChain->imageCount > 0);
    DKP_ASSERT(commandPoolHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = NULL;
    allocateInfo.commandPool = commandPoolHandle;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = pSwapChain->imageCount;

    *ppCommandBufferHandles = (VkCommandBuffer *)DKP_ALLOCATE(
        pAllocator, sizeof **ppCommandBufferHandles * pSwapChain->imageCount);
    if (*ppCommandBufferHandles == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to create the graphics command buffers\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    if (vkAllocateCommandBuffers(
            pDevice->logicalHandle, &allocateInfo, *ppCommandBufferHandles)
        != VK_SUCCESS) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the graphics command buffers\n");
        out = DK_ERROR;
        goto command_buffers_undo;
    }

    goto exit;

command_buffers_undo:
    DKP_FREE(pAllocator, *ppCommandBufferHandles);

exit:
    return out;
}

static void
dkpDestroyGraphicsCommandBuffers(const struct DkpDevice *pDevice,
                                 const struct DkpSwapChain *pSwapChain,
                                 VkCommandPool commandPoolHandle,
                                 VkCommandBuffer *pCommandBufferHandles,
                                 const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pDevice != NULL);
    DKP_ASSERT(pDevice->logicalHandle != NULL);
    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(pSwapChain->imageCount > 0);
    DKP_ASSERT(commandPoolHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pCommandBufferHandles != NULL);
    DKP_ASSERT(pAllocator != NULL);

    vkFreeCommandBuffers(pDevice->logicalHandle,
                         commandPoolHandle,
                         pSwapChain->imageCount,
                         pCommandBufferHandles);
    DKP_FREE(pAllocator, pCommandBufferHandles);
}

static enum DkStatus
dkpRecordGraphicsCommandBuffers(const struct DkpSwapChain *pSwapChain,
                                VkRenderPass renderPassHandle,
                                VkPipeline pipelineHandle,
                                VkFramebuffer *pFramebufferHandles,
                                VkCommandBuffer *pCommandBufferHandles,
                                const VkExtent2D *pImageExtent,
                                const VkClearValue *pClearColor,
                                uint32_t vertexBufferCount,
                                const struct DkpBuffer *pVertexBuffers,
                                uint32_t vertexCount,
                                uint32_t instanceCount,
                                const struct DkAllocationCallbacks *pAllocator,
                                const struct DkLoggingCallbacks *pLogger)
{
    enum DkStatus out;
    uint32_t i;
    VkBuffer *pBuffers;
    VkDeviceSize *pOffsets;

    DKP_ASSERT(pSwapChain != NULL);
    DKP_ASSERT(renderPassHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pipelineHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pFramebufferHandles != NULL);
    DKP_ASSERT(pCommandBufferHandles != NULL);
    DKP_ASSERT(pImageExtent != NULL);
    DKP_ASSERT(pClearColor != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    out = DK_SUCCESS;

    if (vertexBufferCount > 0) {
        DKP_ASSERT(pVertexBuffers != NULL);

        pBuffers = (VkBuffer *)DKP_ALLOCATE(
            pAllocator, sizeof *pBuffers * vertexBufferCount);
        if (pBuffers == NULL) {
            DKP_LOG_TRACE(pLogger, "failed to create the vertex buffers\n");
            out = DK_ERROR_ALLOCATION;
            goto exit;
        }

        pOffsets = (VkDeviceSize *)DKP_ALLOCATE(
            pAllocator, sizeof *pOffsets * vertexBufferCount);
        if (pOffsets == NULL) {
            DKP_LOG_TRACE(pLogger,
                          "failed to create the vertex buffer offsets\n");
            out = DK_ERROR_ALLOCATION;
            goto buffers_cleanup;
        }

        for (i = 0; i < vertexBufferCount; ++i) {
            pBuffers[i] = pVertexBuffers[i].handle;
            pOffsets[i] = pVertexBuffers[i].offset;
        }
    } else {
        pBuffers = NULL;
        pOffsets = NULL;
    }

    for (i = 0; i < pSwapChain->imageCount; ++i) {
        VkCommandBufferBeginInfo beginInfo;
        VkRenderPassBeginInfo renderPassBeginInfo;

        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = NULL;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = NULL;

        if (vkBeginCommandBuffer(pCommandBufferHandles[i], &beginInfo)
            != VK_SUCCESS) {
            DKP_LOG_TRACE(pLogger,
                          "could not begin the command buffer recording\n");
            out = DK_ERROR;
            goto offsets_cleanup;
        }

        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = NULL;
        renderPassBeginInfo.renderPass = renderPassHandle;
        renderPassBeginInfo.framebuffer = pFramebufferHandles[i];
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent = *pImageExtent;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = pClearColor;

        vkCmdBeginRenderPass(pCommandBufferHandles[i],
                             &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(pCommandBufferHandles[i],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineHandle);
        if (vertexBufferCount > 0) {
            vkCmdBindVertexBuffers(
                pCommandBufferHandles[i], 0, 1, pBuffers, pOffsets);
        }
        vkCmdDraw(pCommandBufferHandles[i], vertexCount, instanceCount, 0, 0);
        vkCmdEndRenderPass(pCommandBufferHandles[i]);

        if (vkEndCommandBuffer(pCommandBufferHandles[i]) != VK_SUCCESS) {
            DKP_LOG_TRACE(pLogger,
                          "could not end the command buffer recording\n");
            out = DK_ERROR;
            goto offsets_cleanup;
        }
    }

buffers_cleanup:
    if (pBuffers != NULL) {
        DKP_FREE(pAllocator, pBuffers);
    }

offsets_cleanup:
    if (pOffsets != NULL) {
        DKP_FREE(pAllocator, pOffsets);
    }

exit:
    return out;
}

static enum DkStatus
dkpInitializeRendererSwapChainSystem(struct DkRenderer *pRenderer,
                                     VkSwapchainKHR oldSwapChainHandle)
{
    enum DkStatus out;

    DKP_ASSERT(pRenderer != NULL);

    out = DK_SUCCESS;

    out = dkpInitializeSwapChain(&pRenderer->swapChain,
                                 &pRenderer->device,
                                 pRenderer->surfaceHandle,
                                 &pRenderer->surfaceExtent,
                                 oldSwapChainHandle,
                                 &pRenderer->backEndAllocator,
                                 pRenderer->pAllocator,
                                 pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto exit;
    }

    out = dkpCreateRenderPass(&pRenderer->renderPassHandle,
                              &pRenderer->device,
                              &pRenderer->swapChain,
                              &pRenderer->backEndAllocator,
                              pRenderer->pAllocator,
                              pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto swap_chain_undo;
    }

    out = dkpCreatePipelineLayout(&pRenderer->pipelineLayoutHandle,
                                  &pRenderer->device,
                                  &pRenderer->backEndAllocator,
                                  pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto render_pass_undo;
    }

    out = dkpCreateGraphicsPipeline(&pRenderer->graphicsPipelineHandle,
                                    &pRenderer->device,
                                    pRenderer->pipelineLayoutHandle,
                                    pRenderer->renderPassHandle,
                                    pRenderer->shaderCount,
                                    pRenderer->pShaders,
                                    &pRenderer->swapChain.imageExtent,
                                    pRenderer->vertexBindingDescriptionCount,
                                    pRenderer->pVertexBindingDescriptions,
                                    pRenderer->vertexAttributeDescriptionCount,
                                    pRenderer->pVertexAttributeDescriptions,
                                    &pRenderer->backEndAllocator,
                                    pRenderer->pAllocator,
                                    pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto pipeline_layout_undo;
    }

    out = dkpCreateFramebuffers(&pRenderer->pFramebufferHandles,
                                &pRenderer->device,
                                &pRenderer->swapChain,
                                pRenderer->renderPassHandle,
                                &pRenderer->swapChain.imageExtent,
                                &pRenderer->backEndAllocator,
                                pRenderer->pAllocator,
                                pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto graphics_pipeline_undo;
    }

    out = dkpCreateGraphicsCommandBuffers(
        &pRenderer->pGraphicsCommandBufferHandles,
        &pRenderer->device,
        &pRenderer->swapChain,
        pRenderer->commandPools.handleMap[DKP_QUEUE_TYPE_GRAPHICS],
        pRenderer->pAllocator,
        pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto framebuffers_undo;
    }

    out = dkpRecordGraphicsCommandBuffers(
        &pRenderer->swapChain,
        pRenderer->renderPassHandle,
        pRenderer->graphicsPipelineHandle,
        pRenderer->pFramebufferHandles,
        pRenderer->pGraphicsCommandBufferHandles,
        &pRenderer->swapChain.imageExtent,
        &pRenderer->clearColor,
        pRenderer->vertexBufferCount,
        pRenderer->pVertexBuffers,
        pRenderer->vertexCount,
        pRenderer->instanceCount,
        pRenderer->pAllocator,
        pRenderer->pLogger);
    if (out != DK_SUCCESS) {
        goto graphics_command_buffers_undo;
    }

    goto exit;

graphics_command_buffers_undo:
    vkDeviceWaitIdle(pRenderer->device.logicalHandle);

    dkpDestroyGraphicsCommandBuffers(
        &pRenderer->device,
        &pRenderer->swapChain,
        pRenderer->commandPools.handleMap[DKP_QUEUE_TYPE_GRAPHICS],
        pRenderer->pGraphicsCommandBufferHandles,
        pRenderer->pAllocator);

framebuffers_undo:
    dkpDestroyFramebuffers(&pRenderer->device,
                           &pRenderer->swapChain,
                           pRenderer->pFramebufferHandles,
                           &pRenderer->backEndAllocator,
                           pRenderer->pAllocator);

graphics_pipeline_undo:
    dkpDestroyGraphicsPipeline(&pRenderer->device,
                               pRenderer->graphicsPipelineHandle,
                               &pRenderer->backEndAllocator);

pipeline_layout_undo:
    dkpDestroyPipelineLayout(&pRenderer->device,
                             pRenderer->pipelineLayoutHandle,
                             &pRenderer->backEndAllocator);

render_pass_undo:
    dkpDestroyRenderPass(&pRenderer->device,
                         pRenderer->renderPassHandle,
                         &pRenderer->backEndAllocator);

swap_chain_undo:
    dkpTerminateSwapChain(&pRenderer->device,
                          &pRenderer->swapChain,
                          DKP_OLD_SWAP_CHAIN_PRESERVATION_DISABLED,
                          &pRenderer->backEndAllocator,
                          pRenderer->pAllocator);

exit:
    return out;
}

static void
dkpTerminateRendererSwapChainSystem(
    struct DkRenderer *pRenderer,
    enum DkpOldSwapChainPreservation oldSwapChainPreservation)
{
    DKP_ASSERT(pRenderer != NULL);
    DKP_ASSERT(pRenderer->pGraphicsCommandBufferHandles != NULL);
    DKP_ASSERT(pRenderer->commandPools.handleMap[DKP_QUEUE_TYPE_GRAPHICS]
               != VK_NULL_HANDLE);
    DKP_ASSERT(pRenderer->pFramebufferHandles != NULL);
    DKP_ASSERT(pRenderer->graphicsPipelineHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pRenderer->pipelineLayoutHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pRenderer->renderPassHandle != VK_NULL_HANDLE);
    DKP_ASSERT(pRenderer->swapChain.handle != VK_NULL_HANDLE);

    vkDeviceWaitIdle(pRenderer->device.logicalHandle);

    dkpDestroyGraphicsCommandBuffers(
        &pRenderer->device,
        &pRenderer->swapChain,
        pRenderer->commandPools.handleMap[DKP_QUEUE_TYPE_GRAPHICS],
        pRenderer->pGraphicsCommandBufferHandles,
        pRenderer->pAllocator);

    dkpDestroyFramebuffers(&pRenderer->device,
                           &pRenderer->swapChain,
                           pRenderer->pFramebufferHandles,
                           &pRenderer->backEndAllocator,
                           pRenderer->pAllocator);

    dkpDestroyGraphicsPipeline(&pRenderer->device,
                               pRenderer->graphicsPipelineHandle,
                               &pRenderer->backEndAllocator);

    dkpDestroyPipelineLayout(&pRenderer->device,
                             pRenderer->pipelineLayoutHandle,
                             &pRenderer->backEndAllocator);

    dkpDestroyRenderPass(&pRenderer->device,
                         pRenderer->renderPassHandle,
                         &pRenderer->backEndAllocator);

    dkpTerminateSwapChain(&pRenderer->device,
                          &pRenderer->swapChain,
                          oldSwapChainPreservation,
                          &pRenderer->backEndAllocator,
                          pRenderer->pAllocator);
}

static enum DkStatus
dkpRecreateRendererSwapChain(struct DkRenderer *pRenderer)
{
    DKP_ASSERT(pRenderer != NULL);

    dkpTerminateRendererSwapChainSystem(
        pRenderer, DKP_OLD_SWAP_CHAIN_PRESERVATION_ENABLED);
    return dkpInitializeRendererSwapChainSystem(pRenderer,
                                                pRenderer->swapChain.handle);
}

static void
dkpValidateRendererCreateInfo(int *pValid,
                              const struct DkRendererCreateInfo *pCreateInfo,
                              const struct DkLoggingCallbacks *pLogger)
{
    uint32_t i;

    DKP_ASSERT(pValid != NULL);
    DKP_ASSERT(pCreateInfo != NULL);
    DKP_ASSERT(pLogger != NULL);

    *pValid = DKP_FALSE;

    if (pCreateInfo->shaderCount == 0) {
        DKP_LOG_TRACE(pLogger,
                      "'pCreateInfo->shaderCount' must be greater than 0\n");
        return;
    }

    for (i = 0; i < pCreateInfo->shaderCount; ++i) {
        dkpValidateShaderStage(pValid, pCreateInfo->pShaderInfos[i].stage);
        if (!(*pValid)) {
            DKP_LOG_TRACE(pLogger,
                          "invalid enum value for "
                          "'pCreateInfo->pShaderInfos[%d].stage'\n",
                          i);
            return;
        }
    }

    *pValid = DKP_TRUE;
}

static enum DkStatus
dkpCreateVertexBindingDescriptions(
    VkVertexInputBindingDescription **ppVertexBindingDescriptions,
    uint32_t vertexBindingDescriptionCount,
    const struct DkVertexBindingDescriptionCreateInfo *pCreateInfos,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    uint32_t i;

    DKP_ASSERT(ppVertexBindingDescriptions != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (vertexBindingDescriptionCount == 0 || pCreateInfos == NULL) {
        *ppVertexBindingDescriptions = NULL;
        return DK_SUCCESS;
    }

    *ppVertexBindingDescriptions
        = (VkVertexInputBindingDescription *)DKP_ALLOCATE(
            pAllocator,
            sizeof **ppVertexBindingDescriptions
                * vertexBindingDescriptionCount);
    if (*ppVertexBindingDescriptions == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the vertex binding descriptions\n");
        return DK_ERROR_ALLOCATION;
    }

    for (i = 0; i < vertexBindingDescriptionCount; ++i) {
        VkVertexInputRate backEndInputRate;

        dkpTranslateVertexInputRateToBackEnd(&backEndInputRate,
                                             pCreateInfos[i].inputRate);

        (*ppVertexBindingDescriptions)[i].binding = i;
        (*ppVertexBindingDescriptions)[i].stride
            = (uint32_t)pCreateInfos[i].stride;
        (*ppVertexBindingDescriptions)[i].inputRate = backEndInputRate;
    }

    return DK_SUCCESS;
}

static void
dkpDestroyVertexBindingDescriptions(
    VkVertexInputBindingDescription *pVertexBindingDescriptions,
    const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pAllocator != NULL);

    if (pVertexBindingDescriptions != NULL) {
        DKP_FREE(pAllocator, pVertexBindingDescriptions);
    }
}

static enum DkStatus
dkpCreateVertexAttributeDescriptions(
    VkVertexInputAttributeDescription **ppVertexAttributeDescriptions,
    uint32_t vertexAttributeDescriptionCount,
    const struct DkVertexAttributeDescriptionCreateInfo *pCreateInfos,
    const struct DkAllocationCallbacks *pAllocator,
    const struct DkLoggingCallbacks *pLogger)
{
    uint32_t i;

    DKP_ASSERT(ppVertexAttributeDescriptions != NULL);
    DKP_ASSERT(pAllocator != NULL);
    DKP_ASSERT(pLogger != NULL);

    if (vertexAttributeDescriptionCount == 0 || pCreateInfos == NULL) {
        *ppVertexAttributeDescriptions = NULL;
        return DK_SUCCESS;
    }

    *ppVertexAttributeDescriptions
        = (VkVertexInputAttributeDescription *)DKP_ALLOCATE(
            pAllocator,
            sizeof **ppVertexAttributeDescriptions
                * vertexAttributeDescriptionCount);
    if (*ppVertexAttributeDescriptions == NULL) {
        DKP_LOG_TRACE(pLogger,
                      "failed to allocate the vertex attribute descriptions\n");
        return DK_ERROR_ALLOCATION;
    }

    for (i = 0; i < vertexAttributeDescriptionCount; ++i) {
        VkFormat backEndFormat;

        dkpTranslateFormatToBackEnd(&backEndFormat, pCreateInfos[i].format);

        (*ppVertexAttributeDescriptions)[i].location
            = (uint32_t)pCreateInfos[i].location;
        (*ppVertexAttributeDescriptions)[i].binding
            = (uint32_t)pCreateInfos[i].binding;
        (*ppVertexAttributeDescriptions)[i].offset
            = (uint32_t)pCreateInfos[i].offset;
        (*ppVertexAttributeDescriptions)[i].format = backEndFormat;
    }

    return DK_SUCCESS;
}

static void
dkpDestroyVertexAttributeDescriptions(
    VkVertexInputAttributeDescription *pVertexAttributeDescriptions,
    const struct DkAllocationCallbacks *pAllocator)
{
    DKP_ASSERT(pAllocator != NULL);

    if (pVertexAttributeDescriptions != NULL) {
        DKP_FREE(pAllocator, pVertexAttributeDescriptions);
    }
}

static void *
dkpAllocateBackEndMemory(void *pData,
                         size_t size,
                         size_t alignment,
                         VkSystemAllocationScope allocationScope)
{
    DKP_UNUSED(allocationScope);

    DKP_ASSERT(pData != NULL);
    DKP_ASSERT(size != 0);

    return ((struct DkpBackEndAllocationCallbacksData *)pData)
        ->pAllocator->pfnAllocateAligned(
            ((struct DkpBackEndAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            size,
            alignment);
}

static void
dkpFreeBackEndMemory(void *pData, void *pMemory)
{
    DKP_ASSERT(pData != NULL);

    if (pMemory == NULL) {
        return;
    }

    ((struct DkpBackEndAllocationCallbacksData *)pData)
        ->pAllocator->pfnFreeAligned(
            ((struct DkpBackEndAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            pMemory);
}

static void *
dkpReallocateBackEndMemory(void *pData,
                           void *pOriginal,
                           size_t size,
                           size_t alignment,
                           VkSystemAllocationScope allocationScope)
{
    DKP_UNUSED(allocationScope);

    DKP_ASSERT(pData != NULL);

    if (pOriginal == NULL) {
        return dkpAllocateBackEndMemory(
            pData, size, alignment, allocationScope);
    }

    if (size == 0) {
        dkpFreeBackEndMemory(pData, pOriginal);
        return NULL;
    }

    return ((struct DkpBackEndAllocationCallbacksData *)pData)
        ->pAllocator->pfnReallocateAligned(
            ((struct DkpBackEndAllocationCallbacksData *)pData)
                ->pAllocator->pData,
            pOriginal,
            size,
            alignment);
}

static void
dkpNotifyBackEndInternalAllocation(void *pData,
                                   size_t size,
                                   VkInternalAllocationType allocationType,
                                   VkSystemAllocationScope allocationScope)
{
    DKP_UNUSED(allocationType);
    DKP_UNUSED(allocationScope);

    DKP_ASSERT(pData != NULL);

    DKP_LOG_TRACE(((struct DkpBackEndAllocationCallbacksData *)pData)->pLogger,
                  "renderer back-end internal allocation of %zu bytes\n",
                  size);
}

static void
dkpNotifyBackEndInternalFreeing(void *pData,
                                size_t size,
                                VkInternalAllocationType allocationType,
                                VkSystemAllocationScope allocationScope)
{
    DKP_UNUSED(allocationType);
    DKP_UNUSED(allocationScope);

    DKP_ASSERT(pData != NULL);

    DKP_LOG_TRACE(((struct DkpBackEndAllocationCallbacksData *)pData)->pLogger,
                  "renderer back-end internal freeing of %zu bytes\n",
                  size);
}

enum DkStatus
dkCreateRenderer(struct DkRenderer **ppRenderer,
                 const struct DkRendererCreateInfo *pCreateInfo)
{
    enum DkStatus out;
    uint32_t i;
    const struct DkLoggingCallbacks *pLogger;
    const struct DkAllocationCallbacks *pAllocator;
    int valid;
    int headless;

    out = DK_SUCCESS;

    if (pCreateInfo == NULL || pCreateInfo->pLogger == NULL) {
        dkpGetDefaultLogger(&pLogger);
    } else {
        pLogger = pCreateInfo->pLogger;
    }

    if (ppRenderer == NULL) {
        DKP_LOG_ERROR(pLogger, "invalid argument 'ppRenderer' (NULL)\n");
        out = DK_ERROR_INVALID_VALUE;
        goto exit;
    }

    if (pCreateInfo == NULL) {
        DKP_LOG_ERROR(pLogger, "invalid argument 'pCreateInfo' (NULL)\n");
        out = DK_ERROR_INVALID_VALUE;
        goto exit;
    }

    dkpValidateRendererCreateInfo(&valid, pCreateInfo, pLogger);
    if (!valid) {
        out = DK_ERROR_INVALID_VALUE;
        goto exit;
    }

    if (pCreateInfo->pAllocator == NULL) {
        dkpGetDefaultAllocator(&pAllocator);
    } else {
        pAllocator = pCreateInfo->pAllocator;
    }

    headless = pCreateInfo->pWindowSystemIntegrator == NULL;

    *ppRenderer
        = (struct DkRenderer *)DKP_ALLOCATE(pAllocator, sizeof **ppRenderer);
    if (*ppRenderer == NULL) {
        DKP_LOG_ERROR(pLogger, "failed to allocate the renderer\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    (*ppRenderer)->pLogger = pLogger;
    (*ppRenderer)->pAllocator = pAllocator;
    (*ppRenderer)->backEndAllocatorData.pAllocator = pAllocator;
    (*ppRenderer)->backEndAllocatorData.pLogger = pLogger;
    (*ppRenderer)->backEndAllocator.pUserData
        = &(*ppRenderer)->backEndAllocatorData;
    (*ppRenderer)->backEndAllocator.pfnAllocation = dkpAllocateBackEndMemory;
    (*ppRenderer)->backEndAllocator.pfnReallocation
        = dkpReallocateBackEndMemory;
    (*ppRenderer)->backEndAllocator.pfnFree = dkpFreeBackEndMemory;
    (*ppRenderer)->backEndAllocator.pfnInternalAllocation
        = dkpNotifyBackEndInternalAllocation;
    (*ppRenderer)->backEndAllocator.pfnInternalFree
        = dkpNotifyBackEndInternalFreeing;

    (*ppRenderer)->surfaceExtent.width = (uint32_t)pCreateInfo->surfaceWidth;
    (*ppRenderer)->surfaceExtent.height = (uint32_t)pCreateInfo->surfaceHeight;
    (*ppRenderer)->vertexCount = (uint32_t)pCreateInfo->vertexCount;
    (*ppRenderer)->instanceCount = (uint32_t)pCreateInfo->instanceCount;

    for (i = 0; i < 4; ++i) {
        (*ppRenderer)->clearColor.color.float32[i]
            = (float)pCreateInfo->clearColor[i];
    }

    (*ppRenderer)->vertexBindingDescriptionCount
        = (uint32_t)pCreateInfo->vertexBindingDescriptionCount;

    out = dkpCreateVertexBindingDescriptions(
        &(*ppRenderer)->pVertexBindingDescriptions,
        (*ppRenderer)->vertexBindingDescriptionCount,
        pCreateInfo->pVertexBindingDescriptionInfos,
        (*ppRenderer)->pAllocator,
        (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto renderer_undo;
    }

    (*ppRenderer)->vertexAttributeDescriptionCount
        = (uint32_t)pCreateInfo->vertexAttributeDescriptionCount;

    out = dkpCreateVertexAttributeDescriptions(
        &(*ppRenderer)->pVertexAttributeDescriptions,
        (*ppRenderer)->vertexAttributeDescriptionCount,
        pCreateInfo->pVertexAttributeDescriptionInfos,
        (*ppRenderer)->pAllocator,
        (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto vertex_binding_descriptions_undo;
    }

    out = dkpCreateInstance(&(*ppRenderer)->instanceHandle,
                            pCreateInfo->pApplicationName,
                            (unsigned int)pCreateInfo->applicationMajorVersion,
                            (unsigned int)pCreateInfo->applicationMinorVersion,
                            (unsigned int)pCreateInfo->applicationPatchVersion,
                            pCreateInfo->pWindowSystemIntegrator,
                            &(*ppRenderer)->backEndAllocator,
                            (*ppRenderer)->pAllocator,
                            (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto vertex_attribute_descriptions_undo;
    }

#if DKP_RENDERER_DEBUG_REPORT
    (*ppRenderer)->debugReportCallbackData.pLogger = (*ppRenderer)->pLogger;

    out = dkpCreateDebugReportCallback(
        &(*ppRenderer)->debugReportCallbackHandle,
        (*ppRenderer)->instanceHandle,
        &(*ppRenderer)->debugReportCallbackData,
        &(*ppRenderer)->backEndAllocator,
        (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto instance_undo;
    }
#endif /* DKP_RENDERER_DEBUG_REPORT */

    if (!headless) {
        out = dkpCreateSurface(&(*ppRenderer)->surfaceHandle,
                               (*ppRenderer)->instanceHandle,
                               pCreateInfo->pWindowSystemIntegrator,
                               &(*ppRenderer)->backEndAllocator,
                               (*ppRenderer)->pLogger);
        if (out != DK_SUCCESS) {
            goto debug_report_callback_undo;
        }
    } else {
        (*ppRenderer)->surfaceHandle = VK_NULL_HANDLE;
    }

    out = dkpInitializeDevice(&(*ppRenderer)->device,
                              (*ppRenderer)->instanceHandle,
                              (*ppRenderer)->surfaceHandle,
                              &(*ppRenderer)->backEndAllocator,
                              (*ppRenderer)->pAllocator,
                              (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto surface_undo;
    }

    out = dkpGetDeviceQueues(&(*ppRenderer)->queues, &(*ppRenderer)->device);
    if (out != DK_SUCCESS) {
        goto device_undo;
    }

    out = dkpCreateSemaphores(&(*ppRenderer)->pSemaphoreHandles,
                              &(*ppRenderer)->device,
                              &(*ppRenderer)->backEndAllocator,
                              (*ppRenderer)->pAllocator,
                              (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto device_undo;
    }

    (*ppRenderer)->shaderCount = (uint32_t)pCreateInfo->shaderCount;

    out = dkpCreateShaders(&(*ppRenderer)->pShaders,
                           &(*ppRenderer)->device,
                           (*ppRenderer)->shaderCount,
                           pCreateInfo->pShaderInfos,
                           &(*ppRenderer)->backEndAllocator,
                           (*ppRenderer)->pAllocator,
                           (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto semaphores_undo;
    }

    out = dkpInitializeCommandPools(&(*ppRenderer)->commandPools,
                                    &(*ppRenderer)->device,
                                    &(*ppRenderer)->backEndAllocator,
                                    (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto shaders_undo;
    }

    (*ppRenderer)->vertexBufferCount = (uint32_t)pCreateInfo->vertexBufferCount;

    out = dkpCreateVertexBuffers(
        &(*ppRenderer)->pVertexBuffers,
        &(*ppRenderer)->device,
        (*ppRenderer)->vertexBufferCount,
        pCreateInfo->pVertexBufferInfos,
        (*ppRenderer)->commandPools.handleMap[DKP_QUEUE_TYPE_TRANSFER],
        &(*ppRenderer)->queues,
        &(*ppRenderer)->backEndAllocator,
        (*ppRenderer)->pAllocator,
        (*ppRenderer)->pLogger);
    if (out != DK_SUCCESS) {
        goto command_pools_undo;
    }

    if (!headless) {
        out = dkpInitializeRendererSwapChainSystem(*ppRenderer, VK_NULL_HANDLE);
        if (out != DK_SUCCESS) {
            goto vertex_buffers_undo;
        }
    }

    goto exit;

vertex_buffers_undo:
    dkpDestroyVertexBuffers(&(*ppRenderer)->device,
                            (*ppRenderer)->vertexBufferCount,
                            (*ppRenderer)->pVertexBuffers,
                            &(*ppRenderer)->backEndAllocator,
                            (*ppRenderer)->pAllocator);

command_pools_undo:
    dkpTerminateCommandPools(&(*ppRenderer)->device,
                             &(*ppRenderer)->commandPools,
                             &(*ppRenderer)->backEndAllocator);

shaders_undo:
    dkpDestroyShaders(&(*ppRenderer)->device,
                      (*ppRenderer)->shaderCount,
                      (*ppRenderer)->pShaders,
                      &(*ppRenderer)->backEndAllocator,
                      (*ppRenderer)->pAllocator);

semaphores_undo:
    dkpDestroySemaphores(&(*ppRenderer)->device,
                         (*ppRenderer)->pSemaphoreHandles,
                         &(*ppRenderer)->backEndAllocator,
                         (*ppRenderer)->pAllocator);

device_undo:
    dkpTerminateDevice(&(*ppRenderer)->device,
                       &(*ppRenderer)->backEndAllocator);

surface_undo:
    if (!headless) {
        dkpDestroySurface((*ppRenderer)->instanceHandle,
                          (*ppRenderer)->surfaceHandle,
                          &(*ppRenderer)->backEndAllocator);
    }

debug_report_callback_undo:
#if DKP_RENDERER_DEBUG_REPORT
    dkpDestroyDebugReportCallback((*ppRenderer)->instanceHandle,
                                  (*ppRenderer)->debugReportCallbackHandle,
                                  &(*ppRenderer)->backEndAllocator,
                                  (*ppRenderer)->pLogger);
#else
    goto instance_undo;
#endif /* DKP_RENDERER_DEBUG_REPORT */

instance_undo:
    dkpDestroyInstance((*ppRenderer)->instanceHandle,
                       &(*ppRenderer)->backEndAllocator);

vertex_attribute_descriptions_undo:
    dkpDestroyVertexAttributeDescriptions(
        (*ppRenderer)->pVertexAttributeDescriptions, (*ppRenderer)->pAllocator);

vertex_binding_descriptions_undo:
    dkpDestroyVertexBindingDescriptions(
        (*ppRenderer)->pVertexBindingDescriptions, (*ppRenderer)->pAllocator);

renderer_undo:
    DKP_FREE(pAllocator, (*ppRenderer));
    *ppRenderer = NULL;

exit:
    return out;
}

void
dkDestroyRenderer(struct DkRenderer *pRenderer)
{
    int headless;

    if (pRenderer == NULL) {
        return;
    }

    headless = pRenderer->surfaceHandle == VK_NULL_HANDLE;

    vkDeviceWaitIdle(pRenderer->device.logicalHandle);

    if (!headless) {
        dkpTerminateRendererSwapChainSystem(
            pRenderer, DKP_OLD_SWAP_CHAIN_PRESERVATION_DISABLED);
    }

    dkpDestroyVertexBuffers(&pRenderer->device,
                            pRenderer->vertexBufferCount,
                            pRenderer->pVertexBuffers,
                            &pRenderer->backEndAllocator,
                            pRenderer->pAllocator);
    dkpTerminateCommandPools(&pRenderer->device,
                             &pRenderer->commandPools,
                             &pRenderer->backEndAllocator);
    dkpDestroyShaders(&pRenderer->device,
                      pRenderer->shaderCount,
                      pRenderer->pShaders,
                      &pRenderer->backEndAllocator,
                      pRenderer->pAllocator);
    dkpDestroySemaphores(&pRenderer->device,
                         pRenderer->pSemaphoreHandles,
                         &pRenderer->backEndAllocator,
                         pRenderer->pAllocator);
    dkpTerminateDevice(&pRenderer->device, &pRenderer->backEndAllocator);

    if (!headless) {
        dkpDestroySurface(pRenderer->instanceHandle,
                          pRenderer->surfaceHandle,
                          &pRenderer->backEndAllocator);
    }

#if DKP_RENDERER_DEBUG_REPORT
    dkpDestroyDebugReportCallback(pRenderer->instanceHandle,
                                  pRenderer->debugReportCallbackHandle,
                                  &pRenderer->backEndAllocator,
                                  pRenderer->pLogger);
#endif /* DKP_RENDERER_DEBUG_REPORT */

    dkpDestroyInstance(pRenderer->instanceHandle, &pRenderer->backEndAllocator);
    dkpDestroyVertexAttributeDescriptions(
        pRenderer->pVertexAttributeDescriptions, pRenderer->pAllocator);
    dkpDestroyVertexBindingDescriptions(pRenderer->pVertexBindingDescriptions,
                                        pRenderer->pAllocator);
    DKP_FREE(pRenderer->pAllocator, pRenderer);
}

enum DkStatus
dkResizeRendererSurface(struct DkRenderer *pRenderer,
                        DkUint32 width,
                        DkUint32 height)
{
    DKP_ASSERT(pRenderer != NULL);

    pRenderer->surfaceExtent.width = (uint32_t)width;
    pRenderer->surfaceExtent.height = (uint32_t)height;
    return dkpRecreateRendererSwapChain(pRenderer);
}

enum DkStatus
dkDrawRendererImage(struct DkRenderer *pRenderer)
{
    enum DkStatus out;
    uint32_t imageIndex;
    uint32_t waitSemaphoreCount;
    VkSemaphore *pWaitSemaphores;
    uint32_t signalSemaphoreCount;
    VkSemaphore *pSignalSemaphores;
    VkPipelineStageFlags waitDstStageMask;
    VkSubmitInfo submitInfo;
    VkPresentInfoKHR presentInfo;
    VkSwapchainKHR swapChainHandles[1];
    uint32_t imageIndices[1];

    DKP_ASSERT(pRenderer != NULL);

    out = DK_SUCCESS;

    vkQueueWaitIdle(pRenderer->queues.presentHandle);

    switch (vkAcquireNextImageKHR(
        pRenderer->device.logicalHandle,
        pRenderer->swapChain.handle,
        (uint64_t)-1,
        pRenderer->pSemaphoreHandles[DKP_SEMAPHORE_ID_IMAGE_ACQUIRED],
        VK_NULL_HANDLE,
        &imageIndex)) {
        case VK_SUCCESS:
            break;
        case VK_NOT_READY:
            DKP_LOG_ERROR(pRenderer->pLogger,
                          "could not find a suitable image\n");
            out = DK_ERROR_NOT_AVAILABLE;
            goto exit;
        case VK_TIMEOUT:
            DKP_LOG_ERROR(
                pRenderer->pLogger,
                "could not find a suitable image within the time allowed\n");
            out = DK_ERROR_NOT_AVAILABLE;
            goto exit;
        case VK_SUBOPTIMAL_KHR:
            if (dkpRecreateRendererSwapChain(pRenderer) != DK_SUCCESS) {
                out = DK_ERROR;
                goto exit;
            }

            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            if (dkpRecreateRendererSwapChain(pRenderer) != DK_SUCCESS) {
                out = DK_ERROR;
                goto exit;
            }

            break;
        case VK_ERROR_DEVICE_LOST:
            DKP_LOG_ERROR(pRenderer->pLogger,
                          "the swap chain's device has been lost\n");
            out = DK_ERROR;
            goto exit;
        case VK_ERROR_SURFACE_LOST_KHR:
            DKP_LOG_ERROR(pRenderer->pLogger,
                          "the swap chain's surface has been lost\n");
            out = DK_ERROR;
            goto exit;
        default:
            DKP_LOG_ERROR(pRenderer->pLogger,
                          "could not acquire a new image\n");
            out = DK_ERROR;
            goto exit;
    }

    waitSemaphoreCount = 1;
    pWaitSemaphores = (VkSemaphore *)DKP_ALLOCATE(
        pRenderer->pAllocator, sizeof *pWaitSemaphores * waitSemaphoreCount);
    if (pWaitSemaphores == NULL) {
        DKP_LOG_ERROR(pRenderer->pLogger,
                      "failed to allocate the wait semaphores\n");
        out = DK_ERROR_ALLOCATION;
        goto exit;
    }

    pWaitSemaphores[0]
        = pRenderer->pSemaphoreHandles[DKP_SEMAPHORE_ID_IMAGE_ACQUIRED];

    signalSemaphoreCount = 1;
    pSignalSemaphores = (VkSemaphore *)DKP_ALLOCATE(
        pRenderer->pAllocator,
        sizeof *pSignalSemaphores * signalSemaphoreCount);
    if (pSignalSemaphores == NULL) {
        DKP_LOG_ERROR(pRenderer->pLogger,
                      "failed to allocate the signal semaphores\n");
        out = DK_ERROR_ALLOCATION;
        goto wait_semaphores_cleanup;
    }

    pSignalSemaphores[0]
        = pRenderer->pSemaphoreHandles[DKP_SEMAPHORE_ID_PRESENT_COMPLETED];

    waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = waitSemaphoreCount;
    submitInfo.pWaitSemaphores = pWaitSemaphores;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers
        = &pRenderer->pGraphicsCommandBufferHandles[imageIndex];
    submitInfo.signalSemaphoreCount = signalSemaphoreCount;
    submitInfo.pSignalSemaphores = pSignalSemaphores;

    if (vkQueueSubmit(
            pRenderer->queues.graphicsHandle, 1, &submitInfo, VK_NULL_HANDLE)
        != VK_SUCCESS) {
        DKP_LOG_ERROR(pRenderer->pLogger,
                      "could not submit the graphics command buffer\n");
        out = DK_ERROR;
        goto signal_semaphores_cleanup;
    }

    swapChainHandles[0] = pRenderer->swapChain.handle;
    imageIndices[0] = imageIndex;

    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.waitSemaphoreCount = signalSemaphoreCount;
    presentInfo.pWaitSemaphores = pSignalSemaphores;
    presentInfo.swapchainCount = DKP_GET_ARRAY_SIZE(swapChainHandles);
    presentInfo.pSwapchains = swapChainHandles;
    presentInfo.pImageIndices = imageIndices;
    presentInfo.pResults = NULL;

    vkQueuePresentKHR(pRenderer->queues.presentHandle, &presentInfo);

signal_semaphores_cleanup:
    DKP_FREE(pRenderer->pAllocator, pSignalSemaphores);

wait_semaphores_cleanup:
    DKP_FREE(pRenderer->pAllocator, pWaitSemaphores);

exit:
    return out;
}
