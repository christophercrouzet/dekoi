#include "logger.h"

void
dkGetLogLevelName(const char **ppName, enum DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            *ppName = "debug";
            return;
        case DK_LOG_LEVEL_INFO:
            *ppName = "info";
            return;
        case DK_LOG_LEVEL_WARNING:
            *ppName = "warning";
            return;
        case DK_LOG_LEVEL_ERROR:
            *ppName = "error";
            return;
        default:
            *ppName = "invalid";
    }
}
