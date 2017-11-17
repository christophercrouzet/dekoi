#include <assert.h>
#include <stddef.h>

#include <dekoi/dekoi>
#include <dekoi/renderer>

#include "window.h"


struct Renderer {
    DkRenderer *pHandle;
};


int
createRenderer(Window *pWindow,
               const RendererCreateInfo *pCreateInfo,
               Renderer **ppRenderer)
{
    int out;
    assert(pWindow != NULL);
    assert(pCreateInfo != NULL);
    assert(ppRenderer != NULL);

    out = 0;
    if (dkCreateRenderer(pCreateInfo, NULL, &(*ppRenderer)->pHandle)
        != DK_SUCCESS)
    {
        out = 1;
        goto exit;
    }

    if (bindWindowRenderer(pWindow, *ppRenderer)) {
        out = 1;
        goto renderer_undo;
    }

    goto exit;

renderer_undo:
    dkDestroyRenderer((*ppRenderer)->pHandle, NULL);

exit:
    return out;
}


void
destroyRenderer(Window *pWindow,
                Renderer *pRenderer)
{
    assert(pWindow != NULL);
    assert(pRenderer != NULL);

    bindWindowRenderer(pWindow, NULL);
    dkDestroyRenderer(pRenderer->pHandle, NULL);
}


int
resizeRendererSurface(Renderer *pRenderer,
                      unsigned int width,
                      unsigned int height)
{
    assert(pRenderer != NULL);

    if (dkResizeRendererSurface(pRenderer->pHandle,
                                (DkUint32) width, (DkUint32) height)
        != DK_SUCCESS)
    {
        return 1;
    }

    return 0;
}