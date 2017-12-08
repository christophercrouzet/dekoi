#ifndef DEKOI_PLAYGROUND_TEST_LOGGING_H
#define DEKOI_PLAYGROUND_TEST_LOGGING_H

#include "test.h"

#include <dekoi/logging>

#if defined(PL_LOG_DEBUG)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_DEBUG
#elif defined(PL_LOG_INFO)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_INFO
#elif defined(PL_LOG_WARNING)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_WARNING
#elif defined(PL_LOG_ERROR)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_ERROR
#elif defined(PL_DEBUG)
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_WARNING
#else
#define PL_LOGGING_LEVEL PL_LOG_LEVEL_ERROR
#endif

#define PL_LOG(pLogger, level, args) \
    do { \
        if (level >= PL_LOGGING_LEVEL) { \
            (pLogger)->pfnLog args; \
        } \
    } while (0)

#define PL_DEBUG_0(pLogger, pFormat) \
    PL_LOG( \
        pLogger, \
        PL_LOG_LEVEL_DEBUG, \
        ((pLogger)->pData, PL_LOG_LEVEL_DEBUG, __FILE__, __LINE__, pFormat))

#define PL_DEBUG_1(pLogger, pFormat, _1) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1))

#define PL_DEBUG_2(pLogger, pFormat, _1, _2) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2))

#define PL_DEBUG_3(pLogger, pFormat, _1, _2, _3) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3))

#define PL_DEBUG_4(pLogger, pFormat, _1, _2, _3, _4) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4))

#define PL_DEBUG_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5))

#define PL_DEBUG_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6))

#define PL_DEBUG_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7))

#define PL_DEBUG_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_DEBUG, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_DEBUG, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7, \
            _8))

#define PL_INFO_0(pLogger, pFormat) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, PL_LOG_LEVEL_INFO, __FILE__, __LINE__, pFormat))

#define PL_INFO_1(pLogger, pFormat, _1) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1))

#define PL_INFO_2(pLogger, pFormat, _1, _2) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2))

#define PL_INFO_3(pLogger, pFormat, _1, _2, _3) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3))

#define PL_INFO_4(pLogger, pFormat, _1, _2, _3, _4) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4))

#define PL_INFO_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5))

#define PL_INFO_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6))

#define PL_INFO_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7))

#define PL_INFO_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_INFO, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_INFO, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7, \
            _8))

#define PL_WARNING_0(pLogger, pFormat) \
    PL_LOG( \
        pLogger, \
        PL_LOG_LEVEL_WARNING, \
        ((pLogger)->pData, PL_LOG_LEVEL_WARNING, __FILE__, __LINE__, pFormat))

#define PL_WARNING_1(pLogger, pFormat, _1) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1))

#define PL_WARNING_2(pLogger, pFormat, _1, _2) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2))

#define PL_WARNING_3(pLogger, pFormat, _1, _2, _3) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3))

#define PL_WARNING_4(pLogger, pFormat, _1, _2, _3, _4) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4))

#define PL_WARNING_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5))

#define PL_WARNING_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6))

#define PL_WARNING_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7))

#define PL_WARNING_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_WARNING, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_WARNING, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7, \
            _8))

#define PL_ERROR_0(pLogger, pFormat) \
    PL_LOG( \
        pLogger, \
        PL_LOG_LEVEL_ERROR, \
        ((pLogger)->pData, PL_LOG_LEVEL_ERROR, __FILE__, __LINE__, pFormat))

#define PL_ERROR_1(pLogger, pFormat, _1) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1))

#define PL_ERROR_2(pLogger, pFormat, _1, _2) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2))

#define PL_ERROR_3(pLogger, pFormat, _1, _2, _3) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3))

#define PL_ERROR_4(pLogger, pFormat, _1, _2, _3, _4) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4))

#define PL_ERROR_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5))

#define PL_ERROR_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6))

#define PL_ERROR_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7))

#define PL_ERROR_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    PL_LOG(pLogger, \
           PL_LOG_LEVEL_ERROR, \
           ((pLogger)->pData, \
            PL_LOG_LEVEL_ERROR, \
            __FILE__, \
            __LINE__, \
            pFormat, \
            _1, \
            _2, \
            _3, \
            _4, \
            _5, \
            _6, \
            _7, \
            _8))

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
plCreateDekoiLoggingCallbacks(const PlLoggingCallbacks *pLogger,
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
