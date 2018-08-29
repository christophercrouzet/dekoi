#include "common.h"

void
dkGetResultString(const char **ppString, enum DkResult result)
{
    switch (result) {
        case DK_SUCCESS:
            *ppString = "success";
            return;
        case DK_ERROR:
            *ppString = "error";
            return;
        case DK_ERROR_INVALID_VALUE:
            *ppString = "invalid value";
            return;
        case DK_ERROR_ALLOCATION:
            *ppString = "allocation error";
            return;
        case DK_ERROR_NOT_AVAILABLE:
            *ppString = "not available";
            return;
        default:
            *ppString = "invalid";
    }
}
