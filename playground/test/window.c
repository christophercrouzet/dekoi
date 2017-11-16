#define GLFW_INCLUDE_VULKAN

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <dekoi/dekoi>
#include <dekoi/renderer>

#include "application.h"
#include "renderer.h"
#include "window.h"


typedef struct WindowRendererCallbacksContext {
    GLFWwindow *pWindowHandle;
} WindowRendererCallbacksContext;


struct Window {
    GLFWwindow *pHandle;
    DkWindowCallbacks windowRendererCallbacks;
    Renderer *pRenderer;
};


void
onFramebufferSizeChanged(GLFWwindow *pWindowHandle,
                         int width,
                         int height)
{
    Window *pWindow;

    assert(pWindowHandle != NULL);

    pWindow = (Window *) glfwGetWindowUserPointer(pWindowHandle);
    resizeRendererSurface(pWindow->pRenderer,
                          (unsigned int) width,
                          (unsigned int) height);
}


DkResult
createVulkanInstanceExtensionNames(void *pContext,
                                   DkUint32 *pExtensionCount,
                                   const char ***pppExtensionNames)
{
    assert(pExtensionCount != NULL);
    assert(pppExtensionNames != NULL);

    UNUSED(pContext);

    *pppExtensionNames = glfwGetRequiredInstanceExtensions(
        (uint32_t *) pExtensionCount);
    if (*pppExtensionNames == NULL) {
        fprintf(stderr, "could not retrieve the Vulkan instance extension "
                        "names\n");
        return DK_ERROR;
    }

    return DK_SUCCESS;
}


void
destroyVulkanInstanceExtensionNames(void *pContext,
                                    const char **ppExtensionNames)
{
    assert(ppExtensionNames != NULL);

    UNUSED(pContext);
    UNUSED(ppExtensionNames);
}


DkResult
createVulkanSurface(void *pContext,
                    VkInstance instanceHandle,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    VkSurfaceKHR *pSurfaceHandle)
{
    assert(pContext != NULL);
    assert(instanceHandle != NULL);
    assert(pSurfaceHandle != NULL);

    if (glfwCreateWindowSurface(
            instanceHandle,
            ((WindowRendererCallbacksContext *) pContext)->pWindowHandle,
            pBackEndAllocator,
            pSurfaceHandle)
        != VK_SUCCESS)
    {
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
    WindowRendererCallbacksContext *pWindowRendererCallbacksContext;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(ppWindow != NULL);

    out = 0;

    *ppWindow = (Window *) malloc(sizeof **ppWindow);
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

    (*ppWindow)->pHandle = glfwCreateWindow((int) pCreateInfo->width,
                                            (int) pCreateInfo->height,
                                            pCreateInfo->title, NULL, NULL);
    if ((*ppWindow)->pHandle == NULL) {
        fprintf(stderr, "failed to create the window\n");
        out = 1;
        goto glfw_undo;
    }

    pWindowRendererCallbacksContext = (WindowRendererCallbacksContext *)
        malloc(sizeof *pWindowRendererCallbacksContext);
    if (pWindowRendererCallbacksContext == NULL) {
        fprintf(stderr, "failed to allocate the window renderer callbacks "
                        "context\n");
        out = 1;
        goto glfw_window_undo;
    }

    pWindowRendererCallbacksContext->pWindowHandle = (*ppWindow)->pHandle;

    (*ppWindow)->windowRendererCallbacks.pContext = (void *)
        pWindowRendererCallbacksContext;
    (*ppWindow)->windowRendererCallbacks.pfnCreateInstanceExtensionNames =
        createVulkanInstanceExtensionNames;
    (*ppWindow)->windowRendererCallbacks.pfnDestroyInstanceExtensionNames =
        destroyVulkanInstanceExtensionNames;
    (*ppWindow)->windowRendererCallbacks.pfnCreateSurface = createVulkanSurface;

    glfwSetWindowUserPointer((*ppWindow)->pHandle, *ppWindow);

    glfwSetFramebufferSizeCallback((*ppWindow)->pHandle,
                                   onFramebufferSizeChanged);

    if (bindApplicationWindow(pApplication, *ppWindow)) {
        out = 1;
        goto window_renderer_callbacks_context_undo;
    }

    (*ppWindow)->pRenderer = NULL;
    goto exit;

window_renderer_callbacks_context_undo:
    free((*ppWindow)->windowRendererCallbacks.pContext);

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
destroyWindow(Application *pApplication,
              Window *pWindow)
{
    assert(pApplication != NULL);
    assert(pWindow != NULL);

    bindApplicationWindow(pApplication, NULL);
    free(pWindow->windowRendererCallbacks.pContext);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
    free(pWindow);
}


void
getWindowRendererCallbacks(Window *pWindow,
                           const DkWindowCallbacks **ppWindowRendererCallbacks)
{
    assert(pWindow != NULL);
    assert(ppWindowRendererCallbacks != NULL);

    *ppWindowRendererCallbacks = &pWindow->windowRendererCallbacks;
}


int
bindWindowRenderer(Window *pWindow,
                   Renderer *pRenderer)
{
    assert(pWindow != NULL);

    pWindow->pRenderer = pRenderer;
    return 0;
}


void
getWindowCloseFlag(const Window *pWindow,
                   int *pCloseFlag)
{
    assert(pWindow != NULL);
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
