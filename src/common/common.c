#include "common.h"

const char *
dkGetResultString(enum DkResult result)
{
    switch (result) {
        case DK_SUCCESS:
            return "success";
        case DK_ERROR:
            return "error";
        case DK_ERROR_INVALID_VALUE:
            return "invalid value";
        case DK_ERROR_ALLOCATION:
            return "allocation error";
        case DK_ERROR_NOT_AVAILABLE:
            return "not available";
        default:
            return "invalid";
    }
}
