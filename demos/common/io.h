#ifndef DEKOI_DEMOS_COMMON_IO_H
#define DEKOI_DEMOS_COMMON_IO_H

#include <stddef.h>
#include <stdio.h>

struct DkdLoggingCallbacks;

struct DkdFile {
    FILE *pHandle;
    const char *pPath;
};

int
dkdOpenFile(struct DkdFile *pFile,
            const char *pPath,
            const char *pMode,
            const struct DkdLoggingCallbacks *pLogger);

int
dkdGetFileSize(size_t *pSize,
               struct DkdFile *pFile,
               const struct DkdLoggingCallbacks *pLogger);

int
dkdReadFile(void *pBuffer,
            struct DkdFile *pFile,
            size_t size,
            const struct DkdLoggingCallbacks *pLogger);

int
dkdCloseFile(struct DkdFile *pFile, const struct DkdLoggingCallbacks *pLogger);

#endif /* DEKOI_DEMOS_COMMON_IO_H */
