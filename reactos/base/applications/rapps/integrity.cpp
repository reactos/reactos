/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/integrity.cpp
 * PURPOSE:         Various integrity check mechanisms
 * PROGRAMMERS:     Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *                  Mark Jansen
 */

#include "rapps.h"
#include <sha1.h>


BOOL VerifyInteg(LPCWSTR lpSHA1Hash, LPCWSTR lpFileName)
{
    BOOL ret = FALSE;

    /* first off, does it exist at all? */
    HANDLE file = CreateFileW(lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);

    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    /* let's grab the actual file size to organize the mmap'ing rounds */
    LARGE_INTEGER size;
    GetFileSizeEx(file, &size);

    /* retrieve a handle to map the file contents to memory */
    HANDLE map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (map)
    {
        /* map that thing in address space */
        const unsigned char *file_map = static_cast<const unsigned char *>(MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0));
        if (file_map)
        {
            SHA_CTX ctx;
            /* initialize the SHA-1 context */
            A_SHAInit(&ctx);

            /* feed the data to the cookie monster */
            A_SHAUpdate(&ctx, file_map, size.LowPart);

            /* cool, we don't need this anymore */
            UnmapViewOfFile(file_map);

            /* we're done, compute the final hash */
            ULONG sha[5];
            A_SHAFinal(&ctx, sha);

            WCHAR buf[(sizeof(sha) * 2) + 1];
            for (UINT i = 0; i < sizeof(sha); i++)
                swprintf(buf + 2 * i, L"%02x", ((unsigned char *)sha)[i]);
            /* does the resulting SHA1 match with the provided one? */
            if (!_wcsicmp(buf, lpSHA1Hash))
                ret = TRUE;
        }
        CloseHandle(map);
    }
    CloseHandle(file);
    return ret;
}
