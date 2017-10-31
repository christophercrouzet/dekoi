#include <assert.h>
#include <stddef.h>

#include "application.h"
#include "window.h"


void
createApplication(const ApplicationCreateInfo *pCreateInfo,
                  Application *pApplication)
{
    assert(pCreateInfo != NULL);
    assert(pApplication != NULL);

    pApplication->pName = pCreateInfo->pName;
    pApplication->majorVersion = pCreateInfo->majorVersion;
    pApplication->minorVersion = pCreateInfo->minorVersion;
    pApplication->patchVersion = pCreateInfo->patchVersion;
    pApplication->pWindow = NULL;
}


void
destroyApplication(Application *pApplication)
{
    assert(pApplication != NULL);
}


int
runApplication(Application *pApplication)
{
    assert(pApplication != NULL);

    while (1) {
        pollWindowEvents(pApplication->pWindow);
        getWindowCloseFlag(pApplication->pWindow, &pApplication->stopFlag);
        if (pApplication->stopFlag)
            break;
    }

    return 0;
}
