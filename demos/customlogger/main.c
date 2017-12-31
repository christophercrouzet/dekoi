#include "logger.h"

#include "../common/application.h"
#include "../common/bootstrap.h"
#include "../common/common.h"

#include <assert.h>
#include <stddef.h>

const char *pApplicationName = "customlogger";
const unsigned int majorVersion = 1;
const unsigned int minorVersion = 0;
const unsigned int patchVersion = 0;
const unsigned int width = 1280;
const unsigned int height = 720;
const DkdShaderCreateInfo pShaderInfos[]
    = {{DKD_SHADER_STAGE_VERTEX, "shaders/triangle.vert.spv", "main"},
       {DKD_SHADER_STAGE_FRAGMENT, "shaders/triangle.frag.spv", "main"}};
const float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};

int
dkdSetup(const DkdLoggingCallbacks *pLogger, DkdBootstrapHandles *pHandles)
{
    DkdBootstrapCreateInfos createInfos;

    assert(pHandles != NULL);

    createInfos.application.pName = pApplicationName;
    createInfos.application.majorVersion = majorVersion;
    createInfos.application.minorVersion = minorVersion;
    createInfos.application.patchVersion = patchVersion;
    createInfos.application.pLogger = pLogger;

    createInfos.window.width = width;
    createInfos.window.height = height;
    createInfos.window.pTitle = pApplicationName;
    createInfos.window.pLogger = pLogger;

    createInfos.renderer.pApplicationName = pApplicationName;
    createInfos.renderer.applicationMajorVersion = majorVersion;
    createInfos.renderer.applicationMinorVersion = minorVersion;
    createInfos.renderer.applicationPatchVersion = patchVersion;
    createInfos.renderer.surfaceWidth = width;
    createInfos.renderer.surfaceHeight = height;
    createInfos.renderer.shaderCount = 0;
    createInfos.renderer.pShaderInfos = pShaderInfos;
    createInfos.renderer.clearColor[0] = clearColor[0];
    createInfos.renderer.clearColor[1] = clearColor[1];
    createInfos.renderer.clearColor[2] = clearColor[2];
    createInfos.renderer.clearColor[3] = clearColor[3];
    createInfos.renderer.pLogger = pLogger;

    return dkdSetupBootstrap(&createInfos, pHandles);
}

void
dkdCleanup(DkdBootstrapHandles *pHandles)
{
    assert(pHandles != NULL);

    dkdCleanupBootstrap(pHandles);
}

int
main(void)
{
    int out;
    const DkdLoggingCallbacks *pLogger;
    DkdBootstrapHandles handles;

    out = 0;

    dkdGetCustomLogger(&pLogger);

    if (dkdSetup(pLogger, &handles)) {
        out = 1;
        goto exit;
    }

    if (dkdRunApplication(handles.pApplication)) {
        out = 1;
        goto cleanup;
    }

cleanup:
    dkdCleanup(&handles);

exit:
    return out;
}
