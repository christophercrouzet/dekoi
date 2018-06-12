#include "logger.h"

const char *
dkGetLogLevelString(enum DkLogLevel level)
{
    switch (level) {
        case DK_LOG_LEVEL_DEBUG:
            return "debug";
        case DK_LOG_LEVEL_INFO:
            return "info";
        case DK_LOG_LEVEL_WARNING:
            return "warning";
        case DK_LOG_LEVEL_ERROR:
            return "error";
        default:
            return "invalid";
    }
}
