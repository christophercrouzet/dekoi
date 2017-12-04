#include "io.h"

#include <assert.h>
#include <errno.h>

int
openFile(File *pFile, const char *pPath, const char *pMode)
{
    assert(pFile != NULL);
    assert(pPath != NULL);
    assert(pMode != NULL);

    /* Only POSIX requires `errno` to be set when `fopen()` fails. */
    errno = 0;
    pFile->pHandle = fopen(pPath, pMode);
    if (pFile->pHandle == NULL) {
        fprintf(stderr, "could not open the file '%s'\n", pPath);
        return 1;
    }

    pFile->pPath = pPath;
    return 0;
}

int
getFileSize(File *pFile, size_t *pSize)
{
    int out;
    fpos_t position;
    long size;

    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);
    assert(pSize != NULL);

    out = 0;

    errno = 0;
    if (fgetpos(pFile->pHandle, &position) != 0) {
        fprintf(stderr,
                "could not retrieve the current cursor position for "
                "the file '%s'\n",
                pFile->pPath);
        out = 1;
        goto exit;
    }

    errno = 0;
    if (fseek(pFile->pHandle, 0, SEEK_END) != 0) {
        fprintf(
            stderr, "could not reach the end of the file '%s'\n", pFile->pPath);
        out = 1;
        goto exit;
    }

    size = ftell(pFile->pHandle);
    if (size == EOF) {
        fprintf(stderr,
                "could not retrieve the size of the file '%s'\n",
                pFile->pPath);
        out = 1;
        goto position_restoration;
    }

    *pSize = (size_t)size;

position_restoration:
    errno = 0;
    if (fsetpos(pFile->pHandle, &position) != 0) {
        fprintf(stderr,
                "could not restore the cursor position for the file "
                "'%s'\n",
                pFile->pPath);
        out = 1;
    }

exit:
    return out;
}

int
readFile(File *pFile, size_t size, void *pBuffer)
{
    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);
    assert(pBuffer != NULL);

    if (fread(pBuffer, 1, size, pFile->pHandle) != size) {
        fprintf(stderr, "could not read the file '%s'\n", pFile->pPath);
        return 1;
    }

    return 0;
}

int
closeFile(File *pFile)
{
    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);

    if (fclose(pFile->pHandle) == EOF) {
        fprintf(stderr, "could not close the file '%s'\n", pFile->pPath);
        return 1;
    }

    return 0;
}
