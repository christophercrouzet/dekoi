#include <assert.h>
#include <stddef.h>

#include "application.h"
#include "window.h"


int
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
    return 0;
}


void
destroyApplication(Application *pApplication)
{
    assert(pApplication != NULL);
    UNUSED(pApplication);
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
