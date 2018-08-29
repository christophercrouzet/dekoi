#include "logger.h"

#include "assert.h"
#include "common.h"

#include "../common.h"
#include "../logger.h"

#include <stdarg.h>
#include <stddef.h>

#define ZR_SPECIFY_INTERNAL_LINKAGE
#define ZR_DEFINE_IMPLEMENTATION
#define ZR_ASSERT DKP_ASSERT
#include <zero/logger.h>

static void
dkpTranslateLogLevelToZero(enum ZrLogLevel *pZeroLevel, enum DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            *pZeroLevel = ZR_LOG_LEVEL_DEBUG;
            return;
        case DK_LOG_LEVEL_INFO:
            *pZeroLevel = ZR_LOG_LEVEL_INFO;
            return;
        case DK_LOG_LEVEL_WARNING:
            *pZeroLevel = ZR_LOG_LEVEL_WARNING;
            return;
        case DK_LOG_LEVEL_ERROR:
            *pZeroLevel = ZR_LOG_LEVEL_ERROR;
            return;
        default:
            DKP_ASSERT(0);
            *pZeroLevel = ZR_LOG_LEVEL_DEBUG;
    };
}

static void
dkpLogVaList(void *pData,
             enum DkLogLevel level,
             const char *pFile,
             int line,
             const char *pFormat,
             va_list args)
{
    enum ZrLogLevel zeroLevel;

    DKP_UNUSED(pData);

    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFormat != NULL);

    dkpTranslateLogLevelToZero(&zeroLevel, level);
    zrLogVaList(zeroLevel, pFile, line, pFormat, args);
}

static void
dkpLog(void *pData,
       enum DkLogLevel level,
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

static const struct DkLoggingCallbacks dkpDefaultLogger
    = {NULL, dkpLog, dkpLogVaList};

void
dkpGetDefaultLogger(const struct DkLoggingCallbacks **ppLogger)
{
    DKP_ASSERT(ppLogger != NULL);

    *ppLogger = &dkpDefaultLogger;
}
