#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "application.h"
#include "window.h"


struct Application {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    Window *pWindow;
    int stopFlag;
};


int
createApplication(const ApplicationCreateInfo *pCreateInfo,
                  Application **ppApplication)
{
    assert(pCreateInfo != NULL);
    assert(ppApplication != NULL);

    *ppApplication = (Application *) malloc(sizeof **ppApplication);
    if (*ppApplication == NULL) {
        fprintf(stderr, "failed to allocate the application\n");
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
destroyApplication(Application *pApplication)
{
    free(pApplication);
}


int
bindApplicationWindow(Application *pApplication,
                      Window *pWindow)
{
    assert(pApplication != NULL);

    pApplication->pWindow = pWindow;
    return 0;
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
