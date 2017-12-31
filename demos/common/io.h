#ifndef DEKOI_DEMOS_COMMON_IO_H
#define DEKOI_DEMOS_COMMON_IO_H

#include "common.h"

#include <stddef.h>
#include <stdio.h>

typedef struct DkdFile {
    FILE *pHandle;
    const char *pPath;
} DkdFile;

int
dkdOpenFile(DkdFile *pFile,
            const char *pPath,
            const char *pMode,
            const DkdLoggingCallbacks *pLogger);

int
dkdGetFileSize(DkdFile *pFile,
               const DkdLoggingCallbacks *pLogger,
               size_t *pSize);

int
dkdReadFile(DkdFile *pFile,
            size_t size,
            const DkdLoggingCallbacks *pLogger,
            void *pBuffer);

int
dkdCloseFile(DkdFile *pFile, const DkdLoggingCallbacks *pLogger);

#endif /* DEKOI_DEMOS_COMMON_IO_H */
