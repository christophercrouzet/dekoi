#ifndef DEKOI_DEMOS_COMMON_BOOTSTRAP_H
#define DEKOI_DEMOS_COMMON_BOOTSTRAP_H

#include "application.h"
#include "common.h"
#include "renderer.h"
#include "window.h"

struct DkdBootstrapCreateInfos {
    DkdApplicationCreateInfo application;
    DkdWindowCreateInfo window;
    DkdRendererCreateInfo renderer;
};

struct DkdBootstrapHandles {
    DkdApplication *pApplication;
    DkdWindow *pWindow;
    DkdRenderer *pRenderer;
};

int
dkdSetupBootstrap(const DkdBootstrapCreateInfos *pCreateInfos,
                  DkdBootstrapHandles *pHandles);

void
dkdCleanupBootstrap(DkdBootstrapHandles *pHandles);

#endif /* DEKOI_DEMOS_COMMON_BOOTSTRAP_H */
