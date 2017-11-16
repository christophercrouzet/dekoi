#define GLFW_INCLUDE_VULKAN

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <dekoi/dekoi>
#include <dekoi/renderer>

#include "application.h"
#include "window.h"


typedef struct WindowRendererCallbacksContext {
    GLFWwindow *pWindowHandle;
} WindowRendererCallbacksContext;


struct Window {
    GLFWwindow *pHandle;
    DkRenderer *pRenderer;
};


void
onFramebufferSizeChanged(GLFWwindow *pWindowHandle,
                         int width,
                         int height)
{
    Window *pWindow;

    assert(pWindowHandle != NULL);

    pWindow = (Window *) glfwGetWindowUserPointer(pWindowHandle);
    dkResizeRendererSurface(pWindow->pRenderer,
                            (DkUint32) width, (DkUint32) height);
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
    WindowRendererCallbacksContext windowRendererCallbacksContext;
    DkWindowCallbacks windowRendererCallbacks;
    DkRendererCreateInfo rendererInfo;

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

    windowRendererCallbacksContext.pWindowHandle = (*ppWindow)->pHandle;

    windowRendererCallbacks.pContext = (void *) &windowRendererCallbacksContext;
    windowRendererCallbacks.pfnCreateInstanceExtensionNames =
        createVulkanInstanceExtensionNames;
    windowRendererCallbacks.pfnDestroyInstanceExtensionNames =
        destroyVulkanInstanceExtensionNames;
    windowRendererCallbacks.pfnCreateSurface = createVulkanSurface;

    memset(&rendererInfo, 0, sizeof rendererInfo);
    rendererInfo.pApplicationName = pApplication->pName;
    rendererInfo.applicationMajorVersion =
        (DkUint32) pApplication->majorVersion;
    rendererInfo.applicationMinorVersion =
        (DkUint32) pApplication->minorVersion;
    rendererInfo.applicationPatchVersion =
        (DkUint32) pApplication->patchVersion;
    rendererInfo.surfaceWidth = (DkUint32) pCreateInfo->width;
    rendererInfo.surfaceHeight = (DkUint32) pCreateInfo->height;
    rendererInfo.pWindowCallbacks = &windowRendererCallbacks;
    rendererInfo.pBackEndAllocator = NULL;

    if (dkCreateRenderer(&rendererInfo, NULL, &(*ppWindow)->pRenderer)
        != DK_SUCCESS)
    {
        fprintf(stderr, "failed to create the renderer\n");
        out = 1;
        goto glfw_window_undo;
    }

    glfwSetWindowUserPointer((*ppWindow)->pHandle, *ppWindow);

    glfwSetFramebufferSizeCallback((*ppWindow)->pHandle,
                                   onFramebufferSizeChanged);

    if (bindApplicationWindow(pApplication, *ppWindow)) {
        fprintf(stderr, "could not bind the window with the application\n");
        out = 1;
        goto glfw_window_undo;
    }

    goto exit;

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

    dkDestroyRenderer(pWindow->pRenderer, NULL);
    bindApplicationWindow(pApplication, NULL);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
    free(pWindow);
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
