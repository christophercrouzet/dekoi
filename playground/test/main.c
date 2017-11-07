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
setup(Application *pApplication,
      Window *pWindow)
{
    int out;
    ApplicationCreateInfo applicationInfo;
    WindowCreateInfo windowInfo;

    assert(pApplication != NULL);
    assert(pWindow != NULL);

    memset(&applicationInfo, 0, sizeof(ApplicationCreateInfo));
    applicationInfo.pName = pApplicationName;
    applicationInfo.majorVersion = majorVersion;
    applicationInfo.minorVersion = minorVersion;
    applicationInfo.patchVersion = patchVersion;

    if (createApplication(&applicationInfo, pApplication)) {
        return 1;
    }

    memset(&windowInfo, 0, sizeof(WindowCreateInfo));
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.title = pApplicationName;

    out = 0;
    if (createWindow(pApplication, &windowInfo, pWindow)) {
        out = 1;
        goto application_cleanup;
    }

    return out;

application_cleanup:
    destroyApplication(pApplication);
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
    Application application;
    Window window;

    if (setup(&application, &window)) {
        return EXIT_FAILURE;
    }

    out = EXIT_SUCCESS;
    if (runApplication(&application)) {
        out = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    cleanup(&application, &window);
    return out;
}
