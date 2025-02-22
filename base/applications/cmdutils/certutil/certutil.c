/*
 * PROJECT:     ReactOS certutil
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     CertUtil stub
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 *
 * Note: Only -hashfile is implemented for now, the rest is not present!
 */

#include "precomp.h"
#include <wincrypt.h>
#include <stdlib.h>


static BOOL hash_file(LPCWSTR Filename)
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


static void print_usage()
{
    ConPuts(StdOut, L"Verbs:\n");
    ConPuts(StdOut, L"  -hashfile           -- Display cryptographic hash over a file\n");
    ConPuts(StdOut, L"\n");
    ConPuts(StdOut, L"CertUtil -?           -- Display a list of all verbs\n");
    ConPuts(StdOut, L"CertUtil -hashfile -? -- Display help text for the 'hashfile' verb\n");
}

int wmain(int argc, WCHAR *argv[])
{
    int n;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc == 1) /* i.e. no commandline arguments given */
    {
        print_usage();
        return EXIT_SUCCESS;
    }

    for (n = 1; n < argc; ++n)
    {
        if (!_wcsicmp(argv[n], L"-?"))
        {
            print_usage();
            return EXIT_SUCCESS;
        }
        else if (!_wcsicmp(argv[n], L"-hashfile"))
        {
            if (argc == 3)
            {
                if (!_wcsicmp(argv[n+1], L"-?"))
                {
                    print_usage();
                    return EXIT_SUCCESS;
                }
                else
                {
                    if (!hash_file(argv[n+1]))
                    {
                        /* hash_file prints the failure itself */
                        return EXIT_FAILURE;
                    }

                    ConPuts(StdOut, L"CertUtil: -hashfile command completed successfully\n");
                    return EXIT_SUCCESS;
                }
            }
            else
            {
                ConPrintf(StdOut, L"CertUtil: -hashfile expected 1 argument, got %d\n", argc - 2);
                return EXIT_FAILURE;
            }
        }
        else
        {
            ConPrintf(StdOut, L"CertUtil: Unknown verb: %s\n", argv[n]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
