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
dkdCreateWindow(struct DkdWindow **ppWindow,
                struct DkdApplication *pApplication,
                const struct DkdWindowCreateInfo *pCreateInfo);

void
dkdDestroyWindow(struct DkdApplication *pApplication,
                 struct DkdWindow *pWindow);

void
dkdGetDekoiWindowSystemIntegrator(
    const struct DkWindowSystemIntegrationCallbacks **ppWindowSystemIntegrator,
    struct DkdWindow *pWindow);

int
dkdBindWindowRenderer(struct DkdWindow *pWindow, struct DkdRenderer *pRenderer);

void
dkdGetWindowCloseFlag(int *pCloseFlag, const struct DkdWindow *pWindow);

int
dkdPollWindowEvents(const struct DkdWindow *pWindow);

int
dkdRenderWindowImage(const struct DkdWindow *pWindow);

#endif /* DEKOI_DEMOS_COMMON_WINDOW_H */
