#define GLFW_INCLUDE_VULKAN

#include "window.h"

#include "application.h"
#include "rendering.h"

#include <GLFW/glfw3.h>
#include <dekoi/dekoi>
#include <dekoi/rendering>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct WindowSystemIntegrationCallbacksData {
    GLFWwindow *pWindowHandle;
} WindowSystemIntegrationCallbacksData;

struct Window {
    GLFWwindow *pHandle;
    DkWindowSystemIntegrationCallbacks windowSystemIntegrator;
    Renderer *pRenderer;
};

void
onFramebufferSizeChanged(GLFWwindow *pWindowHandle, int width, int height)
{
    Window *pWindow;

    assert(pWindowHandle != NULL);

    pWindow = (Window *)glfwGetWindowUserPointer(pWindowHandle);
    resizeRendererSurface(
        pWindow->pRenderer, (unsigned int)width, (unsigned int)height);
}

DkResult
createVulkanInstanceExtensionNames(void *pData,
                                   DkUint32 *pExtensionCount,
                                   const char ***pppExtensionNames)
{
    assert(pExtensionCount != NULL);
    assert(pppExtensionNames != NULL);

    UNUSED(pData);

    *pppExtensionNames
        = glfwGetRequiredInstanceExtensions((uint32_t *)pExtensionCount);
    if (*pppExtensionNames == NULL) {
        fprintf(stderr,
                "could not retrieve the Vulkan instance extension names\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

void
destroyVulkanInstanceExtensionNames(void *pData, const char **ppExtensionNames)
{
    assert(ppExtensionNames != NULL);

    UNUSED(pData);
    UNUSED(ppExtensionNames);
}

DkResult
createVulkanSurface(void *pData,
                    VkInstance instanceHandle,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    VkSurfaceKHR *pSurfaceHandle)
{
    assert(pData != NULL);
    assert(instanceHandle != NULL);
    assert(pSurfaceHandle != NULL);

    if (glfwCreateWindowSurface(
            instanceHandle,
            ((WindowSystemIntegrationCallbacksData *)pData)->pWindowHandle,
            pBackEndAllocator,
            pSurfaceHandle)
        != VK_SUCCESS) {
        fprintf(stderr, "failed to create the Vulkan surface\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

int
createWindow(Application *pApplication,
             const WindowCreateInfo *pCreateInfo,
             Window **ppWindow)
{
    int out;
    WindowSystemIntegrationCallbacksData *pWindowSystemIntegratorData;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(ppWindow != NULL);

    out = 0;

    *ppWindow = (Window *)malloc(sizeof **ppWindow);
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
        = (WindowSystemIntegrationCallbacksData *)malloc(
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
        = createVulkanInstanceExtensionNames;
    (*ppWindow)->windowSystemIntegrator.pfnDestroyInstanceExtensionNames
        = destroyVulkanInstanceExtensionNames;
    (*ppWindow)->windowSystemIntegrator.pfnCreateSurface = createVulkanSurface;

    glfwSetWindowUserPointer((*ppWindow)->pHandle, *ppWindow);

    glfwSetFramebufferSizeCallback((*ppWindow)->pHandle,
                                   onFramebufferSizeChanged);

    if (bindApplicationWindow(pApplication, *ppWindow)) {
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
destroyWindow(Application *pApplication, Window *pWindow)
{
    assert(pApplication != NULL);
    assert(pWindow != NULL);
    assert(pWindow->pHandle != NULL);

    bindApplicationWindow(pApplication, NULL);
    free(pWindow->windowSystemIntegrator.pData);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
    free(pWindow);
}

void
getWindowSystemIntegrator(
    Window *pWindow,
    const DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator)
{
    assert(pWindow != NULL);
    assert(ppWindowSystemIntegrator != NULL);

    *ppWindowSystemIntegrator = &pWindow->windowSystemIntegrator;
}

int
bindWindowRenderer(Window *pWindow, Renderer *pRenderer)
{
    assert(pWindow != NULL);

    pWindow->pRenderer = pRenderer;
    return 0;
}

void
getWindowCloseFlag(const Window *pWindow, int *pCloseFlag)
{
    assert(pWindow != NULL);
    assert(pWindow->pHandle != NULL);
    assert(pCloseFlag != NULL);

    *pCloseFlag = glfwWindowShouldClose(pWindow->pHandle);
}

int
pollWindowEvents(const Window *pWindow)
{
    assert(pWindow != NULL);
    UNUSED(pWindow);

    glfwPollEvents();
    return 0;
}

int
renderWindowImage(const Window *pWindow)
{
    assert(pWindow != NULL);

    if (drawRendererImage(pWindow->pRenderer)) {
        return 1;
    }

    return 0;
}
