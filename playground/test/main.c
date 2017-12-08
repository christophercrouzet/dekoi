#include "application.h"
#include "logging.h"
#include "rendering.h"
#include "test.h"
#include "window.h"

#include <assert.h>
#include <stdlib.h>

const char *pApplicationName = "dekoi";
const unsigned int majorVersion = 1;
const unsigned int minorVersion = 0;
const unsigned int patchVersion = 0;
const unsigned int width = 1280;
const unsigned int height = 720;
const PlShaderCreateInfo pShaderInfos[]
    = {{PL_SHADER_STAGE_VERTEX, "shaders/shader.vert.spv", "main"},
       {PL_SHADER_STAGE_FRAGMENT, "shaders/shader.frag.spv", "main"}};
const float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};

int
plSetup(const PlLoggingCallbacks *pLogger,
        PlApplication **ppApplication,
        PlWindow **ppWindow,
        PlRenderer **ppRenderer)
{
    int out;
    PlApplicationCreateInfo applicationInfo;
    PlWindowCreateInfo windowInfo;
    PlRendererCreateInfo rendererInfo;

    assert(ppApplication != NULL);
    assert(ppWindow != NULL);
    assert(ppRenderer != NULL);

    out = 0;

    applicationInfo.pName = pApplicationName;
    applicationInfo.majorVersion = majorVersion;
    applicationInfo.minorVersion = minorVersion;
    applicationInfo.patchVersion = patchVersion;

    if (plCreateApplication(&applicationInfo, pLogger, ppApplication)) {
        out = 1;
        goto exit;
    }

    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.title = pApplicationName;

    if (plCreateWindow(*ppApplication, &windowInfo, pLogger, ppWindow)) {
        out = 1;
        goto application_undo;
    }

    rendererInfo.pApplicationName = pApplicationName;
    rendererInfo.applicationMajorVersion = majorVersion;
    rendererInfo.applicationMinorVersion = minorVersion;
    rendererInfo.applicationPatchVersion = patchVersion;
    rendererInfo.surfaceWidth = width;
    rendererInfo.surfaceHeight = height;
    rendererInfo.shaderCount = sizeof pShaderInfos / sizeof *pShaderInfos;
    rendererInfo.pShaderInfos = pShaderInfos;
    rendererInfo.clearColor[0] = clearColor[0];
    rendererInfo.clearColor[1] = clearColor[1];
    rendererInfo.clearColor[2] = clearColor[2];
    rendererInfo.clearColor[3] = clearColor[3];

    if (plCreateRenderer(*ppWindow, &rendererInfo, pLogger, ppRenderer)) {
        out = 1;
        goto window_undo;
    }

    goto exit;

window_undo:
    plDestroyWindow(*ppApplication, *ppWindow);

application_undo:
    plDestroyApplication(*ppApplication);

exit:
    return out;
}

void
plCleanup(PlApplication *pApplication, PlWindow *pWindow, PlRenderer *pRenderer)
{
    assert(pApplication != NULL);
    assert(pWindow != NULL);
    assert(pRenderer != NULL);

    plDestroyRenderer(pWindow, pRenderer);
    plDestroyWindow(pApplication, pWindow);
    plDestroyApplication(pApplication);
}

int
main(void)
{
    int out;
    const PlLoggingCallbacks *pLogger;
    PlApplication *pApplication;
    PlWindow *pWindow;
    PlRenderer *pRenderer;

    out = 0;

    plGetDefaultLogger(&pLogger);

    if (plSetup(pLogger, &pApplication, &pWindow, &pRenderer)) {
        out = 1;
        goto exit;
    }

    if (plRunApplication(pApplication)) {
        out = 1;
        goto cleanup;
    }

cleanup:
    plCleanup(pApplication, pWindow, pRenderer);

exit:
    return out;
}
