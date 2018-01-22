#include "allocator.h"

#include "../common/allocator.h"
#include "../common/application.h"
#include "../common/bootstrap.h"
#include "../common/common.h"

#include <dekoi/graphics/renderer>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

const char *pApplicationName = "customallocator";
const unsigned int majorVersion = 1;
const unsigned int minorVersion = 0;
const unsigned int patchVersion = 0;
const unsigned int width = 1280;
const unsigned int height = 720;
const DkdShaderCreateInfo shaderInfos[]
    = {{DK_SHADER_STAGE_VERTEX, "shaders/triangle.vert.spv", "main"},
       {DK_SHADER_STAGE_FRAGMENT, "shaders/triangle.frag.spv", "main"}};
const float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};

typedef struct DkdApplicationCallbacksData {
    DkdAllocationCallbacks *pAllocator;
    size_t used;
} DkdApplicationCallbacksData;

int
dkdSetup(const DkdAllocationCallbacks *pAllocator,
         const DkdApplicationCallbacks *pApplicationCallbacks,
         DkdBootstrapHandles *pHandles)
{
    DkdBootstrapCreateInfos createInfos;

    assert(pHandles != NULL);

    createInfos.application.pName = pApplicationName;
    createInfos.application.majorVersion = majorVersion;
    createInfos.application.minorVersion = minorVersion;
    createInfos.application.patchVersion = patchVersion;
    createInfos.application.pLogger = NULL;
    createInfos.application.pAllocator = pAllocator;
    createInfos.application.pCallbacks = pApplicationCallbacks;

    createInfos.window.width = width;
    createInfos.window.height = height;
    createInfos.window.pTitle = pApplicationName;
    createInfos.window.pLogger = NULL;
    createInfos.window.pAllocator = pAllocator;

    createInfos.renderer.pApplicationName = pApplicationName;
    createInfos.renderer.applicationMajorVersion = majorVersion;
    createInfos.renderer.applicationMinorVersion = minorVersion;
    createInfos.renderer.applicationPatchVersion = patchVersion;
    createInfos.renderer.surfaceWidth = width;
    createInfos.renderer.surfaceHeight = height;
    createInfos.renderer.shaderCount = sizeof shaderInfos / sizeof *shaderInfos;
    createInfos.renderer.pShaderInfos = shaderInfos;
    createInfos.renderer.clearColor[0] = clearColor[0];
    createInfos.renderer.clearColor[1] = clearColor[1];
    createInfos.renderer.clearColor[2] = clearColor[2];
    createInfos.renderer.clearColor[3] = clearColor[3];
    createInfos.renderer.pLogger = NULL;
    createInfos.renderer.pAllocator = pAllocator;

    return dkdSetupBootstrap(&createInfos, pHandles);
}

void
dkdCleanup(DkdBootstrapHandles *pHandles)
{
    assert(pHandles != NULL);

    dkdCleanupBootstrap(pHandles);
}

void
dkdApplicationRunCallback(DkdApplication *pApplication, void *pData)
{
    DkdAllocationCallbacks *pAllocator;
    size_t used;

    DKD_UNUSED(pApplication);

    assert(pApplication != NULL);
    assert(pData != NULL);

    pAllocator = ((DkdApplicationCallbacksData *)pData)->pAllocator;
    used = ((DkdAllocationCallbacksData *)pAllocator->pData)->used;
    if (used != ((DkdApplicationCallbacksData *)pData)->used) {
        printf("memory in use: %zu bytes\n", used);
        ((DkdApplicationCallbacksData *)pData)->used = used;
    }
}

int
main(void)
{
    int out;
    DkdAllocationCallbacks *pAllocator;
    DkdApplicationCallbacksData applicationCallbacksData;
    DkdApplicationCallbacks applicationCallbacks;
    DkdBootstrapHandles handles;

    out = 0;

    dkdCreateCustomAllocator(&pAllocator);

    applicationCallbacksData.pAllocator = pAllocator;
    applicationCallbacksData.used = 0;

    applicationCallbacks.pData = &applicationCallbacksData;
    applicationCallbacks.pfnRun = dkdApplicationRunCallback;

    if (dkdSetup(pAllocator, &applicationCallbacks, &handles)) {
        out = 1;
        goto exit;
    }

    if (dkdRunApplication(handles.pApplication)) {
        out = 1;
        goto cleanup;
    }

cleanup:
    dkdCleanup(&handles);
    dkdDestroyCustomAllocator(pAllocator);

exit:
    return out;
}
