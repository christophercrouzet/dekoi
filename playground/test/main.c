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
    ApplicationCreateInfo applicationCreateInfo;
    WindowCreateInfo windowCreateInfo;

    memset(&applicationCreateInfo, 0, sizeof(ApplicationCreateInfo));
    applicationCreateInfo.pName = pApplicationName;
    applicationCreateInfo.majorVersion = majorVersion;
    applicationCreateInfo.minorVersion = minorVersion;
    applicationCreateInfo.patchVersion = patchVersion;
    createApplication(&applicationCreateInfo, pApplication);

    memset(&windowCreateInfo, 0, sizeof(WindowCreateInfo));
    windowCreateInfo.width = width;
    windowCreateInfo.height = height;
    windowCreateInfo.title = pApplicationName;
    createWindow(pApplication, &windowCreateInfo, pWindow);
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
