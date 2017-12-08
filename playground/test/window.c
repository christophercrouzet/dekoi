#define GLFW_INCLUDE_VULKAN

#include "window.h"

#include "application.h"
#include "rendering.h"
#include "test.h"

#include <GLFW/glfw3.h>
#include <dekoi/dekoi>
#include <dekoi/rendering>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct PlWindowSystemIntegrationCallbacksData {
    GLFWwindow *pWindowHandle;
} PlWindowSystemIntegrationCallbacksData;

struct PlWindow {
    GLFWwindow *pHandle;
    DkWindowSystemIntegrationCallbacks windowSystemIntegrator;
    PlRenderer *pRenderer;
};

static void
plOnFramebufferSizeChanged(GLFWwindow *pWindowHandle, int width, int height)
{
    PlWindow *pWindow;

    assert(pWindowHandle != NULL);

    pWindow = (PlWindow *)glfwGetWindowUserPointer(pWindowHandle);
    plResizeRendererSurface(
        pWindow->pRenderer, (unsigned int)width, (unsigned int)height);
}

static DkResult
plCreateVulkanInstanceExtensionNames(void *pData,
                                     DkUint32 *pExtensionCount,
                                     const char ***pppExtensionNames)
{
    assert(pExtensionCount != NULL);
    assert(pppExtensionNames != NULL);

    PL_UNUSED(pData);

    *pppExtensionNames
        = glfwGetRequiredInstanceExtensions((uint32_t *)pExtensionCount);
    if (*pppExtensionNames == NULL) {
        fprintf(stderr,
                "could not retrieve the Vulkan instance extension names\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

static void
plDestroyVulkanInstanceExtensionNames(void *pData,
                                      const char **ppExtensionNames)
{
    assert(ppExtensionNames != NULL);

    PL_UNUSED(pData);
    PL_UNUSED(ppExtensionNames);
}

static DkResult
plCreateVulkanSurface(void *pData,
                      VkInstance instanceHandle,
                      const VkAllocationCallbacks *pBackEndAllocator,
                      VkSurfaceKHR *pSurfaceHandle)
{
    assert(pData != NULL);
    assert(instanceHandle != NULL);
    assert(pSurfaceHandle != NULL);

    if (glfwCreateWindowSurface(
            instanceHandle,
            ((PlWindowSystemIntegrationCallbacksData *)pData)->pWindowHandle,
            pBackEndAllocator,
            pSurfaceHandle)
        != VK_SUCCESS) {
        fprintf(stderr, "failed to create the Vulkan surface\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

int
plCreateWindow(PlApplication *pApplication,
               const PlWindowCreateInfo *pCreateInfo,
               PlWindow **ppWindow)
{
    int out;
    PlWindowSystemIntegrationCallbacksData *pWindowSystemIntegratorData;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(ppWindow != NULL);

    out = 0;

    *ppWindow = (PlWindow *)malloc(sizeof **ppWindow);
    if (*ppWindow == NULL) {
        fprintf(stderr, "failed to allocate the window\n");
        out = 1;
        goto exit;
    }

    if (glfwInit() != GLFW_TRUE) {
        fprintf(stderr, "failed to initialize GLFW\n");
        out = 1;
        goto window_undo;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    (*ppWindow)->pHandle = glfwCreateWindow((int)pCreateInfo->width,
                                            (int)pCreateInfo->height,
                                            pCreateInfo->title,
                                            NULL,
                                            NULL);
    if ((*ppWindow)->pHandle == NULL) {
        fprintf(stderr, "failed to create the window\n");
        out = 1;
        goto glfw_undo;
    }

    pWindowSystemIntegratorData
        = (PlWindowSystemIntegrationCallbacksData *)malloc(
            sizeof *pWindowSystemIntegratorData);
    if (pWindowSystemIntegratorData == NULL) {
        fprintf(stderr,
                "failed to allocate the window renderer callbacks data\n");
        out = 1;
        goto glfw_window_undo;
    }

    pWindowSystemIntegratorData->pWindowHandle = (*ppWindow)->pHandle;

    (*ppWindow)->windowSystemIntegrator.pData
        = (void *)pWindowSystemIntegratorData;
    (*ppWindow)->windowSystemIntegrator.pfnCreateInstanceExtensionNames
        = plCreateVulkanInstanceExtensionNames;
    (*ppWindow)->windowSystemIntegrator.pfnDestroyInstanceExtensionNames
        = plDestroyVulkanInstanceExtensionNames;
    (*ppWindow)->windowSystemIntegrator.pfnCreateSurface
        = plCreateVulkanSurface;

    glfwSetWindowUserPointer((*ppWindow)->pHandle, *ppWindow);

    glfwSetFramebufferSizeCallback((*ppWindow)->pHandle,
                                   plOnFramebufferSizeChanged);

    if (plBindApplicationWindow(pApplication, *ppWindow)) {
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
plDestroyWindow(PlApplication *pApplication, PlWindow *pWindow)
{
    assert(pApplication != NULL);
    assert(pWindow != NULL);
    assert(pWindow->pHandle != NULL);

    plBindApplicationWindow(pApplication, NULL);
    free(pWindow->windowSystemIntegrator.pData);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
    free(pWindow);
}

void
plGetDekoiWindowSystemIntegrator(
    PlWindow *pWindow,
    const DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator)
{
    assert(pWindow != NULL);
    assert(ppWindowSystemIntegrator != NULL);

    *ppWindowSystemIntegrator = &pWindow->windowSystemIntegrator;
}

int
plBindWindowRenderer(PlWindow *pWindow, PlRenderer *pRenderer)
{
    assert(pWindow != NULL);

    pWindow->pRenderer = pRenderer;
    return 0;
}

void
plGetWindowCloseFlag(const PlWindow *pWindow, int *pCloseFlag)
{
    assert(pWindow != NULL);
    assert(pWindow->pHandle != NULL);
    assert(pCloseFlag != NULL);

    *pCloseFlag = glfwWindowShouldClose(pWindow->pHandle);
}

int
plPollWindowEvents(const PlWindow *pWindow)
{
    assert(pWindow != NULL);
    PL_UNUSED(pWindow);

    glfwPollEvents();
    return 0;
}

int
plRenderWindowImage(const PlWindow *pWindow)
{
    assert(pWindow != NULL);

    if (plDrawRendererImage(pWindow->pRenderer)) {
        return 1;
    }

    return 0;
}
