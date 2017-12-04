#ifndef DEKOI_PLAYGROUND_TEST_IO_H
#define DEKOI_PLAYGROUND_TEST_IO_H

#include "test.h"

#include <stddef.h>
#include <stdio.h>

typedef struct File {
    FILE *pHandle;
    const char *pPath;
} File;

int
openFile(File *pFile, const char *pPath, const char *pMode);

int
getFileSize(File *pFile, size_t *pSize);

int
readFile(File *pFile, size_t size, void *pBuffer);

int
closeFile(File *pFile);

#endif /* DEKOI_PLAYGROUND_TEST_IO_H */
