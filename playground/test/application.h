#ifndef DEKOI_PLAYGROUND_TEST_APPLICATION_H
#define DEKOI_PLAYGROUND_TEST_APPLICATION_H

#include "test.h"

struct ApplicationCreateInfo {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
};

int createApplication(const ApplicationCreateInfo *pCreateInfo,
                      Application **ppApplication);
void destroyApplication(Application *pApplication);
int bindApplicationWindow(Application *pApplication,
                          Window *pWindow);
int runApplication(Application *pApplication);

#endif /* DEKOI_PLAYGROUND_TEST_APPLICATION_H */
