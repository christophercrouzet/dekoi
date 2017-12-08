#include "logging.h"

const char *
dkGetLogLevelString(DkLogLevel level)
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
