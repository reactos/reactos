/*
 * PROJECT:     ReactOS certutil
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CertUtil hashfile implementation
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"
#include <wincrypt.h>
#include <stdlib.h>


BOOL hash_file(LPCWSTR Filename)
{
    HCRYPTPROV hProv;
    BOOL bSuccess = FALSE;

    HANDLE hFile = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ConPrintf(StdOut, L"CertUtil: -hashfile command failed: %d\n", GetLastError());
        return bSuccess;
    }

    if (CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        HCRYPTHASH hHash;

        if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
        {
            BYTE Buffer[2048];
            DWORD cbRead;

            while ((bSuccess = ReadFile(hFile, Buffer, sizeof(Buffer), &cbRead, NULL)))
            {
                if (cbRead == 0)
                    break;

                if (!CryptHashData(hHash, Buffer, cbRead, 0))
                {
                    bSuccess = FALSE;
                    ConPrintf(StdOut, L"CertUtil: -hashfile command failed to hash: %d\n", GetLastError());
                    break;
                }
            }

            if (bSuccess)
            {
                BYTE rgbHash[20];
                DWORD cbHash, n;

                if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
                {
                    ConPrintf(StdOut, L"SHA1 hash of %s:\n", Filename);
                    for (n = 0; n < cbHash; ++n)
                    {
                        ConPrintf(StdOut, L"%02x", rgbHash[n]);
                    }
                    ConPuts(StdOut, L"\n");
                }
                else
                {
                    ConPrintf(StdOut, L"CertUtil: -hashfile command failed to extract hash: %d\n", GetLastError());
                    bSuccess = FALSE;
                }
            }

            CryptDestroyHash(hHash);
        }
        else
        {
            ConPrintf(StdOut, L"CertUtil: -hashfile command no algorithm: %d\n", GetLastError());
        }

        CryptReleaseContext(hProv, 0);
    }
    else
    {
        ConPrintf(StdOut, L"CertUtil: -hashfile command no context: %d\n", GetLastError());
    }

    CloseHandle(hFile);
    return bSuccess;
}

