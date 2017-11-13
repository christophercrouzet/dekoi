#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "window.h"


const char *pApplicationName = "dekoi";
const unsigned int majorVersion = 1;
const unsigned int minorVersion = 0;
const unsigned int patchVersion = 0;
const unsigned int width = 1280;
const unsigned int height = 720;


int
setup(Application **ppApplication,
      Window **ppWindow)
{
    int out;
    ApplicationCreateInfo applicationInfo;
    WindowCreateInfo windowInfo;

    assert(ppApplication != NULL);
    assert(ppWindow != NULL);

    out = 0;

    memset(&applicationInfo, 0, sizeof(applicationInfo));
    applicationInfo.pName = pApplicationName;
    applicationInfo.majorVersion = majorVersion;
    applicationInfo.minorVersion = minorVersion;
    applicationInfo.patchVersion = patchVersion;

    if (createApplication(&applicationInfo, ppApplication)) {
        out = 1;
        goto exit;
    }

    memset(&windowInfo, 0, sizeof(windowInfo));
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.title = pApplicationName;

    if (createWindow(*ppApplication, &windowInfo, ppWindow)) {
        out = 1;
        goto application_cleanup;
    }

    goto exit;

application_cleanup:
    destroyApplication(*ppApplication);

exit:
    return out;
}


void
cleanup(Application *pApplication,
        Window *pWindow)
{
    destroyWindow(pWindow);
    destroyApplication(pApplication);
}


int
main(void)
{
    int out;
    Application *pApplication;
    Window *pWindow;

    out = 0;

    if (setup(&pApplication, &pWindow)) {
        out = 1;
        goto exit;
    }

    if (runApplication(pApplication)) {
        out = 1;
        goto cleanup;
    }

cleanup:
    cleanup(pApplication, pWindow);

exit:
    return out;
}
