#include "../common/application.h"
#include "../common/bootstrap.h"
#include "../common/common.h"

#include <dekoi/graphics/renderer>

#include <assert.h>
#include <stddef.h>

static const char *pApplicationName = "triangle";
static const unsigned int majorVersion = 1;
static const unsigned int minorVersion = 0;
static const unsigned int patchVersion = 0;
static const unsigned int width = 1280;
static const unsigned int height = 720;
static const DkdShaderCreateInfo shaderInfos[]
    = {{DK_SHADER_STAGE_VERTEX, "shaders/triangle.vert.spv", "main"},
       {DK_SHADER_STAGE_FRAGMENT, "shaders/triangle.frag.spv", "main"}};
static const float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
static const uint32_t vertexCount = 3;
static const uint32_t instanceCount = 1;

int
dkdSetup(DkdBootstrapHandles *pHandles)
{
    DkdBootstrapCreateInfos createInfos;

    assert(pHandles != NULL);

    createInfos.application.pName = pApplicationName;
    createInfos.application.majorVersion = majorVersion;
    createInfos.application.minorVersion = minorVersion;
    createInfos.application.patchVersion = patchVersion;
    createInfos.application.pLogger = NULL;
    createInfos.application.pAllocator = NULL;
    createInfos.application.pCallbacks = NULL;

    createInfos.window.width = width;
    createInfos.window.height = height;
    createInfos.window.pTitle = pApplicationName;
    createInfos.window.pLogger = NULL;
    createInfos.window.pAllocator = NULL;

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
    createInfos.renderer.vertexBufferCount = 0;
    createInfos.renderer.pVertexBufferInfos = NULL;
    createInfos.renderer.vertexBindingDescriptionCount = 0;
    createInfos.renderer.pVertexBindingDescriptionInfos = NULL;
    createInfos.renderer.vertexAttributeDescriptionCount = 0;
    createInfos.renderer.pVertexAttributeDescriptionInfos = NULL;
    createInfos.renderer.vertexCount = vertexCount;
    createInfos.renderer.instanceCount = instanceCount;
    createInfos.renderer.pLogger = NULL;
    createInfos.renderer.pAllocator = NULL;

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
    DkdBootstrapHandles handles;

    out = 0;

    if (dkdSetup(&handles)) {
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
