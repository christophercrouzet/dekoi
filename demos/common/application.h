#ifndef DEKOI_DEMOS_COMMON_APPLICATION_H
#define DEKOI_DEMOS_COMMON_APPLICATION_H

#include "common.h"

struct DkdApplicationCreateInfo {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    const DkdLoggingCallbacks *pLogger;
    const DkdAllocationCallbacks *pAllocator;
};

int
dkdCreateApplication(const DkdApplicationCreateInfo *pCreateInfo,
                     DkdApplication **ppApplication);

void
dkdDestroyApplication(DkdApplication *pApplication);

int
dkdBindApplicationWindow(DkdApplication *pApplication, DkdWindow *pWindow);

int
dkdRunApplication(DkdApplication *pApplication);

#endif /* DEKOI_DEMOS_COMMON_APPLICATION_H */
