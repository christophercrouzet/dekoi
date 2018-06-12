#include "logger.h"

#include "../common/application.h"
#include "../common/bootstrap.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

static const char *pApplicationName = "customlogger";
static const unsigned int majorVersion = 1;
static const unsigned int minorVersion = 0;
static const unsigned int patchVersion = 0;
static const unsigned int width = 1280;
static const unsigned int height = 720;
static const float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
static const uint32_t vertexCount = 0;
static const uint32_t instanceCount = 0;

int
dkdSetup(const struct DkdLoggingCallbacks *pLogger,
         struct DkdBootstrapHandles *pHandles)
{
    struct DkdBootstrapCreateInfos createInfos;

    assert(pHandles != NULL);

    memset(&createInfos, 0, sizeof createInfos);

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
    createInfos.renderer.clearColor[0] = clearColor[0];
    createInfos.renderer.clearColor[1] = clearColor[1];
    createInfos.renderer.clearColor[2] = clearColor[2];
    createInfos.renderer.clearColor[3] = clearColor[3];
    createInfos.renderer.vertexCount = vertexCount;
    createInfos.renderer.instanceCount = instanceCount;
    createInfos.renderer.pLogger = pLogger;

    return dkdSetupBootstrap(&createInfos, pHandles);
}

void
dkdCleanup(struct DkdBootstrapHandles *pHandles)
{
    assert(pHandles != NULL);

    dkdCleanupBootstrap(pHandles);
}

int
main(void)
{
    int out;
    const struct DkdLoggingCallbacks *pLogger;
    struct DkdBootstrapHandles handles;

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
