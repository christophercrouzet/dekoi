#include "logger.h"

#include "allocator.h"
#include "common.h"

#include <dekoi/common/logger.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define ZR_SPECIFY_INTERNAL_LINKAGE
#define ZR_DEFINE_IMPLEMENTATION
#define ZR_ASSERT assert
#include <zero/logger.h>

static enum ZrLogLevel
dkdTranslateLogLevelToZero(enum DkdLogLevel level)
{
    switch (level) {
        case DKD_LOG_LEVEL_DEBUG:
            return ZR_LOG_LEVEL_DEBUG;
        case DKD_LOG_LEVEL_INFO:
            return ZR_LOG_LEVEL_INFO;
        case DKD_LOG_LEVEL_WARNING:
            return ZR_LOG_LEVEL_WARNING;
        case DKD_LOG_LEVEL_ERROR:
            return ZR_LOG_LEVEL_ERROR;
        default:
            assert(0);
            return ZR_LOG_LEVEL_DEBUG;
    };
}

static void
dkdLogVaList(void *pData,
             enum DkdLogLevel level,
             const char *pFile,
             int line,
             const char *pFormat,
             va_list args)
{
    DKD_UNUSED(pData);

    assert(pFile != NULL);
    assert(pFormat != NULL);

    zrLogVaList(dkdTranslateLogLevelToZero(level), pFile, line, pFormat, args);
}

static void
dkdLog(void *pData,
       enum DkdLogLevel level,
       const char *pFile,
       int line,
       const char *pFormat,
       ...)
{
    va_list args;

    assert(pFile != NULL);
    assert(pFormat != NULL);

    va_start(args, pFormat);
    dkdLogVaList(pData, level, pFile, line, pFormat, args);
    va_end(args);
}

static const struct DkdLoggingCallbacks dkdDefaultLogger
    = {NULL, dkdLog, dkdLogVaList};

static enum DkdLogLevel
dkdTranslateLogLevelFromDekoi(enum DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            return DKD_LOG_LEVEL_DEBUG;
        case DK_LOG_LEVEL_INFO:
            return DKD_LOG_LEVEL_INFO;
        case DK_LOG_LEVEL_WARNING:
            return DKD_LOG_LEVEL_WARNING;
        case DK_LOG_LEVEL_ERROR:
            return DKD_LOG_LEVEL_ERROR;
        default:
            assert(0);
            return DKD_LOG_LEVEL_DEBUG;
    };
}

static void
dkdHandleDekoiLoggingVaList(void *pData,
                            enum DkLogLevel level,
                            const char *pFile,
                            int line,
                            const char *pFormat,
                            va_list args)
{
    assert(pData != NULL);
    assert(pFile != NULL);
    assert(pFormat != NULL);

    ((struct DkdDekoiLoggingCallbacksData *)pData)
        ->pLogger->pfnLogVaList(
            ((struct DkdDekoiLoggingCallbacksData *)pData)->pLogger->pData,
            dkdTranslateLogLevelFromDekoi(level),
            pFile,
            line,
            pFormat,
            args);
}

static void
dkdHandleDekoiLogging(void *pData,
                      enum DkLogLevel level,
                      const char *pFile,
                      int line,
                      const char *pFormat,
                      ...)
{
    va_list args;

    assert(pData != NULL);
    assert(pFile != NULL);
    assert(pFormat != NULL);

    va_start(args, pFormat);
    dkdHandleDekoiLoggingVaList(pData, level, pFile, line, pFormat, args);
    va_end(args);
}

void
dkdGetDefaultLogger(const struct DkdLoggingCallbacks **ppLogger)
{
    assert(ppLogger != NULL);

    *ppLogger = &dkdDefaultLogger;
}

int
dkdCreateDekoiLoggingCallbacks(struct DkLoggingCallbacks **ppDekoiLogger,
                               struct DkdDekoiLoggingCallbacksData *pData,
                               const struct DkdAllocationCallbacks *pAllocator,
                               const struct DkdLoggingCallbacks *pLogger)
{
    assert(ppDekoiLogger != NULL);
    assert(pData != NULL);
    assert(pAllocator != NULL);
    assert(pLogger != NULL);

    *ppDekoiLogger = (struct DkLoggingCallbacks *)DKD_ALLOCATE(
        pAllocator, sizeof **ppDekoiLogger);
    if (*ppDekoiLogger == NULL) {
        DKD_LOG_ERROR(pLogger,
                      "failed to allocate Dekoi's logging callbacks\n");
        return 1;
    }

    (*ppDekoiLogger)->pData = pData;
    (*ppDekoiLogger)->pfnLog = dkdHandleDekoiLogging;
    (*ppDekoiLogger)->pfnLogVaList = dkdHandleDekoiLoggingVaList;
    return 0;
}

void
dkdDestroyDekoiLoggingCallbacks(struct DkLoggingCallbacks *pDekoiLogger,
                                const struct DkdAllocationCallbacks *pAllocator)
{
    assert(pAllocator != NULL);

    DKD_FREE(pAllocator, pDekoiLogger);
}
