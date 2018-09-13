//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       crypthash.cpp
//
//  Contents:   performance suite
//
//  History:    04-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "sha.h"
#include    "md5.h"


BOOL _HashFile(HANDLE hFile, char *pszFile, BOOL fSha1);

DWORD WINAPI TestCryptHash(ThreadData *psData)
{
    COleDateTime    tStart;
    COleDateTime    tEnd;
    DWORD           i;
    char            szFile[MAX_PATH];

    BOOL                    fFind;
    HANDLE                  hFind;
    WIN32_FIND_DATA         sFindData;
    HCRYPTPROV              hProv;
    HANDLE                  hFile;

    WCHAR                   *pwszLastSlash;
    WCHAR                   wszDir[MAX_PATH];
    WCHAR                   wszFile[MAX_PATH];
    DWORD                   dwDirLen;

    hFind   = INVALID_HANDLE_VALUE;

    psData->dwTotalProcessed = 0;

    printf("\n  CRYPT_HASH");

    if (pwszLastSlash = wcsrchr(pwszInFile, L'\\'))
    {
        *pwszLastSlash  = NULL;
        wcscpy(&wszDir[0], pwszInFile);
        wcscat(&wszDir[0], L"\\");
        *pwszLastSlash  = L'\\';
        dwDirLen        = wcslen(&wszDir[0]);
    }
    else
    {
        wszDir[0]   = NULL;
        dwDirLen    = 0;
    }


    tStart = COleDateTime::GetCurrentTime();

    for (i = 0; i < cPasses; i++)
    {
        szFile[0] = NULL;
        WideCharToMultiByte(0, 0, pwszInFile, -1, &szFile[0], MAX_PATH, NULL, NULL);

        if ((hFind = FindFirstFile(&szFile[0], &sFindData)) == INVALID_HANDLE_VALUE)
        {
            goto FileFindError;
        }

        fFind   = TRUE;

        while (fFind)
        {
            if (!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (dwDirLen > 0)
                {
                    wcscpy(&wszFile[0], &wszDir[0]);
                }

                wszFile[dwDirLen] = NULL;
                MultiByteToWideChar(0, 0, &sFindData.cFileName[0], -1, &wszFile[dwDirLen], MAX_PATH * sizeof(WCHAR));

                hFile = CreateFileU(&wszFile[0], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    if (_HashFile(hFile, sFindData.cFileName, 
                                        (psData->dwPassThrough & PASSTHROUGH_SHA1) ? TRUE : FALSE))
                    {
                        psData->dwTotalProcessed++;
                    }
            
                    CloseHandle(hFile);
                }
            }
    
            fFind = FindNextFile(hFind, &sFindData);
        }

        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
        }
    }

ErrorReturn:
    tEnd = COleDateTime::GetCurrentTime();

    psData->tsTotal             = tEnd - tStart;

    return(0);

    TRACE_ERROR_EX(DBG_SS_APP, FileFindError);
}

BOOL _HashFile(HANDLE hFile, char *pszFile, BOOL fSha1)
{
    DWORD       cbHash;
    BYTE        bHash[30];
    BYTE        *pbFile;
    DWORD       cbFile;
    HANDLE      hMappedFile;
    BOOL        fRet;

    MD5_CTX     sMD5;
    A_SHA_CTX   sSHA1;

    pbFile      = NULL;
    hMappedFile = NULL;

    if (fVerbose)
    {
        printf("\n   %s   ", pszFile);
    }

    hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        hMappedFile = NULL;
        goto CreateMapError;
    }

    if (!(pbFile = (BYTE *)MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0)))
    {
        goto MapViewError;
    }

    cbFile = GetFileSize(hFile, NULL);

    if (fSha1)
    {
        memset(&sSHA1, 0x00, sizeof(A_SHA_CTX));
        A_SHAInit(&sSHA1);
        A_SHAUpdate(&sSHA1, pbFile, cbFile);
        A_SHAFinal(&sSHA1, &bHash[0]);
        cbHash = A_SHA_DIGEST_LEN;
    }
    else
    {
        memset(&sMD5, 0x00, sizeof(MD5_CTX));
        MD5Init(&sMD5);
        MD5Update(&sMD5, pbFile, cbFile);
        MD5Final(&sMD5);

        memcpy(&bHash[0], sMD5.digest, MD5DIGESTLEN);
        cbHash = MD5DIGESTLEN;
    }

    //dwTotalBytes += cbFile;

    if (fVerbose)
    {
        DWORD   i;

        for (i = 0; i < cbHash; i++)
        {
            printf("%02.2X", bHash[i]);
        }
    }

    fRet = TRUE;

CommonReturn:

    if (hMappedFile)
    {
        CloseHandle(hMappedFile);
    }

    if (pbFile)
    {
        UnmapViewOfFile(pbFile);
    }

    return(fRet);

ErrorReturn:
    if (fVerbose)
    {
        printf("*failed*");
    }

    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, CreateMapError);
    TRACE_ERROR_EX(DBG_SS_APP, MapViewError);
}
