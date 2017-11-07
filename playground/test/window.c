#define GLFW_INCLUDE_VULKAN

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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
             Window *pWindow)
{
    int out;
    WindowManagerInterfaceContext windowManagerInterfaceContext;
    DkWindowManagerInterface windowManagerInterface;
    DkRendererCreateInfo rendererInfo;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(pWindow != NULL);

    if (glfwInit() != GLFW_TRUE) {
        fprintf(stderr, "failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    out = 0;
    pWindow->pHandle = glfwCreateWindow((int) pCreateInfo->width,
                                        (int) pCreateInfo->height,
                                        pCreateInfo->title, NULL, NULL);
    if (pWindow->pHandle == NULL) {
        fprintf(stderr, "failed to create the window\n");
        out = 1;
        goto glfw_cleanup;
    }

    windowManagerInterfaceContext.pWindow = pWindow->pHandle;

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

    if (dkCreateRenderer(&rendererInfo, NULL, &pWindow->pRenderer)
        != DK_SUCCESS)
    {
        fprintf(stderr, "failed to create the renderer\n");
        out = 1;
        goto window_cleanup;
    }

    pApplication->pWindow = pWindow;
    return out;

window_cleanup:
    glfwDestroyWindow(pWindow->pHandle);

glfw_cleanup:
    glfwTerminate();
    return out;
}


void
destroyWindow(Window *pWindow)
{
    assert(pWindow != NULL);

    dkDestroyRenderer(pWindow->pRenderer);
    glfwDestroyWindow(pWindow->pHandle);
    glfwTerminate();
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
