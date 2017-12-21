#include "logging.h"

#include "assert.h"
#include "dekoi.h"

#include "../dekoi.h"
#include "../logging.h"

#include <stdarg.h>
#include <stdio.h>

static void
dkpLog(void *pData,
       DkLogLevel level,
       const char *pFile,
       int line,
       const char *pFormat,
       ...)
{
    const char *pLevelName;
    va_list args;

    DKP_UNUSED(pData);

    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFormat != NULL);

    pLevelName = dkGetLogLevelString(level);

    va_start(args, pFormat);
    fprintf(stderr, "%s:%d: %s: ", pFile, line, pLevelName);
    vfprintf(stderr, pFormat, args);
    va_end(args);
}

static const DkLoggingCallbacks dkpDefaultLogger = {NULL, dkpLog};

void
dkpGetDefaultLogger(const DkLoggingCallbacks **ppLogger)
{
    DKP_ASSERT(ppLogger != NULL);

    *ppLogger = &dkpDefaultLogger;
}
