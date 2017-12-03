#include "io.h"

#include "assert.h"
#include "dekoi.h"

#include <errno.h>

DkResult
dkpOpenFile(DkpFile *pFile, const char *pPath, const char *pMode)
{
    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pPath != NULL);
    DKP_ASSERT(pMode != NULL);

    /* Only POSIX requires `errno` to be set when `fopen()` fails. */
    errno = 0;
    pFile->pHandle = fopen(pPath, pMode);
    if (pFile->pHandle == NULL) {
        fprintf(stderr, "could not open the file '%s'\n", pPath);
        return DK_ERROR;
    }

    pFile->pPath = pPath;
    return DK_SUCCESS;
}

DkResult
dkpGetFileSize(DkpFile *pFile, size_t *pSize)
{
    DkResult out;
    fpos_t position;
    long size;

    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFile->pHandle != NULL);
    DKP_ASSERT(pSize != NULL);

    out = DK_SUCCESS;

    errno = 0;
    if (fgetpos(pFile->pHandle, &position) != 0) {
        fprintf(stderr,
                "could not retrieve the current cursor position for "
                "the file '%s'\n",
                pFile->pPath);
        out = DK_ERROR;
        goto exit;
    }

    errno = 0;
    if (fseek(pFile->pHandle, 0, SEEK_END) != 0) {
        fprintf(
            stderr, "could not reach the end of the file '%s'\n", pFile->pPath);
        out = DK_ERROR;
        goto exit;
    }

    size = ftell(pFile->pHandle);
    if (size == EOF) {
        fprintf(stderr,
                "could not retrieve the size of the file '%s'\n",
                pFile->pPath);
        out = DK_ERROR;
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
        out = DK_ERROR;
    }

exit:
    return out;
}

DkResult
dkpReadFile(DkpFile *pFile, size_t size, void *pBuffer)
{
    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFile->pHandle != NULL);
    DKP_ASSERT(pBuffer != NULL);

    if (fread(pBuffer, 1, size, pFile->pHandle) != size) {
        fprintf(stderr, "could not read the file '%s'\n", pFile->pPath);
        return DK_ERROR;
    }

    return DK_SUCCESS;
}

DkResult
dkpCloseFile(DkpFile *pFile)
{
    DKP_ASSERT(pFile != NULL);
    DKP_ASSERT(pFile->pHandle != NULL);

    if (fclose(pFile->pHandle) == EOF) {
        fprintf(stderr, "could not close the file '%s'\n", pFile->pPath);
        return DK_ERROR;
    }

    return DK_SUCCESS;
}
