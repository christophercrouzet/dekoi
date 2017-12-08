#ifndef DEKOI_LOGGING_H
#define DEKOI_LOGGING_H

#include "dekoi.h"

typedef enum DkLogLevel {
    DK_LOG_LEVEL_DEBUG = 0,
    DK_LOG_LEVEL_INFO = 1,
    DK_LOG_LEVEL_WARNING = 2,
    DK_LOG_LEVEL_ERROR = 3
} DkLogLevel;

typedef void (*DkPfnLogCallback)(void *pData,
                                 DkLogLevel level,
                                 const char *pFile,
                                 int line,
                                 const char *pFormat,
                                 ...);

struct DkLoggingCallbacks {
    void *pData;
    DkPfnLogCallback pfnLog;
};

const char *
dkGetLogLevelString(DkLogLevel level);

#endif /* DEKOI_LOGGING_H */
