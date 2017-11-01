#define GLFW_INCLUDE_VULKAN

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <GLFW/glfw3.h>
#include <dekoi/renderer>

#include "application.h"
#include "window.h"


void
createWindow(Application *pApplication,
             const WindowCreateInfo *pCreateInfo,
             Window *pWindow)
{
    DkRendererCreateInfo rendererInfo;

    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(pWindow != NULL);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    pWindow->pHandle = glfwCreateWindow((int) pCreateInfo->width,
                                        (int) pCreateInfo->height,
                                        pCreateInfo->title, NULL, NULL);

    memset(&rendererInfo, 0, sizeof(DkRendererCreateInfo));
    rendererInfo.pApplicationName = pApplication->pName;
    rendererInfo.applicationMajorVersion =
        (DkUint32) pApplication->majorVersion;
    rendererInfo.applicationMinorVersion =
        (DkUint32) pApplication->minorVersion;
    rendererInfo.applicationPatchVersion =
        (DkUint32) pApplication->patchVersion;
    rendererInfo.pBackEndAllocator = NULL;
    dkCreateRenderer(&rendererInfo, NULL, &pWindow->pRenderer);

    pApplication->pWindow = pWindow;
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


void
pollWindowEvents(const Window *pWindow)
{
    assert(pWindow != NULL);
    UNUSED(pWindow);

    glfwPollEvents();
}
