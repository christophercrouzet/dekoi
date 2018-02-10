#include "../common/application.h"
#include "../common/bootstrap.h"
#include "../common/common.h"

#include <dekoi/graphics/renderer>

#include <assert.h>
#include <stddef.h>
#include <string.h>

typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Vertex {
    Vector2 position;
    Vector3 color;
} Vertex;

static const Vertex vertices[] = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                  {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                  {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

static const char *pApplicationName = "vertexbuffer";
static const unsigned int majorVersion = 1;
static const unsigned int minorVersion = 0;
static const unsigned int patchVersion = 0;
static const unsigned int width = 1280;
static const unsigned int height = 720;
static const DkdShaderCreateInfo shaderInfos[]
    = {{DK_SHADER_STAGE_VERTEX, "shaders/passthrough.vert.spv", "main"},
       {DK_SHADER_STAGE_FRAGMENT, "shaders/passthrough.frag.spv", "main"}};
static const float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};
static const DkVertexBufferCreateInfo vertexBufferInfos[]
    = {{sizeof vertices, 0, vertices}};
static const DkVertexBindingDescriptionCreateInfo bindingDescriptionInfos[]
    = {{sizeof(Vertex), DK_VERTEX_INPUT_RATE_VERTEX}};
static const DkVertexAttributeDescriptionCreateInfo attributeDescriptionInfos[]
    = {{0, 0, offsetof(Vertex, position), DK_FORMAT_R32G32_SFLOAT},
       {0, 1, offsetof(Vertex, color), DK_FORMAT_R32G32B32_SFLOAT}};
static const uint32_t vertexCount = DKD_GET_ARRAY_SIZE(vertices);
static const uint32_t instanceCount = 1;

int
dkdSetup(DkdBootstrapHandles *pHandles)
{
    DkdBootstrapCreateInfos createInfos;

    assert(pHandles != NULL);

    memset(&createInfos, 0, sizeof createInfos);

    createInfos.application.pName = pApplicationName;
    createInfos.application.majorVersion = majorVersion;
    createInfos.application.minorVersion = minorVersion;
    createInfos.application.patchVersion = patchVersion;

    createInfos.window.width = width;
    createInfos.window.height = height;
    createInfos.window.pTitle = pApplicationName;

    createInfos.renderer.pApplicationName = pApplicationName;
    createInfos.renderer.applicationMajorVersion = majorVersion;
    createInfos.renderer.applicationMinorVersion = minorVersion;
    createInfos.renderer.applicationPatchVersion = patchVersion;
    createInfos.renderer.surfaceWidth = width;
    createInfos.renderer.surfaceHeight = height;
    createInfos.renderer.shaderCount = DKD_GET_ARRAY_SIZE(shaderInfos);
    createInfos.renderer.pShaderInfos = shaderInfos;
    createInfos.renderer.clearColor[0] = clearColor[0];
    createInfos.renderer.clearColor[1] = clearColor[1];
    createInfos.renderer.clearColor[2] = clearColor[2];
    createInfos.renderer.clearColor[3] = clearColor[3];
    createInfos.renderer.vertexBufferCount
        = DKD_GET_ARRAY_SIZE(vertexBufferInfos);
    createInfos.renderer.pVertexBufferInfos = vertexBufferInfos;
    createInfos.renderer.vertexBindingDescriptionCount
        = DKD_GET_ARRAY_SIZE(bindingDescriptionInfos);
    createInfos.renderer.pVertexBindingDescriptionInfos
        = bindingDescriptionInfos;
    createInfos.renderer.vertexAttributeDescriptionCount
        = DKD_GET_ARRAY_SIZE(attributeDescriptionInfos);
    createInfos.renderer.pVertexAttributeDescriptionInfos
        = attributeDescriptionInfos;
    createInfos.renderer.vertexCount = vertexCount;
    createInfos.renderer.instanceCount = instanceCount;

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