#include "dekoi.h"


const char *
dkGetResultString(DkResult result)
{
    switch (result) {
        case DK_SUCCESS:
            return "success";
        case DK_ERROR:
            return "error";
        case DK_ERROR_ALLOCATION:
            return "allocation error";
        default:
            return "invalid";
    }
}
