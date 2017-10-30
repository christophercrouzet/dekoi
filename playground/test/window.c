#include <assert.h>
#include <stddef.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <dekoi/dekoi>

#include "application.h"
#include "window.h"


void
createWindow(Application *pApplication,
             const WindowCreateInfo *pCreateInfo,
             Window *pWindow)
{
    assert(pApplication != NULL);
    assert(pCreateInfo != NULL);
    assert(pWindow != NULL);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pWindow->pHandle = glfwCreateWindow((int) pCreateInfo->width,
                                        (int) pCreateInfo->height,
                                        pCreateInfo->title, NULL, NULL);
    pWindow->pRenderer = NULL;

    pApplication->pWindow = pWindow;
}


void
destroyWindow(Window *pWindow)
{
    assert(pWindow != NULL);

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

    glfwPollEvents();
}
