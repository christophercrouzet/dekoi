#include "logger.h"

#include "assert.h"
#include "common.h"

#include "../common.h"
#include "../logger.h"

#include <stdarg.h>

#define ZR_STATIC
#define ZR_ASSERT DKP_ASSERT
#define ZR_IMPLEMENTATION
#include <zero/logger.h>

static ZrLogLevel
dkpTranslateLogLevelToZero(DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            return ZR_LOG_LEVEL_DEBUG;
        case DK_LOG_LEVEL_INFO:
            return ZR_LOG_LEVEL_INFO;
        case DK_LOG_LEVEL_WARNING:
            return ZR_LOG_LEVEL_WARNING;
        case DK_LOG_LEVEL_ERROR:
            return ZR_LOG_LEVEL_ERROR;
        default:
            DKP_ASSERT(0);
            return ZR_LOG_LEVEL_DEBUG;
    };
}

static void
dkpLogVaList(void *pData,
             DkLogLevel level,
             const char *pFile,
             int line,
             const char *pFormat,
             va_list args)
{
    DKP_UNUSED(pData);

    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFormat != NULL);

    zrLogVaList(dkpTranslateLogLevelToZero(level), pFile, line, pFormat, args);
}

static void
dkpLog(void *pData,
       DkLogLevel level,
       const char *pFile,
       int line,
       const char *pFormat,
       ...)
{
    va_list args;

    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFormat != NULL);

    va_start(args, pFormat);
    dkpLogVaList(pData, level, pFile, line, pFormat, args);
    va_end(args);
}

static const DkLoggingCallbacks dkpDefaultLogger = {NULL, dkpLog, dkpLogVaList};

void
dkpGetDefaultLogger(const DkLoggingCallbacks **ppLogger)
{
    DKP_ASSERT(ppLogger != NULL);

    *ppLogger = &dkpDefaultLogger;
}
