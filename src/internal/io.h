#ifndef DEKOI_INTERNAL_IO_H
#define DEKOI_INTERNAL_IO_H

#include "../dekoi.h"

#include <stddef.h>
#include <stdio.h>

typedef struct DkpFile {
    FILE *pHandle;
    const char *pPath;
} DkpFile;

DkResult dkpOpenFile(DkpFile *pFile, const char *pPath, const char *pMode);
DkResult dkpGetFileSize(DkpFile *pFile, size_t *pSize);
DkResult dkpReadFile(DkpFile *pFile, size_t size, void *pBuffer);
DkResult dkpCloseFile(DkpFile *pFile);

#endif /* DEKOI_INTERNAL_IO_H */
