#include <stdlib.h>

#include "application.h"
#include "window.h"


const unsigned int width = 1280;
const unsigned int height = 720;
const char *applicationName = "dekoi";


void
setup(Application *pApplication,
      Window *pWindow)
{
    ApplicationCreateInfo applicationCreateInfo;
    WindowCreateInfo windowCreateInfo;

    applicationCreateInfo.name = applicationName;
    createApplication(&applicationCreateInfo, pApplication);

    windowCreateInfo.width = width;
    windowCreateInfo.height = height;
    windowCreateInfo.title = applicationName;
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
