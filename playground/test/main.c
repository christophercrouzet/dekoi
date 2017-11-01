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


void
setup(Application *pApplication,
      Window *pWindow)
{
    ApplicationCreateInfo applicationInfo;
    WindowCreateInfo windowInfo;

    memset(&applicationInfo, 0, sizeof(ApplicationCreateInfo));
    applicationInfo.pName = pApplicationName;
    applicationInfo.majorVersion = majorVersion;
    applicationInfo.minorVersion = minorVersion;
    applicationInfo.patchVersion = patchVersion;
    createApplication(&applicationInfo, pApplication);

    memset(&windowInfo, 0, sizeof(WindowCreateInfo));
    windowInfo.width = width;
    windowInfo.height = height;
    windowInfo.title = pApplicationName;
    createWindow(pApplication, &windowInfo, pWindow);
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
    Application application;
    Window window;

    setup(&application, &window);
    runApplication(&application);
    cleanup(&application, &window);
    return EXIT_SUCCESS;
}
