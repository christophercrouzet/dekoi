#ifndef DEKOI_PLAYGROUND_TEST_APPLICATION_H
#define DEKOI_PLAYGROUND_TEST_APPLICATION_H

#include "test.h"

struct PlApplicationCreateInfo {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
};

int
plCreateApplication(const PlApplicationCreateInfo *pCreateInfo,
                    PlApplication **ppApplication);

void
plDestroyApplication(PlApplication *pApplication);

int
plBindApplicationWindow(PlApplication *pApplication, PlWindow *pWindow);

int
plRunApplication(PlApplication *pApplication);

#endif /* DEKOI_PLAYGROUND_TEST_APPLICATION_H */
