#ifndef DEKOI_PLAYGROUND_TEST_LOGGING_H
#define DEKOI_PLAYGROUND_TEST_LOGGING_H

#include "test.h"

#include <dekoi/logging>

#if defined(PL_ENABLE_DEBUG_LOGGING)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_DEBUG
#elif defined(PL_ENABLE_INFO_LOGGING)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_INFO
#elif defined(PL_ENABLE_WARNING_LOGGING)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_WARNING
#elif defined(PL_ENABLE_ERROR_LOGGING)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_ERROR
#elif defined(PL_DEBUG)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_WARNING
#else
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_ERROR
#endif

#define PL_LOG(pLogger, level, ...)                                            \
    do {                                                                       \
        if (level >= PL_LOGGING_LEVEL) {                                       \
            (pLogger)->pfnLog(                                                 \
                (pLogger)->pData, level, __FILE__, __LINE__, __VA_ARGS__);     \
        }                                                                      \
    } while (0)

#define PL_LOG_DEBUG(pLogger, ...)                                             \
    PL_LOG(pLogger, PL_LOG_LEVEL_DEBUG, __VA_ARGS__)

#define PL_LOG_INFO(pLogger, ...)                                              \
    PL_LOG(pLogger, PL_LOG_LEVEL_INFO, __VA_ARGS__)

#define PL_LOG_WARNING(pLogger, ...)                                           \
    PL_LOG(pLogger, PL_LOG_LEVEL_WARNING, __VA_ARGS__)

#define PL_LOG_ERROR(pLogger, ...)                                             \
    PL_LOG(pLogger, PL_LOG_LEVEL_ERROR, __VA_ARGS__)

typedef enum PlLogLevel {
    PL_LOG_LEVEL_DEBUG = 0,
    PL_LOG_LEVEL_INFO = 1,
    PL_LOG_LEVEL_WARNING = 2,
    PL_LOG_LEVEL_ERROR = 3
} PlLogLevel;

typedef void (*PlPfnLogCallback)(void *pData,
                                 PlLogLevel level,
                                 const char *pFile,
                                 int line,
                                 const char *pFormat,
                                 ...);

struct PlLoggingCallbacks {
    void *pData;
    PlPfnLogCallback pfnLog;
};

struct PlDekoiLoggingCallbacksData {
    const PlLoggingCallbacks *pLogger;
};

void
plGetDefaultLogger(const PlLoggingCallbacks **ppLogger);

int
plCreateDekoiLoggingCallbacks(PlDekoiLoggingCallbacksData *pData,
                              const PlLoggingCallbacks *pLogger,
                              DkLoggingCallbacks **ppDekoiLogger);

void
plDestroyDekoiLoggingCallbacks(DkLoggingCallbacks *pDekoiLogger);

void
plLog(void *pData,
      PlLogLevel level,
      const char *pFile,
      int line,
      const char *pFormat,
      ...);

#endif /* DEKOI_PLAYGROUND_TEST_LOGGING_H */
