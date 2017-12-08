#include "io.h"

#include "logging.h"
#include "test.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>

int
plOpenFile(PlFile *pFile,
           const char *pPath,
           const char *pMode,
           const PlLoggingCallbacks *pLogger)
{
    assert(pFile != NULL);
    assert(pPath != NULL);
    assert(pMode != NULL);

    /* Only POSIX requires `errno` to be set when `fopen()` fails. */
    errno = 0;
    pFile->pHandle = fopen(pPath, pMode);
    if (pFile->pHandle == NULL) {
        PL_ERROR_1(pLogger, "could not open the file '%s'\n", pPath);
        return 1;
    }

    pFile->pPath = pPath;
    return 0;
}

int
plGetFileSize(PlFile *pFile, const PlLoggingCallbacks *pLogger, size_t *pSize)
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
        PL_ERROR_1(pLogger,
                   "could not retrieve the current cursor position for the "
                   "file '%s'\n",
                   pFile->pPath);
        out = 1;
        goto exit;
    }

    errno = 0;
    if (fseek(pFile->pHandle, 0, SEEK_END) != 0) {
        PL_ERROR_1(pLogger,
                   "could not reach the end of the file '%s'\n",
                   pFile->pPath);
        out = 1;
        goto exit;
    }

    size = ftell(pFile->pHandle);
    if (size == EOF) {
        PL_ERROR_1(pLogger,
                   "could not retrieve the size of the file '%s'\n",
                   pFile->pPath);
        out = 1;
        goto position_restoration;
    }

    *pSize = (size_t)size;

position_restoration:
    errno = 0;
    if (fsetpos(pFile->pHandle, &position) != 0) {
        PL_ERROR_1(pLogger,
                   "could not restore the cursor position for the file '%s'\n",
                   pFile->pPath);
        out = 1;
    }

exit:
    return out;
}

int
plReadFile(PlFile *pFile,
           size_t size,
           const PlLoggingCallbacks *pLogger,
           void *pBuffer)
{
    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);
    assert(pBuffer != NULL);

    if (fread(pBuffer, 1, size, pFile->pHandle) != size) {
        PL_ERROR_1(pLogger, "could not read the file '%s'\n", pFile->pPath);
        return 1;
    }

    return 0;
}

int
plCloseFile(PlFile *pFile, const PlLoggingCallbacks *pLogger)
{
    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);

    if (fclose(pFile->pHandle) == EOF) {
        PL_ERROR_1(pLogger, "could not close the file '%s'\n", pFile->pPath);
        return 1;
    }

    return 0;
}
