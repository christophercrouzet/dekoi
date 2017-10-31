#include "dekoi.h"


const char *
dkGetResultString(DkResult result)
{
    switch (result) {
        case DK_SUCCESS:
            return "Success";
        case DK_ERROR:
            return "Error";
        default:
            return "Invalid";
    }
}
