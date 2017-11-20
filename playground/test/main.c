#include <assert.h>
#include <stdlib.h>

#include <dekoi/renderer>

#include "application.h"
#include "renderer.h"
#include "test.h"
#include "window.h"


const char *pApplicationName = "dekoi";
const unsigned int majorVersion = 1;
const unsigned int minorVersion = 0;
const unsigned int patchVersion = 0;
const unsigned int width = 1280;
const unsigned int height = 720;
const DkShaderCreateInfo pShaderInfos[] = {
    { DK_SHADER_STAGE_VERTEX, "shaders/shader.vert.spv", "main" },
    { DK_SHADER_STAGE_FRAGMENT, "shaders/shader.frag.spv", "main" }
};
const DkFloat32 clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };


int
setup(Application **ppApplication,
      Window **ppWindow,
      Renderer **ppRenderer)
{
    int out;
    ApplicationCreateInfo applicationInfo;
    WindowCreateInfo windowInfo;
    const DkWindowCallbacks *pWindowRendererCallbacks;
    RendererCreateInfo rendererInfo;

    assert(ppApplication != NULL);
    assert(ppWindow != NULL);
    assert(ppRenderer != NULL);

    out = 0;

    applicationInfo.pName = pApplicationName;
    applicationInfo.majorVersion = majorVersion;
    applicationInfo.minorVersion = minorVersion;
    applicationInfo.patchVersion = patchVersion;

    if (createApplication(&applicationInfo, ppApplication)) {
        out = 1;
        goto exit;
    }

    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.title = pApplicationName;

    if (createWindow(*ppApplication, &windowInfo, ppWindow)) {
        out = 1;
        goto application_undo;
    }

    getWindowRendererCallbacks(*ppWindow, &pWindowRendererCallbacks);

    rendererInfo.pApplicationName = pApplicationName;
    rendererInfo.applicationMajorVersion = majorVersion;
    rendererInfo.applicationMinorVersion = minorVersion;
    rendererInfo.applicationPatchVersion = patchVersion;
    rendererInfo.surfaceWidth = width;
    rendererInfo.surfaceHeight = height;
    rendererInfo.pWindowCallbacks = pWindowRendererCallbacks;
    rendererInfo.shaderCount = sizeof pShaderInfos / sizeof *pShaderInfos;
    rendererInfo.pShaderInfos = pShaderInfos;
    rendererInfo.pClearColor = &clearColor;
    rendererInfo.pBackEndAllocator = NULL;

    if (createRenderer(*ppWindow, &rendererInfo, ppRenderer)) {
        out = 1;
        goto window_undo;
    }

    goto exit;

window_undo:
    destroyWindow(*ppApplication, *ppWindow);

application_undo:
    destroyApplication(*ppApplication);

exit:
    return out;
}


void
cleanup(Application *pApplication,
        Window *pWindow,
        Renderer *pRenderer)
{
    assert(pApplication != NULL);
    assert(pWindow != NULL);
    assert(pRenderer != NULL);

    destroyRenderer(pWindow, pRenderer);
    destroyWindow(pApplication, pWindow);
    destroyApplication(pApplication);
}


int
main(void)
{
    int out;
    Application *pApplication;
    Window *pWindow;
    Renderer *pRenderer;

    out = 0;

    if (setup(&pApplication, &pWindow, &pRenderer)) {
        out = 1;
        goto exit;
    }

    if (runApplication(pApplication)) {
        out = 1;
        goto cleanup;
    }

cleanup:
    cleanup(pApplication, pWindow, pRenderer);

exit:
    return out;
}
