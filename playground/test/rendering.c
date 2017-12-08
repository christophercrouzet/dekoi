#include "rendering.h"

#include "io.h"
#include "logging.h"
#include "test.h"
#include "window.h"

#include <dekoi/dekoi>
#include <dekoi/rendering>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct PlRenderer {
    DkLoggingCallbacks *pDekoiLogger;
    DkRenderer *pHandle;
};

static int
plCreateShaderCode(const char *pFilePath,
                   const PlLoggingCallbacks *pLogger,
                   DkSize *pShaderCodeSize,
                   DkUint32 **ppShaderCode)
{
    int out;
    PlFile file;

    assert(pFilePath != NULL);
    assert(pShaderCodeSize != NULL);
    assert(ppShaderCode != NULL);

    out = 0;

    if (plOpenFile(&file, pFilePath, "rb", pLogger)) {
        out = 1;
        goto exit;
    }

    if (plGetFileSize(&file, pLogger, pShaderCodeSize)) {
        out = 1;
        goto file_closing;
    }

    assert(*pShaderCodeSize % sizeof **ppShaderCode == 0);

    *ppShaderCode = (DkUint32 *)malloc(*pShaderCodeSize);
    if (*ppShaderCode == NULL) {
        PL_ERROR_1(pLogger,
                   "failed to allocate the shader code for the file '%s'\n",
                   pFilePath);
        out = 1;
        goto file_closing;
    }

    if (plReadFile(&file, *pShaderCodeSize, pLogger, (void *)*ppShaderCode)) {
        out = 1;
        goto code_undo;
    }

    goto cleanup;

code_undo:
    free(*ppShaderCode);

cleanup:;

file_closing:
    if (plCloseFile(&file, pLogger)) {
        out = 1;
    }

exit:
    return out;
}

static void
plDestroyShaderCode(DkUint32 *pShaderCode)
{
    assert(pShaderCode != NULL);

    free(pShaderCode);
}

static DkShaderStage
plTranslateShaderStage(PlShaderStage shaderStage)
{
    switch (shaderStage) {
        case PL_SHADER_STAGE_VERTEX:
            return DK_SHADER_STAGE_VERTEX;
        case PL_SHADER_STAGE_TESSELLATION_CONTROL:
            return DK_SHADER_STAGE_TESSELLATION_CONTROL;
        case PL_SHADER_STAGE_TESSELLATION_EVALUATION:
            return DK_SHADER_STAGE_TESSELLATION_EVALUATION;
        case PL_SHADER_STAGE_GEOMETRY:
            return DK_SHADER_STAGE_GEOMETRY;
        case PL_SHADER_STAGE_FRAGMENT:
            return DK_SHADER_STAGE_FRAGMENT;
        case PL_SHADER_STAGE_COMPUTE:
            return DK_SHADER_STAGE_COMPUTE;
        default:
            assert(0);
            return (DkShaderStage)0;
    }
}

int
plCreateRenderer(PlWindow *pWindow,
                 const PlRendererCreateInfo *pCreateInfo,
                 const PlLoggingCallbacks *pLogger,
                 PlRenderer **ppRenderer)
{
    int out;
    unsigned int i;
    const DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator;
    DkShaderCreateInfo *pShaderInfos;
    DkRendererCreateInfo backEndInfo;

    assert(pWindow != NULL);
    assert(pCreateInfo != NULL);
    assert(ppRenderer != NULL);

    out = 0;

    plGetDekoiWindowSystemIntegrator(pWindow, &pWindowSystemIntegrator);

    if (pCreateInfo->shaderCount > 0) {
        pShaderInfos = (DkShaderCreateInfo *)malloc(sizeof *pShaderInfos
                                                    * pCreateInfo->shaderCount);
        if (pShaderInfos == NULL) {
            PL_ERROR_0(pLogger, "failed to allocate the shader infos\n");
            out = 1;
            goto exit;
        }

        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            pShaderInfos[i].pCode = NULL;
        }

        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            DkSize codeSize;
            DkUint32 *pCode;

            if (plCreateShaderCode(pCreateInfo->pShaderInfos[i].pFilePath,
                                   pLogger,
                                   &codeSize,
                                   &pCode)) {
                out = 1;
                goto shader_infos_cleanup;
            }

            pShaderInfos[i].stage
                = plTranslateShaderStage(pCreateInfo->pShaderInfos[i].stage);
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
    backEndInfo.pWindowSystemIntegrator = pWindowSystemIntegrator;
    backEndInfo.shaderCount = (DkUint32)pCreateInfo->shaderCount;
    backEndInfo.pShaderInfos = pShaderInfos;
    backEndInfo.clearColor[0] = (DkFloat32)pCreateInfo->clearColor[0];
    backEndInfo.clearColor[1] = (DkFloat32)pCreateInfo->clearColor[1];
    backEndInfo.clearColor[2] = (DkFloat32)pCreateInfo->clearColor[2];
    backEndInfo.clearColor[3] = (DkFloat32)pCreateInfo->clearColor[3];
    backEndInfo.pBackEndAllocator = NULL;

    *ppRenderer = (PlRenderer *)malloc(sizeof **ppRenderer);
    if (*ppRenderer == NULL) {
        PL_ERROR_0(pLogger, "failed to allocate the renderer\n");
        out = 1;
        goto shader_infos_cleanup;
    }

    if (plCreateDekoiLoggingCallbacks(pLogger, &(*ppRenderer)->pDekoiLogger)) {
        out = 1;
        goto renderer_undo;
    }

    if (dkCreateRenderer(&backEndInfo,
                         NULL,
                         (*ppRenderer)->pDekoiLogger,
                         &(*ppRenderer)->pHandle)
        != DK_SUCCESS) {
        out = 1;
        goto dekoi_logger_undo;
    }

    if (plBindWindowRenderer(pWindow, *ppRenderer)) {
        out = 1;
        goto dekoi_logger_undo;
    }

    goto cleanup;

dekoi_logger_undo:
    plDestroyDekoiLoggingCallbacks((*ppRenderer)->pDekoiLogger);

renderer_undo:
    dkDestroyRenderer((*ppRenderer)->pHandle);
    free(*ppRenderer);

cleanup:;

shader_infos_cleanup:
    for (i = 0; i < pCreateInfo->shaderCount; ++i) {
        if (pShaderInfos[i].pCode != NULL) {
            plDestroyShaderCode(pShaderInfos[i].pCode);
        }
    }

    free(pShaderInfos);

exit:
    return out;
}

void
plDestroyRenderer(PlWindow *pWindow, PlRenderer *pRenderer)
{
    assert(pWindow != NULL);
    assert(pRenderer != NULL);
    assert(pRenderer->pHandle != NULL);

    plBindWindowRenderer(pWindow, NULL);
    dkDestroyRenderer(pRenderer->pHandle);
    plDestroyDekoiLoggingCallbacks(pRenderer->pDekoiLogger);
    free(pRenderer);
}

int
plResizeRendererSurface(PlRenderer *pRenderer,
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
plDrawRendererImage(PlRenderer *pRenderer)
{
    assert(pRenderer != NULL);

    if (dkDrawRendererImage(pRenderer->pHandle) != DK_SUCCESS) {
        return 1;
    }

    return 0;
}
