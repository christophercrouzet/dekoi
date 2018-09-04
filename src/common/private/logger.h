#ifndef DEKOI_COMMON_PRIVATE_LOGGER_H
#define DEKOI_COMMON_PRIVATE_LOGGER_H

#include "common.h"

#include "../common.h"
#include "../logger.h"

#if defined(DK_SET_LOGGING_LEVEL_DEBUG)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_DEBUG
#elif defined(DK_SET_LOGGING_LEVEL_TRACE)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_TRACE
#elif defined(DK_SET_LOGGING_LEVEL_INFO)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_INFO
#elif defined(DK_SET_LOGGING_LEVEL_WARNING)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_WARNING
#elif defined(DK_SET_LOGGING_LEVEL_ERROR)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_ERROR
#elif defined(DK_ENABLE_DEBUGGING)                                             \
    || (!defined(DK_DISABLE_DEBUGGING)                                         \
        && (defined(DEBUG) || !defined(NDEBUG)))
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_DEBUG
#else
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_WARNING
#endif

#define DKP_LOG(pLogger, level, ...)                                           \
    do {                                                                       \
        if (level <= DKP_LOGGING_LEVEL) {                                      \
            (pLogger)->pfnLog(                                                 \
                (pLogger)->pData, level, __FILE__, __LINE__, __VA_ARGS__);     \
        }                                                                      \
    } while (0)

#define DKP_LOG_DEBUG(pLogger, ...)                                            \
    DKP_LOG(pLogger, DK_LOG_LEVEL_DEBUG, __VA_ARGS__)

#define DKP_LOG_TRACE(pLogger, ...)                                            \
    DKP_LOG(pLogger, DK_LOG_LEVEL_TRACE, __VA_ARGS__)

#define DKP_LOG_INFO(pLogger, ...)                                             \
    DKP_LOG(pLogger, DK_LOG_LEVEL_INFO, __VA_ARGS__)

#define DKP_LOG_WARNING(pLogger, ...)                                          \
    DKP_LOG(pLogger, DK_LOG_LEVEL_WARNING, __VA_ARGS__)

#define DKP_LOG_ERROR(pLogger, ...)                                            \
    DKP_LOG(pLogger, DK_LOG_LEVEL_ERROR, __VA_ARGS__)

void
dkpGetDefaultLogger(const struct DkLoggingCallbacks **ppLogger);

#endif /* DEKOI_COMMON_PRIVATE_LOGGER_H */
