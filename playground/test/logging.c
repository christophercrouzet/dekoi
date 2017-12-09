#include "logging.h"

#include "test.h"

#include <dekoi/logging>

#ifdef PL_PLATFORM_UNIX
/* Request the POSIX.1 standard features before including standard headers. */
#define _POSIX_C_SOURCE 1
#include <unistd.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum PlAnsiColor {
    PL_ANSI_COLOR_RESET = 0,
    PL_ANSI_COLOR_BLACK = 1,
    PL_ANSI_COLOR_RED = 2,
    PL_ANSI_COLOR_GREEN = 3,
    PL_ANSI_COLOR_YELLOW = 4,
    PL_ANSI_COLOR_BLUE = 5,
    PL_ANSI_COLOR_MAGENTA = 6,
    PL_ANSI_COLOR_CYAN = 7,
    PL_ANSI_COLOR_BRIGHT_BLACK = 8,
    PL_ANSI_COLOR_BRIGHT_RED = 9,
    PL_ANSI_COLOR_BRIGHT_GREEN = 10,
    PL_ANSI_COLOR_BRIGHT_YELLOW = 11,
    PL_ANSI_COLOR_BRIGHT_BLUE = 12,
    PL_ANSI_COLOR_BRIGHT_MAGENTA = 13,
    PL_ANSI_COLOR_BRIGHT_CYAN = 14,
    PL_ANSI_COLOR_ENUM_LAST = PL_ANSI_COLOR_BRIGHT_CYAN,
    PL_ANSI_COLOR_ENUM_COUNT = PL_ANSI_COLOR_ENUM_LAST + 1
} PlAnsiColor;

static const char *
plGetLogLevelString(PlLogLevel level)
{
    switch (level) {
        case PL_LOG_LEVEL_DEBUG:
            return "debug";
        case PL_LOG_LEVEL_INFO:
            return "info";
        case PL_LOG_LEVEL_WARNING:
            return "warning";
        case PL_LOG_LEVEL_ERROR:
            return "error";
        default:
            return "invalid";
    }
}

static PlAnsiColor
plGetLogLevelColor(PlLogLevel level)
{
    switch (level) {
        case PL_LOG_LEVEL_DEBUG:
            return PL_ANSI_COLOR_BRIGHT_MAGENTA;
        case PL_LOG_LEVEL_INFO:
            return PL_ANSI_COLOR_BRIGHT_CYAN;
        case PL_LOG_LEVEL_WARNING:
            return PL_ANSI_COLOR_BRIGHT_YELLOW;
        case PL_LOG_LEVEL_ERROR:
            return PL_ANSI_COLOR_BRIGHT_RED;
        default:
            assert(0);
            return PL_ANSI_COLOR_RESET;
    };
}

static const char *
plGetAnsiColorString(PlAnsiColor color)
{
    switch (color) {
        case PL_ANSI_COLOR_RESET:
            return "\x1b[0m";
        case PL_ANSI_COLOR_BLACK:
            return "\x1b[30m";
        case PL_ANSI_COLOR_RED:
            return "\x1b[31m";
        case PL_ANSI_COLOR_GREEN:
            return "\x1b[32m";
        case PL_ANSI_COLOR_YELLOW:
            return "\x1b[33m";
        case PL_ANSI_COLOR_BLUE:
            return "\x1b[34m";
        case PL_ANSI_COLOR_MAGENTA:
            return "\x1b[35m";
        case PL_ANSI_COLOR_CYAN:
            return "\x1b[36m";
        case PL_ANSI_COLOR_BRIGHT_BLACK:
            return "\x1b[1;30m";
        case PL_ANSI_COLOR_BRIGHT_RED:
            return "\x1b[1;31m";
        case PL_ANSI_COLOR_BRIGHT_GREEN:
            return "\x1b[1;32m";
        case PL_ANSI_COLOR_BRIGHT_YELLOW:
            return "\x1b[1;33m";
        case PL_ANSI_COLOR_BRIGHT_BLUE:
            return "\x1b[1;34m";
        case PL_ANSI_COLOR_BRIGHT_MAGENTA:
            return "\x1b[1;35m";
        case PL_ANSI_COLOR_BRIGHT_CYAN:
            return "\x1b[1;36m";
        default:
            assert(0);
            return "";
    }
}

static void
plLogHelper(void *pData,
            PlLogLevel level,
            const char *pFile,
            int line,
            const char *pFormat,
            va_list args)
{
    const char *pLevelName;
    const char *pLevelColorStart;
    const char *pLevelColorEnd;

    assert(pFile != NULL);
    assert(pFormat != NULL);

    PL_UNUSED(pData);

    pLevelName = plGetLogLevelString(level);

#ifdef PL_PLATFORM_UNIX
    if (isatty(fileno(stderr))) {
        PlAnsiColor levelColor;

        levelColor = plGetLogLevelColor(level);
        pLevelColorStart = plGetAnsiColorString(levelColor);
        pLevelColorEnd = plGetAnsiColorString(PL_ANSI_COLOR_RESET);
    } else {
        pLevelColorStart = pLevelColorEnd = "";
    }
#else
    pLevelColorStart = pLevelColorEnd = "";
#endif

    fprintf(stderr,
            "%s:%d: %s%s%s: ",
            pFile,
            line,
            pLevelColorStart,
            pLevelName,
            pLevelColorEnd);
    vfprintf(stderr, pFormat, args);
}

static PlLogLevel
plInterpretDekoiLogLevel(DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            return PL_LOG_LEVEL_DEBUG;
        case DK_LOG_LEVEL_INFO:
            return PL_LOG_LEVEL_INFO;
        case DK_LOG_LEVEL_WARNING:
            return PL_LOG_LEVEL_WARNING;
        case DK_LOG_LEVEL_ERROR:
            return PL_LOG_LEVEL_ERROR;
        default:
            assert(0);
            return PL_LOG_LEVEL_DEBUG;
    };
}

static void
plDekoiLogCallback(void *pData,
                   DkLogLevel level,
                   const char *pFile,
                   int line,
                   const char *pFormat,
                   ...)
{
    va_list args;

    va_start(args, pFormat);
    plLogHelper(
        pData, plInterpretDekoiLogLevel(level), pFile, line, pFormat, args);
    va_end(args);
}

static const PlLoggingCallbacks plDefaultLogger = {NULL, plLog};

void
plGetDefaultLogger(const PlLoggingCallbacks **ppLogger)
{
    assert(ppLogger != NULL);

    *ppLogger = &plDefaultLogger;
}

int
plCreateDekoiLoggingCallbacks(const PlLoggingCallbacks *pLogger,
                              DkLoggingCallbacks **ppDekoiLogger)
{
    int out;
    PlDekoiLoggingCallbacksData *pLoggerData;

    assert(pLogger != NULL);
    assert(ppDekoiLogger != NULL);

    out = 0;

    *ppDekoiLogger = (DkLoggingCallbacks *)malloc(sizeof **ppDekoiLogger);
    if (*ppDekoiLogger == NULL) {
        PL_ERROR_0(pLogger, "failed to allocate Dekoi's logging callbacks\n");
        out = 1;
        goto exit;
    }

    pLoggerData = (PlDekoiLoggingCallbacksData *)malloc(sizeof *pLoggerData);
    if (pLoggerData == NULL) {
        PL_ERROR_0(pLogger,
                   "failed to allocate Dekoi's logging callbacks data\n");
        out = 1;
        goto dekoi_logger_undo;
    }

    pLoggerData->pLogger = pLogger;

    (*ppDekoiLogger)->pData = pLoggerData;
    (*ppDekoiLogger)->pfnLog = plDekoiLogCallback;

    goto exit;

dekoi_logger_undo:
    free(*ppDekoiLogger);

exit:
    return out;
}

void
plDestroyDekoiLoggingCallbacks(DkLoggingCallbacks *pDekoiLogger)
{
    if (pDekoiLogger == NULL) {
        return;
    }

    free(pDekoiLogger->pData);
    free(pDekoiLogger);
}

void
plLog(void *pData,
      PlLogLevel level,
      const char *pFile,
      int line,
      const char *pFormat,
      ...)
{
    va_list args;

    va_start(args, pFormat);
    plLogHelper(pData, level, pFile, line, pFormat, args);
    va_end(args);
}
