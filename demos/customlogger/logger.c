#include "logger.h"

#include "../common/common.h"
#include "../common/logger.h"

#if !defined(_WIN32)                                                           \
    && (defined(__unix__) || defined(__unix)                                   \
        || (defined(__APPLE__) && defined(__MACH__)))
#define DKD_USE_COLOURS
#endif

#ifdef DKD_USE_COLOURS
/* Request the POSIX.1 standard features before including standard headers. */
#define _POSIX_C_SOURCE 1
#include <unistd.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

typedef enum DkdAnsiColor {
    DKD_ANSI_COLOR_RESET = 0,
    DKD_ANSI_COLOR_BLACK = 1,
    DKD_ANSI_COLOR_RED = 2,
    DKD_ANSI_COLOR_GREEN = 3,
    DKD_ANSI_COLOR_YELLOW = 4,
    DKD_ANSI_COLOR_BLUE = 5,
    DKD_ANSI_COLOR_MAGENTA = 6,
    DKD_ANSI_COLOR_CYAN = 7,
    DKD_ANSI_COLOR_BRIGHT_BLACK = 8,
    DKD_ANSI_COLOR_BRIGHT_RED = 9,
    DKD_ANSI_COLOR_BRIGHT_GREEN = 10,
    DKD_ANSI_COLOR_BRIGHT_YELLOW = 11,
    DKD_ANSI_COLOR_BRIGHT_BLUE = 12,
    DKD_ANSI_COLOR_BRIGHT_MAGENTA = 13,
    DKD_ANSI_COLOR_BRIGHT_CYAN = 14,
    DKD_ANSI_COLOR_ENUM_LAST = DKD_ANSI_COLOR_BRIGHT_CYAN,
    DKD_ANSI_COLOR_ENUM_COUNT = DKD_ANSI_COLOR_ENUM_LAST + 1
} DkdAnsiColor;

static const char *
dkdGetLogLevelString(DkdLogLevel level)
{
    switch (level) {
        case DKD_LOG_LEVEL_DEBUG:
            return "debug";
        case DKD_LOG_LEVEL_INFO:
            return "info";
        case DKD_LOG_LEVEL_WARNING:
            return "warning";
        case DKD_LOG_LEVEL_ERROR:
            return "error";
        default:
            return "invalid";
    }
}

static DkdAnsiColor
dkdGetLogLevelColor(DkdLogLevel level)
{
    switch (level) {
        case DKD_LOG_LEVEL_DEBUG:
            return DKD_ANSI_COLOR_BRIGHT_MAGENTA;
        case DKD_LOG_LEVEL_INFO:
            return DKD_ANSI_COLOR_BRIGHT_CYAN;
        case DKD_LOG_LEVEL_WARNING:
            return DKD_ANSI_COLOR_BRIGHT_YELLOW;
        case DKD_LOG_LEVEL_ERROR:
            return DKD_ANSI_COLOR_BRIGHT_RED;
        default:
            assert(0);
            return DKD_ANSI_COLOR_RESET;
    };
}

static const char *
dkdGetAnsiColorString(DkdAnsiColor color)
{
    switch (color) {
        case DKD_ANSI_COLOR_RESET:
            return "\x1b[0m";
        case DKD_ANSI_COLOR_BLACK:
            return "\x1b[30m";
        case DKD_ANSI_COLOR_RED:
            return "\x1b[31m";
        case DKD_ANSI_COLOR_GREEN:
            return "\x1b[32m";
        case DKD_ANSI_COLOR_YELLOW:
            return "\x1b[33m";
        case DKD_ANSI_COLOR_BLUE:
            return "\x1b[34m";
        case DKD_ANSI_COLOR_MAGENTA:
            return "\x1b[35m";
        case DKD_ANSI_COLOR_CYAN:
            return "\x1b[36m";
        case DKD_ANSI_COLOR_BRIGHT_BLACK:
            return "\x1b[1;30m";
        case DKD_ANSI_COLOR_BRIGHT_RED:
            return "\x1b[1;31m";
        case DKD_ANSI_COLOR_BRIGHT_GREEN:
            return "\x1b[1;32m";
        case DKD_ANSI_COLOR_BRIGHT_YELLOW:
            return "\x1b[1;33m";
        case DKD_ANSI_COLOR_BRIGHT_BLUE:
            return "\x1b[1;34m";
        case DKD_ANSI_COLOR_BRIGHT_MAGENTA:
            return "\x1b[1;35m";
        case DKD_ANSI_COLOR_BRIGHT_CYAN:
            return "\x1b[1;36m";
        default:
            assert(0);
            return "";
    }
}

static void
dkdLogVaList(void *pData,
             DkdLogLevel level,
             const char *pFile,
             int line,
             const char *pFormat,
             va_list args)
{
    const char *pLevelName;
    const char *pLevelColorStart;
    const char *pLevelColorEnd;

    DKD_UNUSED(pData);

    assert(pFile != NULL);
    assert(pFormat != NULL);

    pLevelName = dkdGetLogLevelString(level);

#ifdef DKD_USE_COLOURS
    if (isatty(fileno(stderr))) {
        DkdAnsiColor levelColor;

        levelColor = dkdGetLogLevelColor(level);
        pLevelColorStart = dkdGetAnsiColorString(levelColor);
        pLevelColorEnd = dkdGetAnsiColorString(DKD_ANSI_COLOR_RESET);
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

static void
dkdLog(void *pData,
       DkdLogLevel level,
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

static const DkdLoggingCallbacks dkdCustomLogger = {NULL, dkdLog, dkdLogVaList};

void
dkdGetCustomLogger(const DkdLoggingCallbacks **ppLogger)
{
    assert(ppLogger != NULL);

    *ppLogger = &dkdCustomLogger;
}
