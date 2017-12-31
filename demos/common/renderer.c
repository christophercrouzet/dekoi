#include "renderer.h"

#include "common.h"
#include "io.h"
#include "logger.h"
#include "window.h"

#include <dekoi/common/common>
#include <dekoi/graphics/graphics>
#include <dekoi/graphics/renderer>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct DkdRenderer {
    DkdDekoiLoggingCallbacksData dekoiLoggerData;
    DkLoggingCallbacks *pDekoiLogger;
    DkRenderer *pHandle;
};

static int
dkdCreateShaderCode(const char *pFilePath,
                    const DkdLoggingCallbacks *pLogger,
                    DkSize *pShaderCodeSize,
                    DkUint32 **ppShaderCode)
{
    int out;
    DkdFile file;

    assert(pFilePath != NULL);
    assert(pShaderCodeSize != NULL);
    assert(ppShaderCode != NULL);

    out = 0;

    if (dkdOpenFile(&file, pFilePath, "rb", pLogger)) {
        out = 1;
        goto exit;
    }

    if (dkdGetFileSize(&file, pLogger, (size_t *)pShaderCodeSize)) {
        out = 1;
        goto file_closing;
    }

    assert(*pShaderCodeSize % sizeof **ppShaderCode == 0);

    *ppShaderCode = (DkUint32 *)malloc(*pShaderCodeSize);
    if (*ppShaderCode == NULL) {
        DKD_LOG_ERROR(pLogger,
                      "failed to allocate the shader code for the file '%s'\n",
                      pFilePath);
        out = 1;
        goto file_closing;
    }

    if (dkdReadFile(&file, *pShaderCodeSize, pLogger, (void *)*ppShaderCode)) {
        out = 1;
        goto code_undo;
    }

    goto cleanup;

code_undo:
    free(*ppShaderCode);

cleanup:;

file_closing:
    if (dkdCloseFile(&file, pLogger)) {
        out = 1;
    }

exit:
    return out;
}

static void
dkdDestroyShaderCode(DkUint32 *pShaderCode)
{
    free(pShaderCode);
}

static DkShaderStage
dkdTranslateShaderStage(DkdShaderStage shaderStage)
{
    switch (shaderStage) {
        case DKD_SHADER_STAGE_VERTEX:
            return DK_SHADER_STAGE_VERTEX;
        case DKD_SHADER_STAGE_TESSELLATION_CONTROL:
            return DK_SHADER_STAGE_TESSELLATION_CONTROL;
        case DKD_SHADER_STAGE_TESSELLATION_EVALUATION:
            return DK_SHADER_STAGE_TESSELLATION_EVALUATION;
        case DKD_SHADER_STAGE_GEOMETRY:
            return DK_SHADER_STAGE_GEOMETRY;
        case DKD_SHADER_STAGE_FRAGMENT:
            return DK_SHADER_STAGE_FRAGMENT;
        case DKD_SHADER_STAGE_COMPUTE:
            return DK_SHADER_STAGE_COMPUTE;
        default:
            assert(0);
            return (DkShaderStage)0;
    }
}

int
dkdCreateRenderer(DkdWindow *pWindow,
                  const DkdRendererCreateInfo *pCreateInfo,
                  DkdRenderer **ppRenderer)
{
    int out;
    unsigned int i;
    const DkdLoggingCallbacks *pLogger;
    const DkWindowSystemIntegrationCallbacks *pWindowSystemIntegrator;
    DkShaderCreateInfo *pShaderInfos;
    DkRendererCreateInfo backEndInfo;

    assert(pWindow != NULL);
    assert(pCreateInfo != NULL);
    assert(ppRenderer != NULL);

    if (pCreateInfo->pLogger == NULL) {
        dkdGetDefaultLogger(&pLogger);
    } else {
        pLogger = pCreateInfo->pLogger;
    }

    out = 0;

    dkdGetDekoiWindowSystemIntegrator(pWindow, &pWindowSystemIntegrator);

    if (pCreateInfo->shaderCount > 0) {
        pShaderInfos = (DkShaderCreateInfo *)malloc(sizeof *pShaderInfos
                                                    * pCreateInfo->shaderCount);
        if (pShaderInfos == NULL) {
            DKD_LOG_ERROR(pLogger, "failed to allocate the shader infos\n");
            out = 1;
            goto exit;
        }

        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            pShaderInfos[i].pCode = NULL;
        }

        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            DkSize codeSize;
            DkUint32 *pCode;

            if (dkdCreateShaderCode(pCreateInfo->pShaderInfos[i].pFilePath,
                                    pLogger,
                                    &codeSize,
                                    &pCode)) {
                out = 1;
                goto shader_infos_cleanup;
            }

            pShaderInfos[i].stage
                = dkdTranslateShaderStage(pCreateInfo->pShaderInfos[i].stage);
            pShaderInfos[i].codeSize = codeSize;
            pShaderInfos[i].pCode = pCode;
            pShaderInfos[i].pEntryPointName
                = pCreateInfo->pShaderInfos[i].pEntryPointName;
        }
    } else {
        pShaderInfos = NULL;
    }

    *ppRenderer = (DkdRenderer *)malloc(sizeof **ppRenderer);
    if (*ppRenderer == NULL) {
        DKD_LOG_ERROR(pLogger, "failed to allocate the renderer\n");
        out = 1;
        goto shader_infos_cleanup;
    }

    (*ppRenderer)->dekoiLoggerData.pLogger = pLogger;

    if (dkdCreateDekoiLoggingCallbacks(&(*ppRenderer)->dekoiLoggerData,
                                       pLogger,
                                       &(*ppRenderer)->pDekoiLogger)) {
        out = 1;
        goto renderer_undo;
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
    backEndInfo.pLogger
        = pCreateInfo->pLogger == NULL ? NULL : (*ppRenderer)->pDekoiLogger;
    backEndInfo.pAllocator = NULL;

    if (dkCreateRenderer(&backEndInfo, &(*ppRenderer)->pHandle) != DK_SUCCESS) {
        out = 1;
        goto dekoi_logger_undo;
    }

    if (dkdBindWindowRenderer(pWindow, *ppRenderer)) {
        out = 1;
        goto dekoi_renderer_undo;
    }

    goto cleanup;

dekoi_renderer_undo:
    dkDestroyRenderer((*ppRenderer)->pHandle);

dekoi_logger_undo:
    dkdDestroyDekoiLoggingCallbacks((*ppRenderer)->pDekoiLogger);

renderer_undo:
    free(*ppRenderer);

cleanup:;

shader_infos_cleanup:
    for (i = 0; i < pCreateInfo->shaderCount; ++i) {
        if (pShaderInfos[i].pCode != NULL) {
            dkdDestroyShaderCode(pShaderInfos[i].pCode);
        }
    }

    free(pShaderInfos);

exit:
    return out;
}

void
dkdDestroyRenderer(DkdWindow *pWindow, DkdRenderer *pRenderer)
{
    assert(pWindow != NULL);

    dkdBindWindowRenderer(pWindow, NULL);

    if (pRenderer == NULL) {
        return;
    }

    assert(pRenderer->pHandle != NULL);
    assert(pRenderer->pDekoiLogger != NULL);

    dkDestroyRenderer(pRenderer->pHandle);
    dkdDestroyDekoiLoggingCallbacks(pRenderer->pDekoiLogger);
    free(pRenderer);
}

int
dkdResizeRendererSurface(DkdRenderer *pRenderer,
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
dkdDrawRendererImage(DkdRenderer *pRenderer)
{
    assert(pRenderer != NULL);

    if (dkDrawRendererImage(pRenderer->pHandle) != DK_SUCCESS) {
        return 1;
    }

    return 0;
}
