#include "common.h"

void
dkGetStatusDescription(const char **ppDescription, enum DkStatus status)
{
    switch (status) {
        case DK_SUCCESS:
            *ppDescription = "success";
            return;
        case DK_ERROR:
            *ppDescription = "error";
            return;
        case DK_ERROR_INVALID_VALUE:
            *ppDescription = "invalid value";
            return;
        case DK_ERROR_ALLOCATION:
            *ppDescription = "allocation error";
            return;
        case DK_ERROR_NOT_AVAILABLE:
            *ppDescription = "not available";
            return;
        default:
            *ppDescription = "invalid";
    }
}
