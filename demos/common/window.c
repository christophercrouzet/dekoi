#define GLFW_INCLUDE_VULKAN

#include "window.h"

#include "application.h"
#include "allocator.h"
#include "common.h"
#include "logger.h"
#include "renderer.h"

#include <GLFW/glfw3.h>
#include <dekoi/common/common.h>
#include <dekoi/graphics/renderer.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

struct DkdWindowSystemIntegrationCallbacksData {
    GLFWwindow *pWindowHandle;
};

struct DkdWindow {
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;
    GLFWwindow *pHandle;
    struct DkWindowSystemIntegrationCallbacks windowSystemIntegrator;
    struct DkdRenderer *pRenderer;
};

static void
dkdOnFramebufferSizeChanged(GLFWwindow *pWindowHandle, int width, int height)
{
    struct DkdWindow *pWindow;

    assert(pWindowHandle != NULL);

    pWindow = (struct DkdWindow *)glfwGetWindowUserPointer(pWindowHandle);
    dkdResizeRendererSurface(
        pWindow->pRenderer, (unsigned int)width, (unsigned int)height);
}

static enum DkResult
dkdCreateVulkanInstanceExtensionNames(
    DkUint32 *pExtensionCount,
    const char ***pppExtensionNames,
    void *pData,
    const struct DkLoggingCallbacks *pDekoiLogger)
{
    DKD_UNUSED(pData);

    assert(pExtensionCount != NULL);
    assert(pppExtensionNames != NULL);
    assert(pDekoiLogger != NULL);

    *pppExtensionNames
        = glfwGetRequiredInstanceExtensions((uint32_t *)pExtensionCount);
    if (*pppExtensionNames == NULL) {
        DKD_LOG_ERROR(
            ((struct DkdDekoiLoggingCallbacksData *)pDekoiLogger->pData)
                ->pLogger,
            "could not retrieve the Vulkan instance extension names\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
dkdDestroyVulkanInstanceExtensionNames(
    void *pData,
    const struct DkLoggingCallbacks *pDekoiLogger,
    const char **ppExtensionNames)
{
    DKD_UNUSED(pData);
    DKD_UNUSED(pDekoiLogger);
    DKD_UNUSED(ppExtensionNames);

    assert(pDekoiLogger != NULL);
    assert(ppExtensionNames != NULL);
}

static enum DkResult
dkdCreateVulkanSurface(VkSurfaceKHR *pSurfaceHandle,
                       void *pData,
                       VkInstance instanceHandle,
                       const VkAllocationCallbacks *pBackEndAllocator,
                       const struct DkLoggingCallbacks *pDekoiLogger)
{
    assert(pSurfaceHandle != NULL);
    assert(pData != NULL);
    assert(instanceHandle != NULL);
    assert(pDekoiLogger != NULL);

    if (glfwCreateWindowSurface(
            instanceHandle,
            ((struct DkdWindowSystemIntegrationCallbacksData *)pData)
                ->pWindowHandle,
            pBackEndAllocator,
            pSurfaceHandle)
        != VK_SUCCESS) {
        DKD_LOG_ERROR(
            ((struct DkdDekoiLoggingCallbacksData *)pDekoiLogger->pData)
                ->pLogger,
            "failed to create the Vulkan surface\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

int
dkdCreateWindow(struct DkdWindow **ppWindow,
                struct DkdApplication *pApplication,
                const struct DkdWindowCreateInfo *pCreateInfo)
{
    int out;
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;
    struct DkdWindowSystemIntegrationCallbacksData *pWindowSystemIntegratorData;

    assert(ppWindow != NULL);
    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);

    if (pCreateInfo->pLogger == NULL) {
        dkdGetDefaultLogger(&pLogger);
    } else {
        pLogger = pCreateInfo->pLogger;
    }

    if (pCreateInfo->pAllocator == NULL) {
        dkdGetDefaultAllocator(&pAllocator);
    } else {
        pAllocator = pCreateInfo->pAllocator;
    }

    out = 0;

    *ppWindow = (struct DkdWindow *)DKD_ALLOCATE(pAllocator, sizeof **ppWindow);
    if (*ppWindow == NULL) {
        DKD_LOG_ERROR(pLogger, "failed to allocate the window\n");
        out = 1;
        goto exit;
    }

    if (glfwInit() != GLFW_TRUE) {
        DKD_LOG_ERROR(pLogger, "failed to initialize GLFW\n");
        out = 1;
        goto window_undo;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    (*ppWindow)->pLogger = pLogger;
    (*ppWindow)->pAllocator = pAllocator;
    (*ppWindow)->pHandle = glfwCreateWindow((int)pCreateInfo->width,
                                            (int)pCreateInfo->height,
                                            pCreateInfo->pTitle,
                                            NULL,
                                            NULL);
    if ((*ppWindow)->pHandle == NULL) {
        DKD_LOG_ERROR(pLogger, "failed to create the window\n");
        out = 1;
        goto glfw_undo;
    }

    pWindowSystemIntegratorData
        = (struct DkdWindowSystemIntegrationCallbacksData *)DKD_ALLOCATE(
            (*ppWindow)->pAllocator, sizeof *pWindowSystemIntegratorData);
    if (pWindowSystemIntegratorData == NULL) {
        DKD_LOG_ERROR(
            pLogger,
            "failed to allocate the window system integrator callbacks data\n");
        out = 1;
        goto glfw_window_undo;
    }

    pWindowSystemIntegratorData->pWindowHandle = (*ppWindow)->pHandle;

    (*ppWindow)->windowSystemIntegrator.pData
        = (void *)pWindowSystemIntegratorData;
    (*ppWindow)->windowSystemIntegrator.pfnCreateInstanceExtensionNames
        = dkdCreateVulkanInstanceExtensionNames;
    (*ppWindow)->windowSystemIntegrator.pfnDestroyInstanceExtensionNames
        = dkdDestroyVulkanInstanceExtensionNames;
    (*ppWindow)->windowSystemIntegrator.pfnCreateSurface
        = dkdCreateVulkanSurface;

    glfwSetWindowUserPointer((*ppWindow)->pHandle, *ppWindow);

    glfwSetFramebufferSizeCallback((*ppWindow)->pHandle,
                                   dkdOnFramebufferSizeChanged);

    if (dkdBindApplicationWindow(pApplication, *ppWindow)) {
        out = 1;
        goto window_renderer_callbacks_data_undo;
    }

    (*ppWindow)->pRenderer = NULL;
    goto exit;

window_renderer_callbacks_data_undo:
    DKD_FREE((*ppWindow)->pAllocator,
             (*ppWindow)->windowSystemIntegrator.pData);

glfw_window_undo:
    glfwDestroyWindow((*ppWindow)->pHandle);

glfw_undo:
    glfwTerminate();

window_undo:
    DKD_FREE(pAllocator, *ppWindow);

exit:
    return out;
}

void
dkdDestroyWindow(struct DkdApplication *pApplication, struct DkdWindow *pWindow)
{
    assert(pApplication != NULL);

    dkdBindApplicationWindow(pApplication, NULL);

    if (pWindow == NULL) {
        return;
    }

    assert(pWindow->pHandle != NULL);

    DKD_FREE(pWindow->pAllocator, pWindow->windowSystemIntegrator.pData);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
    DKD_FREE(pWindow->pAllocator, pWindow);
}

void
dkdGetDekoiWindowSystemIntegrator(
    const struct DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator,
    struct DkdWindow *pWindow)
{
    assert(ppWindowSystemIntegrator != NULL);
    assert(pWindow != NULL);

    *ppWindowSystemIntegrator = &pWindow->windowSystemIntegrator;
}

int
dkdBindWindowRenderer(struct DkdWindow *pWindow, struct DkdRenderer *pRenderer)
{
    assert(pWindow != NULL);

    pWindow->pRenderer = pRenderer;
    return 0;
}

void
dkdGetWindowCloseFlag(int *pCloseFlag, const struct DkdWindow *pWindow)
{
    assert(pCloseFlag != NULL);
    assert(pWindow != NULL);
    assert(pWindow->pHandle != NULL);

    *pCloseFlag = glfwWindowShouldClose(pWindow->pHandle);
}

int
dkdPollWindowEvents(const struct DkdWindow *pWindow)
{
    DKD_UNUSED(pWindow);

    assert(pWindow != NULL);

    glfwPollEvents();
    return 0;
}

int
dkdRenderWindowImage(const struct DkdWindow *pWindow)
{
    assert(pWindow != NULL);

    if (dkdDrawRendererImage(pWindow->pRenderer)) {
        return 1;
    }

    return 0;
}
