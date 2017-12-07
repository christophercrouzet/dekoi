#ifndef DEKOI_PLAYGROUND_TEST_IO_H
#define DEKOI_PLAYGROUND_TEST_IO_H

#include "test.h"

#include <stddef.h>
#include <stdio.h>

typedef struct PlFile {
    FILE *pHandle;
    const char *pPath;
} PlFile;

int
plOpenFile(PlFile *pFile, const char *pPath, const char *pMode);

int
plGetFileSize(PlFile *pFile, size_t *pSize);

int
plReadFile(PlFile *pFile, size_t size, void *pBuffer);

int
plCloseFile(PlFile *pFile);

#endif /* DEKOI_PLAYGROUND_TEST_IO_H */
