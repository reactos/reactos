//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       hshstrss.cpp
//
//  Contents:   Hashing Stress
//
//  History:    21-Nov-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "crypthlp.h"


BOOL _HashFile(HANDLE hFile, HCRYPTPROV hProv, char *pszFile);

BOOL    fVerbose        = FALSE;
DWORD   dwTotalBytes    = 0;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    cWArgv_                 *pArgs;
    WCHAR                   *pwszFile;
    DWORD                   dwTotal;
    HANDLE                  hFile;
    int                     iRet;

    BOOL                    fFind;
    HANDLE                  hFind;
    WIN32_FIND_DATA         sFindData;
    HCRYPTPROV              hProv;

    COleDateTime            tStart;
    COleDateTime            tEnd;
    COleDateTimeSpan        tsTotal;

    iRet                = 1;    // cmd fail
    dwTotal             = 0;
    hFind               = INVALID_HANDLE_VALUE;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,         IDS_PARAMTEXT_HELP,         WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,      IDS_PARAMTEXT_VERBOSE,      WARGV_VALUETYPE_BOOL, (void *)FALSE);

    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s", pArgs->GetUsageString());

        goto NeededHelp;
    }

    fVerbose    = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));

    if (!(pwszFile = pArgs->GetFileName()))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto ParamError;
    }

    //
    //  start our timer
    //
    tStart              = COleDateTime::GetCurrentTime();

    if (fVerbose)
    {
        printf("\nProcessing:");
    }

    char    szFile[MAX_PATH];

    szFile[0] = NULL;
    WideCharToMultiByte(0, 0, pwszFile, -1, &szFile[0], MAX_PATH, NULL, NULL);

    if ((hFind = FindFirstFile(&szFile[0], &sFindData)) == INVALID_HANDLE_VALUE)
    {
        goto FileFindError;
    }

    fFind   = TRUE;

    hProv = I_CryptGetDefaultCryptProv(0);

    while (fFind)
    {
        if (!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            hFile = CreateFile(sFindData.cFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                if (_HashFile(hFile, hProv, sFindData.cFileName))
                {
                    dwTotal++;
                }
            }
        }

        fFind = FindNextFile(hFind, &sFindData);
    }

    //
    //  end timer
    //
    tEnd    = COleDateTime::GetCurrentTime();
    tsTotal = tEnd - tStart;

    printf("\n");
    printf("\nTiming:");
    printf("\nTotal files:                  %ld", dwTotal);
    printf("\nProcessing time:              %s", (LPCSTR)tsTotal.Format("%D:%H:%M:%S"));
    printf("\nAverage per file:             %f", (double)tsTotal.GetTotalSeconds() / (double)dwTotal);
    printf("\nTotal bytes:                  %ld (k)", dwTotalBytes / 1000L);
    printf("\nAverage per (k):              %f", (double)tsTotal.GetTotalSeconds() / (double)(dwTotalBytes / 1000L));
    printf("\nAverage bytes per file (k):   %f", (double)(dwTotalBytes / 1000L) / (double)dwTotal);
    printf("\n");

    iRet = 0;

    CommonReturn:
        DELETE_OBJECT(pArgs);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
        }

        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, FileFindError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
}


BOOL _HashFile(HANDLE hFile, HCRYPTPROV hProv, char *pszFile)
{
    HCRYPTHASH  hHash;
    DWORD       cbHash;
    BYTE        bHash[30];
    BYTE        *pbFile;
    DWORD       cbFile;
    HANDLE      hMappedFile;
    BOOL        fRet;

    pbFile      = NULL;
    hMappedFile = NULL;
    hHash       = NULL;

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

    if (!(CryptCreateHash(hProv, CALG_SHA1, NULL, 0, &hHash)))
    {
        goto CreateHashError;
    }

    if (!(CryptHashData(hHash, pbFile, cbFile, 0)))
    {
        goto HashDataError;
    }

    cbHash = 30;

    if (!(CryptGetHashParam(hHash, HP_HASHVAL, bHash, &cbHash, 0)))
    {
        goto HashParamError;
    }

    dwTotalBytes += cbFile;

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

    if (hHash)
    {
        CryptDestroyHash(hHash);
    }

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
    TRACE_ERROR_EX(DBG_SS_APP, CreateHashError);
    TRACE_ERROR_EX(DBG_SS_APP, HashDataError);
    TRACE_ERROR_EX(DBG_SS_APP, HashParamError);
}
