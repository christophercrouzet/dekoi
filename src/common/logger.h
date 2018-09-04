#ifndef DEKOI_COMMON_LOGGER_H
#define DEKOI_COMMON_LOGGER_H

#include "common.h"

#include <stdarg.h>

enum DkLogLevel {
    DK_LOG_LEVEL_ERROR = 0,
    DK_LOG_LEVEL_WARNING = 1,
    DK_LOG_LEVEL_INFO = 2,
    DK_LOG_LEVEL_TRACE = 3,
    DK_LOG_LEVEL_DEBUG = 4
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

void
dkGetLogLevelName(const char **ppName, enum DkLogLevel level);

#endif /* DEKOI_COMMON_LOGGER_H */
