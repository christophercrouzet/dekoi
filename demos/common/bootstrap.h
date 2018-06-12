#ifndef DEKOI_DEMOS_COMMON_BOOTSTRAP_H
#define DEKOI_DEMOS_COMMON_BOOTSTRAP_H

#include "application.h"
#include "renderer.h"
#include "window.h"

struct DkdBootstrapCreateInfos {
    struct DkdApplicationCreateInfo application;
    struct DkdWindowCreateInfo window;
    struct DkdRendererCreateInfo renderer;
};

struct DkdBootstrapHandles {
    struct DkdApplication *pApplication;
    struct DkdWindow *pWindow;
    struct DkdRenderer *pRenderer;
};

int
dkdSetupBootstrap(struct DkdBootstrapHandles *pHandles,
                  const struct DkdBootstrapCreateInfos *pCreateInfos);

void
dkdCleanupBootstrap(struct DkdBootstrapHandles *pHandles);

#endif /* DEKOI_DEMOS_COMMON_BOOTSTRAP_H */
