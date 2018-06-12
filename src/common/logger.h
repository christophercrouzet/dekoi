#ifndef DEKOI_COMMON_LOGGER_H
#define DEKOI_COMMON_LOGGER_H

#include "common.h"

#include <stdarg.h>

enum DkLogLevel {
    DK_LOG_LEVEL_DEBUG = 0,
    DK_LOG_LEVEL_INFO = 1,
    DK_LOG_LEVEL_WARNING = 2,
    DK_LOG_LEVEL_ERROR = 3
} DkLogLevel;

typedef void (*DkPfnLogCallback)(void *pData,
                                 enum DkLogLevel level,
                                 const char *pFile,
                                 int line,
                                 const char *pFormat,
                                 ...);
typedef void (*DkPfnLogVaListCallback)(void *pData,
                                       enum DkLogLevel level,
                                       const char *pFile,
                                       int line,
                                       const char *pFormat,
                                       va_list args);

struct DkLoggingCallbacks {
    void *pData;
    DkPfnLogCallback pfnLog;
    DkPfnLogVaListCallback pfnLogVaList;
};

const char *
dkGetLogLevelString(enum DkLogLevel level);

#endif /* DEKOI_COMMON_LOGGER_H */
