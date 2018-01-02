#ifndef DEKOI_DEMOS_COMMON_WINDOW_H
#define DEKOI_DEMOS_COMMON_WINDOW_H

#include "common.h"

struct DkWindowSystemIntegrationCallbacks;

struct DkdWindowCreateInfo {
    unsigned int width;
    unsigned int height;
    const char *pTitle;
    const DkdLoggingCallbacks *pLogger;
    const DkdAllocationCallbacks *pAllocator;
};

int
dkdCreateWindow(DkdApplication *pApplication,
                const DkdWindowCreateInfo *pCreateInfo,
                DkdWindow **ppWindow);

void
dkdDestroyWindow(DkdApplication *pApplication, DkdWindow *pWindow);

void
dkdGetDekoiWindowSystemIntegrator(
    DkdWindow *pWindow,
    const struct DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator);

int
dkdBindWindowRenderer(DkdWindow *pWindow, DkdRenderer *pRenderer);

void
dkdGetWindowCloseFlag(const DkdWindow *pWindow, int *pCloseFlag);

int
dkdPollWindowEvents(const DkdWindow *pWindow);

int
dkdRenderWindowImage(const DkdWindow *pWindow);

#endif /* DEKOI_DEMOS_COMMON_WINDOW_H */
