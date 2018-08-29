#include "logger.h"

void
dkGetLogLevelString(const char **ppString, enum DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            *ppString = "debug";
            return;
        case DK_LOG_LEVEL_INFO:
            *ppString = "info";
            return;
        case DK_LOG_LEVEL_WARNING:
            *ppString = "warning";
            return;
        case DK_LOG_LEVEL_ERROR:
            *ppString = "error";
            return;
        default:
            *ppString = "invalid";
    }
}
