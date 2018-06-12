#ifndef DEKOI_DEMOS_COMMON_APPLICATION_H
#define DEKOI_DEMOS_COMMON_APPLICATION_H

struct DkdApplication;
struct DkdWindow;

typedef void (*DkdPfnRunCallback)(struct DkdApplication *pApplication,
                                  void *pData);

struct DkdApplicationCallbacks {
    void *pData;
    DkdPfnRunCallback pfnRun;
};

struct DkdApplicationCreateInfo {
    const char *pName;
    unsigned int majorVersion;
    unsigned int minorVersion;
    unsigned int patchVersion;
    const struct DkdLoggingCallbacks *pLogger;
    const struct DkdAllocationCallbacks *pAllocator;
    const struct DkdApplicationCallbacks *pCallbacks;
};

int
dkdCreateApplication(const struct DkdApplicationCreateInfo *pCreateInfo,
                     struct DkdApplication **ppApplication);

void
dkdDestroyApplication(struct DkdApplication *pApplication);

int
dkdBindApplicationWindow(struct DkdApplication *pApplication,
                         struct DkdWindow *pWindow);

int
dkdRunApplication(struct DkdApplication *pApplication);

#endif /* DEKOI_DEMOS_COMMON_APPLICATION_H */
