#ifndef DEKOI_PLAYGROUND_TEST_WINDOW_H
#define DEKOI_PLAYGROUND_TEST_WINDOW_H

#include "test.h"

struct DkWindowSystemIntegrationCallbacks;

struct PlWindowCreateInfo {
    unsigned int width;
    unsigned int height;
    const char *title;
    const PlLoggingCallbacks *pLogger;
};

int
plCreateWindow(PlApplication *pApplication,
               const PlWindowCreateInfo *pCreateInfo,
               PlWindow **ppWindow);

void
plDestroyWindow(PlApplication *pApplication, PlWindow *pWindow);

void
plGetDekoiWindowSystemIntegrator(
    PlWindow *pWindow,
    const struct DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator);

int
plBindWindowRenderer(PlWindow *pWindow, PlRenderer *pRenderer);

void
plGetWindowCloseFlag(const PlWindow *pWindow, int *pCloseFlag);

int
plPollWindowEvents(const PlWindow *pWindow);

int
plRenderWindowImage(const PlWindow *pWindow);

#endif /* DEKOI_PLAYGROUND_TEST_WINDOW_H */
