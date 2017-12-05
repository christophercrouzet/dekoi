#include "renderer.h"

#include "io.h"
#include "window.h"

#include <dekoi/dekoi>
#include <dekoi/renderer>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct Renderer {
    DkRenderer *pHandle;
};

static int
createShaderCode(const char *pFilePath,
                 DkSize *pShaderCodeSize,
                 DkUint32 **ppShaderCode)
{
    int out;
    File file;

    assert(pFilePath != NULL);
    assert(pShaderCodeSize != NULL);
    assert(ppShaderCode != NULL);

    out = 0;

    if (openFile(&file, pFilePath, "rb")) {
        out = 1;
        goto exit;
    }

    if (getFileSize(&file, pShaderCodeSize)) {
        out = 1;
        goto file_closing;
    }

    assert(*pShaderCodeSize % sizeof **ppShaderCode == 0);

    *ppShaderCode = (DkUint32 *)malloc(*pShaderCodeSize);
    if (*ppShaderCode == NULL) {
        fprintf(stderr,
                "failed to allocate the shader code for the file '%s'\n",
                pFilePath);
        out = 1;
        goto file_closing;
    }

    if (readFile(&file, *pShaderCodeSize, (void *)*ppShaderCode)) {
        out = 1;
        goto code_undo;
    }

    goto cleanup;

code_undo:
    free(*ppShaderCode);

cleanup:;

file_closing:
    if (closeFile(&file)) {
        out = 1;
    }

exit:
    return out;
}

static void
destroyShaderCode(DkUint32 *pShaderCode)
{
    assert(pShaderCode != NULL);

    free(pShaderCode);
}

static DkShaderStage
dkpTranslateShaderStage(ShaderStage shaderStage)
{
    switch (shaderStage) {
        case SHADER_STAGE_VERTEX:
            return DK_SHADER_STAGE_VERTEX;
        case SHADER_STAGE_TESSELLATION_CONTROL:
            return DK_SHADER_STAGE_TESSELLATION_CONTROL;
        case SHADER_STAGE_TESSELLATION_EVALUATION:
            return DK_SHADER_STAGE_TESSELLATION_EVALUATION;
        case SHADER_STAGE_GEOMETRY:
            return DK_SHADER_STAGE_GEOMETRY;
        case SHADER_STAGE_FRAGMENT:
            return DK_SHADER_STAGE_FRAGMENT;
        case SHADER_STAGE_COMPUTE:
            return DK_SHADER_STAGE_COMPUTE;
        default:
            assert(0);
            return (DkShaderStage)0;
    }
}

int
createRenderer(Window *pWindow,
               const RendererCreateInfo *pCreateInfo,
               Renderer **ppRenderer)
{
    int out;
    unsigned int i;
    const DkWindowCallbacks *pWindowRendererCallbacks;
    DkShaderCreateInfo *pShaderInfos;
    DkRendererCreateInfo backEndInfo;

    assert(pWindow != NULL);
    assert(pCreateInfo != NULL);
    assert(ppRenderer != NULL);

    out = 0;

    *ppRenderer = (Renderer *)malloc(sizeof **ppRenderer);
    if (*ppRenderer == NULL) {
        fprintf(stderr, "failed to allocate the renderer\n");
        out = 1;
        goto exit;
    }

    getWindowRendererCallbacks(pWindow, &pWindowRendererCallbacks);

    if (pCreateInfo->shaderCount > 0) {
        pShaderInfos = (DkShaderCreateInfo *)malloc(sizeof *pShaderInfos
                                                    * pCreateInfo->shaderCount);
        if (pShaderInfos == NULL) {
            fprintf(stderr, "failed to allocate the shader infos\n");
            out = 1;
            goto exit;
        }

        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            pShaderInfos[i].pCode = NULL;
        }

        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            DkSize codeSize;
            DkUint32 *pCode;

            if (createShaderCode(pCreateInfo->pShaderInfos[i].pFilePath,
                                 &codeSize,
                                 &pCode)) {
                out = 1;
                goto shader_infos_cleanup;
            }

            pShaderInfos[i].stage
                = dkpTranslateShaderStage(pCreateInfo->pShaderInfos[i].stage);
            pShaderInfos[i].codeSize = codeSize;
            pShaderInfos[i].pCode = pCode;
            pShaderInfos[i].pEntryPointName
                = pCreateInfo->pShaderInfos[i].pEntryPointName;
        }
    } else {
        pShaderInfos = NULL;
    }

    backEndInfo.pApplicationName = pCreateInfo->pApplicationName;
    backEndInfo.applicationMajorVersion
        = (DkUint32)pCreateInfo->applicationMajorVersion;
    backEndInfo.applicationMinorVersion
        = (DkUint32)pCreateInfo->applicationMinorVersion;
    backEndInfo.applicationPatchVersion
        = (DkUint32)pCreateInfo->applicationPatchVersion;
    backEndInfo.surfaceWidth = (DkUint32)pCreateInfo->surfaceWidth;
    backEndInfo.surfaceHeight = (DkUint32)pCreateInfo->surfaceHeight;
    backEndInfo.pWindowCallbacks = pWindowRendererCallbacks;
    backEndInfo.shaderCount = (DkUint32)pCreateInfo->shaderCount;
    backEndInfo.pShaderInfos = pShaderInfos;
    backEndInfo.clearColor[0] = (DkFloat32)pCreateInfo->clearColor[0];
    backEndInfo.clearColor[1] = (DkFloat32)pCreateInfo->clearColor[1];
    backEndInfo.clearColor[2] = (DkFloat32)pCreateInfo->clearColor[2];
    backEndInfo.clearColor[3] = (DkFloat32)pCreateInfo->clearColor[3];
    backEndInfo.pBackEndAllocator = NULL;

    if (dkCreateRenderer(&backEndInfo, NULL, &(*ppRenderer)->pHandle)
        != DK_SUCCESS) {
        out = 1;
        goto shader_infos_cleanup;
    }

    if (bindWindowRenderer(pWindow, *ppRenderer)) {
        out = 1;
        goto renderer_undo;
    }

    goto cleanup;

renderer_undo:
    dkDestroyRenderer((*ppRenderer)->pHandle, NULL);

cleanup:;

shader_infos_cleanup:
    for (i = 0; i < pCreateInfo->shaderCount; ++i) {
        if (pShaderInfos[i].pCode != NULL) {
            destroyShaderCode(pShaderInfos[i].pCode);
        }
    }

    free(pShaderInfos);

exit:
    return out;
}

void
destroyRenderer(Window *pWindow, Renderer *pRenderer)
{
    assert(pWindow != NULL);
    assert(pRenderer != NULL);
    assert(pRenderer->pHandle != NULL);

    bindWindowRenderer(pWindow, NULL);
    dkDestroyRenderer(pRenderer->pHandle, NULL);
    free(pRenderer);
}

int
resizeRendererSurface(Renderer *pRenderer,
                      unsigned int width,
                      unsigned int height)
{
    assert(pRenderer != NULL);

    if (dkResizeRendererSurface(
            pRenderer->pHandle, (DkUint32)width, (DkUint32)height)
        != DK_SUCCESS) {
        return 1;
    }

    return 0;
}

int
drawRendererImage(Renderer *pRenderer)
{
    assert(pRenderer != NULL);

    if (dkDrawRendererImage(pRenderer->pHandle) != DK_SUCCESS) {
        return 1;
    }

    return 0;
}
