#include "application.h"

#include "logging.h"
#include "test.h"
#include "window.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct PlApplication {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    PlWindow *pWindow;
    int stopFlag;
};

int
plCreateApplication(const PlApplicationCreateInfo *pCreateInfo,
                    const PlLoggingCallbacks *pLogger,
                    PlApplication **ppApplication)
{
    assert(pCreateInfo != NULL);
    assert(ppApplication != NULL);

    *ppApplication = (PlApplication *)malloc(sizeof **ppApplication);
    if (*ppApplication == NULL) {
        PL_ERROR_0(pLogger, "failed to allocate the application\n");
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
plDestroyApplication(PlApplication *pApplication)
{
    assert(pApplication != NULL);

    free(pApplication);
}

int
plBindApplicationWindow(PlApplication *pApplication, PlWindow *pWindow)
{
    assert(pApplication != NULL);

    pApplication->pWindow = pWindow;
    return 0;
}

int
plRunApplication(PlApplication *pApplication)
{
    assert(pApplication != NULL);
    assert(pApplication->pWindow != NULL);

    while (1) {
        plPollWindowEvents(pApplication->pWindow);
        plGetWindowCloseFlag(pApplication->pWindow, &pApplication->stopFlag);
        if (pApplication->stopFlag) {
            break;
        }

        if (plRenderWindowImage(pApplication->pWindow)) {
            return 1;
        }
    }

    return 0;
}
