#include "bootstrap.h"

#include "application.h"
#include "logger.h"
#include "renderer.h"
#include "window.h"

#include <assert.h>
#include <stddef.h>

int
dkdSetupBootstrap(const struct DkdBootstrapCreateInfos *pCreateInfos,
                  struct DkdBootstrapHandles *pHandles)
{
    int out;

    assert(pCreateInfos != NULL);
    assert(pHandles != NULL);

    out = 0;

    if (dkdCreateApplication(&pCreateInfos->application,
                             &pHandles->pApplication)) {
        out = 1;
        goto exit;
    }

    if (dkdCreateWindow(pHandles->pApplication,
                        &pCreateInfos->window,
                        &pHandles->pWindow)) {
        out = 1;
        goto application_undo;
    }

    if (dkdCreateRenderer(
            pHandles->pWindow, &pCreateInfos->renderer, &pHandles->pRenderer)) {
        out = 1;
        goto window_undo;
    }

    goto exit;

window_undo:
    dkdDestroyWindow(pHandles->pApplication, pHandles->pWindow);

application_undo:
    dkdDestroyApplication(pHandles->pApplication);

exit:
    return out;
}

void
dkdCleanupBootstrap(struct DkdBootstrapHandles *pHandles)
{
    assert(pHandles != NULL);

    dkdDestroyRenderer(pHandles->pWindow, pHandles->pRenderer);
    dkdDestroyWindow(pHandles->pApplication, pHandles->pWindow);
    dkdDestroyApplication(pHandles->pApplication);
}
