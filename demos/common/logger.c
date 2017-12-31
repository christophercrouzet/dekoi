#include "logger.h"

#include "common.h"

#include <dekoi/logging>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void
dkdLog(void *pData,
       DkdLogLevel level,
       const char *pFile,
       int line,
       const char *pFormat,
       ...)
{
    va_list args;

    DKD_UNUSED(pData);
    DKD_UNUSED(level);
    DKD_UNUSED(pFile);
    DKD_UNUSED(line);

    va_start(args, pFormat);
    vfprintf(stderr, pFormat, args);
    va_end(args);
}

static const DkdLoggingCallbacks dkdDefaultLogger = {NULL, dkdLog};

static DkdLogLevel
dkdInterpretDekoiLogLevel(DkLogLevel level)
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
dkdHandleDekoiLogging(void *pData,
                      DkLogLevel level,
                      const char *pFile,
                      int line,
                      const char *pFormat,
                      ...)
{
    va_list args;

    va_start(args, pFormat);
    ((DkdDekoiLoggingCallbacksData *)pData)
        ->pLogger->pfnLog(
            ((DkdDekoiLoggingCallbacksData *)pData)->pLogger->pData,
            dkdInterpretDekoiLogLevel(level),
            pFile,
            line,
            pFormat,
            args);
    va_end(args);
}

void
dkdGetDefaultLogger(const DkdLoggingCallbacks **ppLogger)
{
    assert(ppLogger != NULL);

    *ppLogger = &dkdDefaultLogger;
}

int
dkdCreateDekoiLoggingCallbacks(DkdDekoiLoggingCallbacksData *pData,
                               const DkdLoggingCallbacks *pLogger,
                               DkLoggingCallbacks **ppDekoiLogger)
{
    *ppDekoiLogger = NULL;

    assert(pData != NULL);
    assert(pLogger != NULL);
    assert(ppDekoiLogger != NULL);

    *ppDekoiLogger = (DkLoggingCallbacks *)malloc(sizeof **ppDekoiLogger);
    if (*ppDekoiLogger == NULL) {
        DKD_LOG_ERROR(pLogger,
                      "failed to allocate Dekoi's logging callbacks\n");
        return 1;
    }

    (*ppDekoiLogger)->pData = pData;
    (*ppDekoiLogger)->pfnLog = dkdHandleDekoiLogging;
    return 0;
}

void
dkdDestroyDekoiLoggingCallbacks(DkLoggingCallbacks *pDekoiLogger)
{
    free(pDekoiLogger);
}
