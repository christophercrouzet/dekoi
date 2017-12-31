#define GLFW_INCLUDE_VULKAN

#include "window.h"

#include "application.h"
#include "common.h"
#include "logger.h"
#include "renderer.h"

#include <GLFW/glfw3.h>
#include <dekoi/dekoi>
#include <dekoi/rendering>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct DkdWindowSystemIntegrationCallbacksData {
    GLFWwindow *pWindowHandle;
} DkdWindowSystemIntegrationCallbacksData;

struct DkdWindow {
    GLFWwindow *pHandle;
    DkWindowSystemIntegrationCallbacks windowSystemIntegrator;
    DkdRenderer *pRenderer;
};

static void
dkdOnFramebufferSizeChanged(GLFWwindow *pWindowHandle, int width, int height)
{
    DkdWindow *pWindow;

    assert(pWindowHandle != NULL);

    pWindow = (DkdWindow *)glfwGetWindowUserPointer(pWindowHandle);
    dkdResizeRendererSurface(
        pWindow->pRenderer, (unsigned int)width, (unsigned int)height);
}

static DkResult
dkdCreateVulkanInstanceExtensionNames(void *pData,
                                      const DkLoggingCallbacks *pDekoiLogger,
                                      DkUint32 *pExtensionCount,
                                      const char ***pppExtensionNames)
{
    DKD_UNUSED(pData);

    assert(pExtensionCount != NULL);
    assert(pppExtensionNames != NULL);

    *pppExtensionNames
        = glfwGetRequiredInstanceExtensions((uint32_t *)pExtensionCount);
    if (*pppExtensionNames == NULL) {
        DKD_LOG_ERROR(
            ((DkdDekoiLoggingCallbacksData *)pDekoiLogger->pData)->pLogger,
            "could not retrieve the Vulkan instance extension names\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
dkdDestroyVulkanInstanceExtensionNames(void *pData,
                                       const DkLoggingCallbacks *pDekoiLogger,
                                       const char **ppExtensionNames)
{
    DKD_UNUSED(pData);
    DKD_UNUSED(pDekoiLogger);
    DKD_UNUSED(ppExtensionNames);

    assert(ppExtensionNames != NULL);
}

static DkResult
dkdCreateVulkanSurface(void *pData,
                       VkInstance instanceHandle,
                       const VkAllocationCallbacks *pBackEndAllocator,
                       const DkLoggingCallbacks *pDekoiLogger,
                       VkSurfaceKHR *pSurfaceHandle)
{
    assert(pData != NULL);
    assert(instanceHandle != NULL);
    assert(pSurfaceHandle != NULL);

    if (glfwCreateWindowSurface(
            instanceHandle,
            ((DkdWindowSystemIntegrationCallbacksData *)pData)->pWindowHandle,
            pBackEndAllocator,
            pSurfaceHandle)
        != VK_SUCCESS) {
        DKD_LOG_ERROR(
            ((DkdDekoiLoggingCallbacksData *)pDekoiLogger->pData)->pLogger,
            "failed to create the Vulkan surface\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

int
dkdCreateWindow(DkdApplication *pApplication,
                const DkdWindowCreateInfo *pCreateInfo,
                DkdWindow **ppWindow)
{
    int out;
    const DkdLoggingCallbacks *pLogger;
    DkdWindowSystemIntegrationCallbacksData *pWindowSystemIntegratorData;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(ppWindow != NULL);

    *ppWindow = NULL;

    if (pCreateInfo->pLogger == NULL) {
        dkdGetDefaultLogger(&pLogger);
    } else {
        pLogger = pCreateInfo->pLogger;
    }

    out = 0;

    *ppWindow = (DkdWindow *)malloc(sizeof **ppWindow);
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
        = (DkdWindowSystemIntegrationCallbacksData *)malloc(
            sizeof *pWindowSystemIntegratorData);
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
    free((*ppWindow)->windowSystemIntegrator.pData);

glfw_window_undo:
    glfwDestroyWindow((*ppWindow)->pHandle);

glfw_undo:
    glfwTerminate();

window_undo:
    free(*ppWindow);

exit:
    return out;
}

void
dkdDestroyWindow(DkdApplication *pApplication, DkdWindow *pWindow)
{
    assert(pApplication != NULL);

    dkdBindApplicationWindow(pApplication, NULL);

    if (pWindow == NULL) {
        return;
    }

    assert(pWindow->pHandle != NULL);

    free(pWindow->windowSystemIntegrator.pData);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
    free(pWindow);
}

void
dkdGetDekoiWindowSystemIntegrator(
    DkdWindow *pWindow,
    const DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator)
{
    assert(pWindow != NULL);
    assert(ppWindowSystemIntegrator != NULL);

    *ppWindowSystemIntegrator = &pWindow->windowSystemIntegrator;
}

int
dkdBindWindowRenderer(DkdWindow *pWindow, DkdRenderer *pRenderer)
{
    assert(pWindow != NULL);

    pWindow->pRenderer = pRenderer;
    return 0;
}

void
dkdGetWindowCloseFlag(const DkdWindow *pWindow, int *pCloseFlag)
{
    assert(pWindow != NULL);
    assert(pWindow->pHandle != NULL);
    assert(pCloseFlag != NULL);

    *pCloseFlag = glfwWindowShouldClose(pWindow->pHandle);
}

int
dkdPollWindowEvents(const DkdWindow *pWindow)
{
    DKD_UNUSED(pWindow);

    assert(pWindow != NULL);

    glfwPollEvents();
    return 0;
}

int
dkdRenderWindowImage(const DkdWindow *pWindow)
{
    assert(pWindow != NULL);

    if (dkdDrawRendererImage(pWindow->pRenderer)) {
        return 1;
    }

    return 0;
}
