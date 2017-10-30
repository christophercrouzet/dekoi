#ifndef DEKOI_PLAYGROUND_TEST_APPLICATION_H
#define DEKOI_PLAYGROUND_TEST_APPLICATION_H

#include "test.h"

struct Application {
    const char *name;
    Window *pWindow;
    int stopFlag;
};

struct ApplicationCreateInfo {
    const char *name;
};

void createApplication(const ApplicationCreateInfo *pCreateInfo,
                       Application *pApplication);
void destroyApplication(Application *pApplication);
int runApplication(Application *pApplication);

#endif /* DEKOI_PLAYGROUND_TEST_APPLICATION_H */
