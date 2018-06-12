#include "application.h"

#include "allocator.h"
#include "logger.h"
#include "window.h"

#include <assert.h>
#include <stddef.h>

struct DkdApplication {
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;
    const struct DkdApplicationCallbacks *pCallbacks;
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    struct DkdWindow *pWindow;
    int stopFlag;
};

int
dkdCreateApplication(struct DkdApplication **ppApplication,
                     const struct DkdApplicationCreateInfo *pCreateInfo)
{
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;

    assert(ppApplication != NULL);
    assert(pCreateInfo != NULL);

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

    *ppApplication = (struct DkdApplication *)DKD_ALLOCATE(
        pAllocator, sizeof **ppApplication);
    if (*ppApplication == NULL) {
        DKD_LOG_ERROR(pLogger, "failed to allocate the application\n");
        return 1;
    }

    (*ppApplication)->pLogger = pLogger;
    (*ppApplication)->pAllocator = pAllocator;
    (*ppApplication)->pCallbacks = pCreateInfo->pCallbacks;
    (*ppApplication)->pName = pCreateInfo->pName;
    (*ppApplication)->majorVersion = pCreateInfo->majorVersion;
    (*ppApplication)->minorVersion = pCreateInfo->minorVersion;
    (*ppApplication)->patchVersion = pCreateInfo->patchVersion;
    (*ppApplication)->pWindow = NULL;
    return 0;
}

void
dkdDestroyApplication(struct DkdApplication *pApplication)
{
    DKD_FREE(pApplication->pAllocator, pApplication);
}

int
dkdBindApplicationWindow(struct DkdApplication *pApplication,
                         struct DkdWindow *pWindow)
{
    assert(pApplication != NULL);

    pApplication->pWindow = pWindow;
    return 0;
}

int
dkdRunApplication(struct DkdApplication *pApplication)
{
    assert(pApplication != NULL);
    assert(pApplication->pWindow != NULL);

    while (1) {
        dkdPollWindowEvents(pApplication->pWindow);
        dkdGetWindowCloseFlag(&pApplication->stopFlag, pApplication->pWindow);
        if (pApplication->stopFlag) {
            break;
        }

        if (pApplication->pCallbacks != NULL
            && pApplication->pCallbacks->pfnRun != NULL) {
            pApplication->pCallbacks->pfnRun(pApplication,
                                             pApplication->pCallbacks->pData);
        }

        if (dkdRenderWindowImage(pApplication->pWindow)) {
            return 1;
        }
    }

    return 0;
}
