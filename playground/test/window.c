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


typedef struct WindowManagerInterfaceContext {
    GLFWwindow *pWindow;
} WindowManagerInterfaceContext;


DkResult
createVulkanInstanceExtensionNames(void *pContext,
                                   DkUint32 *pExtensionCount,
                                   const char ***pppExtensionNames)
{
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
    UNUSED(pContext);
    UNUSED(ppExtensionNames);
}


DkResult
createVulkanSurface(void *pContext,
                    VkInstance instance,
                    const VkAllocationCallbacks *pBackEndAllocator,
                    VkSurfaceKHR *pSurface)
{
    if (glfwCreateWindowSurface(
            instance,
            ((WindowManagerInterfaceContext *) pContext)->pWindow,
            pBackEndAllocator,
            pSurface)
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
    WindowManagerInterfaceContext windowManagerInterfaceContext;
    DkWindowManagerInterface windowManagerInterface;
    DkRendererCreateInfo rendererInfo;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(ppWindow != NULL);

    *ppWindow = (Window *) malloc(sizeof(Window));
    if (*ppWindow == NULL) {
        fprintf(stderr, "failed to allocate the window\n");
        return 1;
    }

    out = 0;
    if (glfwInit() != GLFW_TRUE) {
        fprintf(stderr, "failed to initialize GLFW\n");
        out = 1;
        goto window_cleanup;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    (*ppWindow)->pHandle = glfwCreateWindow((int) pCreateInfo->width,
                                            (int) pCreateInfo->height,
                                            pCreateInfo->title, NULL, NULL);
    if ((*ppWindow)->pHandle == NULL) {
        fprintf(stderr, "failed to create the window\n");
        out = 1;
        goto glfw_cleanup;
    }

    windowManagerInterfaceContext.pWindow = (*ppWindow)->pHandle;

    windowManagerInterface.pContext = (void *) &windowManagerInterfaceContext;
    windowManagerInterface.pfnCreateInstanceExtensionNames =
        createVulkanInstanceExtensionNames;
    windowManagerInterface.pfnDestroyInstanceExtensionNames =
        destroyVulkanInstanceExtensionNames;
    windowManagerInterface.pfnCreateSurface = createVulkanSurface;

    memset(&rendererInfo, 0, sizeof(DkRendererCreateInfo));
    rendererInfo.pApplicationName = pApplication->pName;
    rendererInfo.applicationMajorVersion =
        (DkUint32) pApplication->majorVersion;
    rendererInfo.applicationMinorVersion =
        (DkUint32) pApplication->minorVersion;
    rendererInfo.applicationPatchVersion =
        (DkUint32) pApplication->patchVersion;
    rendererInfo.pWindowManagerInterface = &windowManagerInterface;
    rendererInfo.pBackEndAllocator = NULL;

    if (dkCreateRenderer(&rendererInfo, NULL, &(*ppWindow)->pRenderer)
        != DK_SUCCESS)
    {
        fprintf(stderr, "failed to create the renderer\n");
        out = 1;
        goto glfw_window_cleanup;
    }

    pApplication->pWindow = *ppWindow;
    return out;

glfw_window_cleanup:
    glfwDestroyWindow((*ppWindow)->pHandle);

glfw_cleanup:
    glfwTerminate();

window_cleanup:
    free(*ppWindow);
    return out;
}


void
destroyWindow(Window *pWindow)
{
    assert(pWindow != NULL);

    dkDestroyRenderer(pWindow->pRenderer);
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
