#include "renderer.h"

#include "allocator.h"
#include "common.h"
#include "io.h"
#include "logger.h"
#include "window.h"

#include <dekoi/common/common>
#include <dekoi/graphics/graphics>
#include <dekoi/graphics/renderer>

#include <assert.h>
#include <stddef.h>

struct DkdRenderer {
    const DkdLoggingCallbacks *pLogger;
    const DkdAllocationCallbacks *pAllocator;
    DkdDekoiLoggingCallbacksData dekoiLoggerData;
    DkLoggingCallbacks *pDekoiLogger;
    DkdDekoiAllocationCallbacksData dekoiAllocatorData;
    DkAllocationCallbacks *pDekoiAllocator;
    DkRenderer *pHandle;
};

static int
dkdCreateShaderCode(const char *pFilePath,
                    const DkdAllocationCallbacks *pAllocator,
                    const DkdLoggingCallbacks *pLogger,
                    DkSize *pShaderCodeSize,
                    DkUint32 **ppShaderCode)
{
    int out;
    DkdFile file;

    assert(pFilePath != NULL);
    assert(pAllocator != NULL);
    assert(pLogger != NULL);
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

    *ppShaderCode = (DkUint32 *)DKD_ALLOCATE(pAllocator, *pShaderCodeSize);
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
    DKD_FREE(pAllocator, *ppShaderCode);

cleanup:;

file_closing:
    if (dkdCloseFile(&file, pLogger)) {
        out = 1;
    }

exit:
    return out;
}

static void
dkdDestroyShaderCode(DkUint32 *pShaderCode,
                     const DkdAllocationCallbacks *pAllocator)
{
    assert(pAllocator != NULL);

    DKD_FREE(pAllocator, pShaderCode);
}

int
dkdCreateRenderer(DkdWindow *pWindow,
                  const DkdRendererCreateInfo *pCreateInfo,
                  DkdRenderer **ppRenderer)
{
    int out;
    unsigned int i;
    const DkdLoggingCallbacks *pLogger;
    const DkdAllocationCallbacks *pAllocator;
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

    if (pCreateInfo->pAllocator == NULL) {
        dkdGetDefaultAllocator(&pAllocator);
    } else {
        pAllocator = pCreateInfo->pAllocator;
    }

    out = 0;

    dkdGetDekoiWindowSystemIntegrator(pWindow, &pWindowSystemIntegrator);

    if (pCreateInfo->shaderCount > 0) {
        pShaderInfos = (DkShaderCreateInfo *)DKD_ALLOCATE(
            pAllocator, sizeof *pShaderInfos * pCreateInfo->shaderCount);
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
                                    pAllocator,
                                    pLogger,
                                    &codeSize,
                                    &pCode)) {
                out = 1;
                goto shader_infos_cleanup;
            }

            pShaderInfos[i].stage = pCreateInfo->pShaderInfos[i].stage;
            pShaderInfos[i].codeSize = codeSize;
            pShaderInfos[i].pCode = pCode;
            pShaderInfos[i].pEntryPointName
                = pCreateInfo->pShaderInfos[i].pEntryPointName;
        }
    } else {
        pShaderInfos = NULL;
    }

    *ppRenderer = (DkdRenderer *)DKD_ALLOCATE(pAllocator, sizeof **ppRenderer);
    if (*ppRenderer == NULL) {
        DKD_LOG_ERROR(pLogger, "failed to allocate the renderer\n");
        out = 1;
        goto shader_infos_cleanup;
    }

    (*ppRenderer)->pLogger = pLogger;
    (*ppRenderer)->pAllocator = pAllocator;
    (*ppRenderer)->dekoiLoggerData.pLogger = pLogger;
    (*ppRenderer)->dekoiAllocatorData.pAllocator = pAllocator;

    if (dkdCreateDekoiLoggingCallbacks(&(*ppRenderer)->dekoiLoggerData,
                                       (*ppRenderer)->pAllocator,
                                       (*ppRenderer)->pLogger,
                                       &(*ppRenderer)->pDekoiLogger)) {
        out = 1;
        goto renderer_undo;
    }

    if (dkdCreateDekoiAllocationCallbacks(&(*ppRenderer)->dekoiAllocatorData,
                                          (*ppRenderer)->pAllocator,
                                          (*ppRenderer)->pLogger,
                                          &(*ppRenderer)->pDekoiAllocator)) {
        out = 1;
        goto dekoi_logger_undo;
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
    backEndInfo.vertexBufferCount = (DkUint32)pCreateInfo->vertexBufferCount;
    backEndInfo.pVertexBufferInfos = pCreateInfo->pVertexBufferInfos;
    backEndInfo.vertexBindingDescriptionCount
        = (DkUint32)pCreateInfo->vertexBindingDescriptionCount;
    backEndInfo.pVertexBindingDescriptionInfos
        = pCreateInfo->pVertexBindingDescriptionInfos;
    backEndInfo.vertexAttributeDescriptionCount
        = (DkUint32)pCreateInfo->vertexAttributeDescriptionCount;
    backEndInfo.pVertexAttributeDescriptionInfos
        = pCreateInfo->pVertexAttributeDescriptionInfos;
    backEndInfo.vertexCount = (DkUint32)pCreateInfo->vertexCount;
    backEndInfo.instanceCount = (DkUint32)pCreateInfo->instanceCount;
    backEndInfo.pLogger
        = pCreateInfo->pLogger == NULL ? NULL : (*ppRenderer)->pDekoiLogger;
    backEndInfo.pAllocator = pCreateInfo->pAllocator == NULL
                                 ? NULL
                                 : (*ppRenderer)->pDekoiAllocator;

    if (dkCreateRenderer(&backEndInfo, &(*ppRenderer)->pHandle) != DK_SUCCESS) {
        out = 1;
        goto dekoi_allocator_undo;
    }

    if (dkdBindWindowRenderer(pWindow, *ppRenderer)) {
        out = 1;
        goto dekoi_renderer_undo;
    }

    goto cleanup;

dekoi_renderer_undo:
    dkDestroyRenderer((*ppRenderer)->pHandle);

dekoi_allocator_undo:
    dkdDestroyDekoiAllocationCallbacks((*ppRenderer)->pDekoiAllocator,
                                       (*ppRenderer)->pAllocator);

dekoi_logger_undo:
    dkdDestroyDekoiLoggingCallbacks((*ppRenderer)->pDekoiLogger,
                                    (*ppRenderer)->pAllocator);

renderer_undo:
    DKD_FREE(pAllocator, *ppRenderer);

cleanup:;

shader_infos_cleanup:
    if (pShaderInfos != NULL) {
        for (i = 0; i < pCreateInfo->shaderCount; ++i) {
            if (pShaderInfos[i].pCode != NULL) {
                dkdDestroyShaderCode(pShaderInfos[i].pCode, pAllocator);
            }
        }

        DKD_FREE(pAllocator, pShaderInfos);
    }

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
    dkdDestroyDekoiAllocationCallbacks(pRenderer->pDekoiAllocator,
                                       pRenderer->pAllocator);
    dkdDestroyDekoiLoggingCallbacks(pRenderer->pDekoiLogger,
                                    pRenderer->pAllocator);
    DKD_FREE(pRenderer->pAllocator, pRenderer);
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
