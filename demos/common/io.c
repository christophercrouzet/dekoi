#include "io.h"

#include "common.h"
#include "logger.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>

int
dkdOpenFile(DkdFile *pFile,
            const char *pPath,
            const char *pMode,
            const DkdLoggingCallbacks *pLogger)
{
    assert(pFile != NULL);
    assert(pPath != NULL);
    assert(pMode != NULL);

    /* Only POSIX requires `errno` to be set when `fopen()` fails. */
    errno = 0;
    pFile->pHandle = fopen(pPath, pMode);
    if (pFile->pHandle == NULL) {
        DKD_LOG_ERROR(pLogger, "could not open the file '%s'\n", pPath);
        return 1;
    }

    pFile->pPath = pPath;
    return 0;
}

int
dkdGetFileSize(DkdFile *pFile,
               const DkdLoggingCallbacks *pLogger,
               size_t *pSize)
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
        DKD_LOG_ERROR(pLogger,
                      "could not retrieve the current cursor position for the "
                      "file '%s'\n",
                      pFile->pPath);
        out = 1;
        goto exit;
    }

    errno = 0;
    if (fseek(pFile->pHandle, 0, SEEK_END) != 0) {
        DKD_LOG_ERROR(pLogger,
                      "could not reach the end of the file '%s'\n",
                      pFile->pPath);
        out = 1;
        goto exit;
    }

    size = ftell(pFile->pHandle);
    if (size == EOF) {
        DKD_LOG_ERROR(pLogger,
                      "could not retrieve the size of the file '%s'\n",
                      pFile->pPath);
        out = 1;
        goto position_restoration;
    }

    *pSize = (size_t)size;

position_restoration:
    errno = 0;
    if (fsetpos(pFile->pHandle, &position) != 0) {
        DKD_LOG_ERROR(
            pLogger,
            "could not restore the cursor position for the file '%s'\n",
            pFile->pPath);
        out = 1;
    }

exit:
    return out;
}

int
dkdReadFile(DkdFile *pFile,
            size_t size,
            const DkdLoggingCallbacks *pLogger,
            void *pBuffer)
{
    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);
    assert(pBuffer != NULL);

    if (fread(pBuffer, 1, size, pFile->pHandle) != size) {
        DKD_LOG_ERROR(pLogger, "could not read the file '%s'\n", pFile->pPath);
        return 1;
    }

    return 0;
}

int
dkdCloseFile(DkdFile *pFile, const DkdLoggingCallbacks *pLogger)
{
    assert(pFile != NULL);
    assert(pFile->pHandle != NULL);

    if (fclose(pFile->pHandle) == EOF) {
        DKD_LOG_ERROR(pLogger, "could not close the file '%s'\n", pFile->pPath);
        return 1;
    }

    return 0;
}
