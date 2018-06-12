#ifndef DEKOI_DEMOS_COMMON_WINDOW_H
#define DEKOI_DEMOS_COMMON_WINDOW_H

struct DkdApplication;
struct DkdRenderer;
struct DkdWindow;
struct DkWindowSystemIntegrationCallbacks;

struct DkdWindowCreateInfo {
    unsigned int width;
    unsigned int height;
    const char *pTitle;
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;
};

int
dkdCreateWindow(struct DkdApplication *pApplication,
                const struct DkdWindowCreateInfo *pCreateInfo,
                struct DkdWindow **ppWindow);

void
dkdDestroyWindow(struct DkdApplication *pApplication,
                 struct DkdWindow *pWindow);

void
dkdGetDekoiWindowSystemIntegrator(
    struct DkdWindow *pWindow,
    const struct DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator);

int
dkdBindWindowRenderer(struct DkdWindow *pWindow, struct DkdRenderer *pRenderer);

void
dkdGetWindowCloseFlag(const struct DkdWindow *pWindow, int *pCloseFlag);

int
dkdPollWindowEvents(const struct DkdWindow *pWindow);

int
dkdRenderWindowImage(const struct DkdWindow *pWindow);

#endif /* DEKOI_DEMOS_COMMON_WINDOW_H */
