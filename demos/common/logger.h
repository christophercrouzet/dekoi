#ifndef DEKOI_DEMOS_COMMON_LOGGER_H
#define DEKOI_DEMOS_COMMON_LOGGER_H

#include "common.h"

#include <dekoi/common/logger>

#if defined(DKD_ENABLE_DEBUG_LOGGING)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_DEBUG
#elif defined(DKD_ENABLE_INFO_LOGGING)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_INFO
#elif defined(DKD_ENABLE_WARNING_LOGGING)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_WARNING
#elif defined(DKD_ENABLE_ERROR_LOGGING)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_ERROR
#elif defined(DKD_DEBUG)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_WARNING
#else
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_ERROR
#endif

#define DKD_LOG(pLogger, level, ...)                                           \
    do {                                                                       \
        if (level >= DKD_LOGGING_LEVEL) {                                      \
            (pLogger)->pfnLog(                                                 \
                (pLogger)->pData, level, __FILE__, __LINE__, __VA_ARGS__);     \
        }                                                                      \
    } while (0)

#define DKD_LOG_DEBUG(pLogger, ...)                                            \
    DKD_LOG(pLogger, DKD_LOG_LEVEL_DEBUG, __VA_ARGS__)

#define DKD_LOG_INFO(pLogger, ...)                                             \
    DKD_LOG(pLogger, DKD_LOG_LEVEL_INFO, __VA_ARGS__)

#define DKD_LOG_WARNING(pLogger, ...)                                          \
    DKD_LOG(pLogger, DKD_LOG_LEVEL_WARNING, __VA_ARGS__)

#define DKD_LOG_ERROR(pLogger, ...)                                            \
    DKD_LOG(pLogger, DKD_LOG_LEVEL_ERROR, __VA_ARGS__)

typedef enum DkdLogLevel {
    DKD_LOG_LEVEL_DEBUG = 0,
    DKD_LOG_LEVEL_INFO = 1,
    DKD_LOG_LEVEL_WARNING = 2,
    DKD_LOG_LEVEL_ERROR = 3
} DkdLogLevel;

typedef void (*DkdPfnLogCallback)(void *pData,
                                  DkdLogLevel level,
                                  const char *pFile,
                                  int line,
                                  const char *pFormat,
                                  ...);

struct DkdLoggingCallbacks {
    void *pData;
    DkdPfnLogCallback pfnLog;
};

struct DkdDekoiLoggingCallbacksData {
    const DkdLoggingCallbacks *pLogger;
};

void
dkdGetDefaultLogger(const DkdLoggingCallbacks **ppLogger);

int
dkdCreateDekoiLoggingCallbacks(DkdDekoiLoggingCallbacksData *pData,
                               const DkdLoggingCallbacks *pLogger,
                               DkLoggingCallbacks **ppDekoiLogger);

void
dkdDestroyDekoiLoggingCallbacks(DkLoggingCallbacks *pDekoiLogger);

#endif /* DEKOI_DEMOS_COMMON_LOGGER_H */
