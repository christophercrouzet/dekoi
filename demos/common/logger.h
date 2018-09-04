#ifndef DEKOI_DEMOS_COMMON_LOGGER_H
#define DEKOI_DEMOS_COMMON_LOGGER_H

#include <dekoi/common/logger.h>

#include <stdarg.h>

#if defined(DKD_SET_LOGGING_LEVEL_DEBUG)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_DEBUG
#elif defined(DKD_SET_LOGGING_LEVEL_INFO)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_INFO
#elif defined(DKD_SET_LOGGING_LEVEL_WARNING)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_WARNING
#elif defined(DKD_SET_LOGGING_LEVEL_ERROR)
#define DKD_LOGGING_LEVEL DKD_LOG_LEVEL_ERROR
#elif defined(DKD_ENABLE_DEBUGGING)
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

enum DkdLogLevel {
    DKD_LOG_LEVEL_DEBUG = 0,
    DKD_LOG_LEVEL_INFO = 1,
    DKD_LOG_LEVEL_WARNING = 2,
    DKD_LOG_LEVEL_ERROR = 3
};

struct DkdAllocationCallbacks;

typedef void (*DkdPfnLogCallback)(void *pData,
                                  enum DkdLogLevel level,
                                  const char *pFile,
                                  int line,
                                  const char *pFormat,
                                  ...);
typedef void (*DkdPfnLogVaListCallback)(void *pData,
                                        enum DkdLogLevel level,
                                        const char *pFile,
                                        int line,
                                        const char *pFormat,
                                        va_list args);

struct DkdLoggingCallbacks {
    void *pData;
    DkdPfnLogCallback pfnLog;
    DkdPfnLogVaListCallback pfnLogVaList;
};

struct DkdDekoiLoggingCallbacksData {
    const struct DkdLoggingCallbacks *pLogger;
};

void
dkdGetDefaultLogger(const struct DkdLoggingCallbacks **ppLogger);

int
dkdCreateDekoiLoggingCallbacks(struct DkLoggingCallbacks **ppDekoiLogger,
                               struct DkdDekoiLoggingCallbacksData *pData,
                               const struct DkdAllocationCallbacks *pAllocator,
                               const struct DkdLoggingCallbacks *pLogger);

void
dkdDestroyDekoiLoggingCallbacks(
    struct DkLoggingCallbacks *pDekoiLogger,
    const struct DkdAllocationCallbacks *pAllocator);

#endif /* DEKOI_DEMOS_COMMON_LOGGER_H */
