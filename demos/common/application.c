#include "application.h"

#include "common.h"
#include "logger.h"
#include "window.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct DkdApplication {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    DkdWindow *pWindow;
    int stopFlag;
};

int
dkdCreateApplication(const DkdApplicationCreateInfo *pCreateInfo,
                     DkdApplication **ppApplication)
{
    const DkdLoggingCallbacks *pLogger;

    assert(pCreateInfo != NULL);
    assert(ppApplication != NULL);

    *ppApplication = NULL;

    if (pCreateInfo->pLogger == NULL) {
        dkdGetDefaultLogger(&pLogger);
    } else {
        pLogger = pCreateInfo->pLogger;
    }

    *ppApplication = (DkdApplication *)malloc(sizeof **ppApplication);
    if (*ppApplication == NULL) {
        DKD_LOG_ERROR(pLogger, "failed to allocate the application\n");
        return 1;
    }

    (*ppApplication)->pName = pCreateInfo->pName;
    (*ppApplication)->majorVersion = pCreateInfo->majorVersion;
    (*ppApplication)->minorVersion = pCreateInfo->minorVersion;
    (*ppApplication)->patchVersion = pCreateInfo->patchVersion;
    (*ppApplication)->pWindow = NULL;
    return 0;
}

void
dkdDestroyApplication(DkdApplication *pApplication)
{
    free(pApplication);
}

int
dkdBindApplicationWindow(DkdApplication *pApplication, DkdWindow *pWindow)
{
    assert(pApplication != NULL);

    pApplication->pWindow = pWindow;
    return 0;
}

int
dkdRunApplication(DkdApplication *pApplication)
{
    assert(pApplication != NULL);
    assert(pApplication->pWindow != NULL);

    while (1) {
        dkdPollWindowEvents(pApplication->pWindow);
        dkdGetWindowCloseFlag(pApplication->pWindow, &pApplication->stopFlag);
        if (pApplication->stopFlag) {
            break;
        }

        if (dkdRenderWindowImage(pApplication->pWindow)) {
            return 1;
        }
    }

    return 0;
}
