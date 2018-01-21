#include "logger.h"

#include "assert.h"
#include "common.h"

#include "../common.h"
#include "../logger.h"

#include <stdarg.h>
#include <stdio.h>

static void
dkpLogVaList(void *pData,
             DkLogLevel level,
             const char *pFile,
             int line,
             const char *pFormat,
             va_list args)
{
    const char *pLevelName;

    DKP_UNUSED(pData);

    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFormat != NULL);

    pLevelName = dkGetLogLevelString(level);

    fprintf(stderr, "%s:%d: %s: ", pFile, line, pLevelName);
    vfprintf(stderr, pFormat, args);
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
