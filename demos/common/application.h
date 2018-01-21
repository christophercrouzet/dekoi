#ifndef DEKOI_DEMOS_COMMON_APPLICATION_H
#define DEKOI_DEMOS_COMMON_APPLICATION_H

#include "common.h"

typedef void (*DkdPfnRunCallback)(DkdApplication *pApplication, void *pData);

struct DkdApplicationCallbacks {
    void *pData;
    DkdPfnRunCallback pfnRun;
};

struct DkdApplicationCreateInfo {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    const DkdLoggingCallbacks *pLogger;
    const DkdAllocationCallbacks *pAllocator;
    const DkdApplicationCallbacks *pCallbacks;
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
