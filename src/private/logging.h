#ifndef DEKOI_PRIVATE_LOGGING_H
#define DEKOI_PRIVATE_LOGGING_H

#include "../dekoi.h"
#include "../logging.h"

#if defined(DK_LOG_DEBUG)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_DEBUG
#elif defined(DK_LOG_INFO)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_INFO
#elif defined(DK_LOG_WARNING)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_WARNING
#elif defined(DK_LOG_ERROR)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_ERROR
#elif defined(DK_DEBUG)
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_WARNING
#else
#define DKP_LOGGING_LEVEL DK_LOG_LEVEL_ERROR
#endif

#define DKP_LOG(pLogger, level, args) \
    do { \
        if (level >= DKP_LOGGING_LEVEL) { \
            (pLogger)->pfnLog args; \
        } \
    } while (0)

#define DKP_DEBUG_0(pLogger, pFormat) \
    DKP_LOG( \
        pLogger, \
        DK_LOG_LEVEL_DEBUG, \
        ((pLogger)->pData, DK_LOG_LEVEL_DEBUG, __FILE__, __LINE__, pFormat))

#define DKP_DEBUG_1(pLogger, pFormat, _1) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1))

#define DKP_DEBUG_2(pLogger, pFormat, _1, _2) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2))

#define DKP_DEBUG_3(pLogger, pFormat, _1, _2, _3) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3))

#define DKP_DEBUG_4(pLogger, pFormat, _1, _2, _3, _4) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4))

#define DKP_DEBUG_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5))

#define DKP_DEBUG_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6))

#define DKP_DEBUG_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7))

#define DKP_DEBUG_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_DEBUG, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_DEBUG, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7, \
             _8))

#define DKP_INFO_0(pLogger, pFormat) \
    DKP_LOG( \
        pLogger, \
        DK_LOG_LEVEL_INFO, \
        ((pLogger)->pData, DK_LOG_LEVEL_INFO, __FILE__, __LINE__, pFormat))

#define DKP_INFO_1(pLogger, pFormat, _1) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1))

#define DKP_INFO_2(pLogger, pFormat, _1, _2) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2))

#define DKP_INFO_3(pLogger, pFormat, _1, _2, _3) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3))

#define DKP_INFO_4(pLogger, pFormat, _1, _2, _3, _4) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4))

#define DKP_INFO_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5))

#define DKP_INFO_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6))

#define DKP_INFO_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7))

#define DKP_INFO_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_INFO, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_INFO, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7, \
             _8))

#define DKP_WARNING_0(pLogger, pFormat) \
    DKP_LOG( \
        pLogger, \
        DK_LOG_LEVEL_WARNING, \
        ((pLogger)->pData, DK_LOG_LEVEL_WARNING, __FILE__, __LINE__, pFormat))

#define DKP_WARNING_1(pLogger, pFormat, _1) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1))

#define DKP_WARNING_2(pLogger, pFormat, _1, _2) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2))

#define DKP_WARNING_3(pLogger, pFormat, _1, _2, _3) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3))

#define DKP_WARNING_4(pLogger, pFormat, _1, _2, _3, _4) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4))

#define DKP_WARNING_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5))

#define DKP_WARNING_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6))

#define DKP_WARNING_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7))

#define DKP_WARNING_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_WARNING, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_WARNING, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7, \
             _8))

#define DKP_ERROR_0(pLogger, pFormat) \
    DKP_LOG( \
        pLogger, \
        DK_LOG_LEVEL_ERROR, \
        ((pLogger)->pData, DK_LOG_LEVEL_ERROR, __FILE__, __LINE__, pFormat))

#define DKP_ERROR_1(pLogger, pFormat, _1) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1))

#define DKP_ERROR_2(pLogger, pFormat, _1, _2) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2))

#define DKP_ERROR_3(pLogger, pFormat, _1, _2, _3) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3))

#define DKP_ERROR_4(pLogger, pFormat, _1, _2, _3, _4) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4))

#define DKP_ERROR_5(pLogger, pFormat, _1, _2, _3, _4, _5) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5))

#define DKP_ERROR_6(pLogger, pFormat, _1, _2, _3, _4, _5, _6) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6))

#define DKP_ERROR_7(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7))

#define DKP_ERROR_8(pLogger, pFormat, _1, _2, _3, _4, _5, _6, _7, _8) \
    DKP_LOG(pLogger, \
            DK_LOG_LEVEL_ERROR, \
            ((pLogger)->pData, \
             DK_LOG_LEVEL_ERROR, \
             __FILE__, \
             __LINE__, \
             pFormat, \
             _1, \
             _2, \
             _3, \
             _4, \
             _5, \
             _6, \
             _7, \
             _8))

void
dkpGetDefaultLogger(const DkLoggingCallbacks **ppLogger);

#endif /* DEKOI_PRIVATE_LOGGING_H */