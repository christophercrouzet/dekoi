#include "bootstrap.h"

#include "application.h"
#include "logger.h"
#include "renderer.h"
#include "window.h"

#include <assert.h>
#include <stddef.h>

int
dkdSetupBootstrap(struct DkdBootstrapHandles *pHandles,
                  const struct DkdBootstrapCreateInfos *pCreateInfos)
{
    int out;

    assert(pHandles != NULL);
    assert(pCreateInfos != NULL);

    out = 0;

    if (dkdCreateApplication(&pHandles->pApplication,
                             &pCreateInfos->application)) {
        out = 1;
        goto exit;
    }

    if (dkdCreateWindow(&pHandles->pWindow,
                        pHandles->pApplication,
                        &pCreateInfos->window)) {
        out = 1;
        goto application_undo;
    }

    if (dkdCreateRenderer(
            &pHandles->pRenderer, pHandles->pWindow, &pCreateInfos->renderer)) {
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
