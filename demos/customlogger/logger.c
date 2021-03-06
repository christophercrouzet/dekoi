#include "logger.h"

#include "../common/common.h"
#include "../common/logger.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

static void
dkdLogVaList(void *pData,
             enum DkdLogLevel level,
             const char *pFile,
             int line,
             const char *pFormat,
             va_list args)
{
    DKD_UNUSED(pData);
    DKD_UNUSED(level);
    DKD_UNUSED(pFile);
    DKD_UNUSED(line);

    assert(pFile != NULL);
    assert(pFormat != NULL);

    vfprintf(stderr, pFormat, args);
    printf("¯\\_(ツ)_/¯\n");
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

static const struct DkdLoggingCallbacks dkdCustomLogger
    = {NULL, dkdLog, dkdLogVaList};

void
dkdGetCustomLogger(const struct DkdLoggingCallbacks **ppLogger)
{
    assert(ppLogger != NULL);

    *ppLogger = &dkdCustomLogger;
}
