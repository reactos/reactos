//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wvtstrss.cpp
//
//  Contents:   WinVerifyTrust Stress
//
//  History:    13-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


#define     STRING_SEPERATOR            L'*'

typedef struct STATEINFO_
{
    WCHAR           wszCatalogFile[MAX_PATH];
    HANDLE          hState;

} STATEINFO;

void            _StripQuotes(WCHAR *pwszIn);
void            _Add2CatDB(WCHAR *pwszCatFile, WCHAR *pwszMemberTag, WCHAR *pwszMemberFile, DWORD dwExpectedReturn);
void            _VerifyMember(WCHAR *pwszMemberTag, WCHAR *pwszMemberFile, DWORD dwExpectedReturn);
void            _CloseWVTHandles(void);
STATEINFO *     _FindStateHandle(WCHAR *pwszCatalogFile);
void            _ToLower(WCHAR *pwszInOut);


Stack_      *pStateHandles  = NULL;
GUID        gAction         = DRIVER_ACTION_VERIFY;
GUID        gSS             = DRIVER_ACTION_VERIFY;
HCATADMIN   hCatAdmin       = NULL;
BOOL        fCatalogAdded   = FALSE;
BOOL        fVerbose;
DWORD       dwTotalCatalogs = 0;
DWORD       dwTotalErrors   = 0;
HWND        hWnd            = NULL;


extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    WCHAR                   *pwszLoopFile;
    WCHAR                   *pwszCatFile;
    WCHAR                   *pwszMemFile;
    WCHAR                   *pwsz;
    DWORD                   dwExpectedReturn;
    DWORD                   dwCount;
    DWORD                   dwTotalFiles;
    BOOL                    fVerbose;
    int                     iRet;

    cWArgv_                 *pArgs;
    fParse_                 *pLoopFile;

    COleDateTime            tStart;
    COleDateTime            tEnd;
    COleDateTimeSpan        tsTotal;

    pLoopFile           = NULL;

    dwTotalFiles        = 0;
    dwCount             = 1;
    iRet                = 0;
    dwExpectedReturn    = S_OK;

    hWnd                = GetDesktopWindow();

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    if (!(pStateHandles = new Stack_(NULL)))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,     IDS_PARAMTEXT_HELP,     WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,  IDS_PARAMTEXT_VERBOSE,  WARGV_VALUETYPE_BOOL,   (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_SSGUID,   IDS_PARAMTEXT_SSGUID,   WARGV_VALUETYPE_WCHAR,  NULL);
    pArgs->Add2List(IDS_PARAM_ADD2DB,   IDS_PARAMTEXT_ADD2DB,   WARGV_VALUETYPE_WCHAR,  NULL);

    pArgs->Fill(argc, wargv);

    if (pArgs->GetValue(IDS_PARAM_HELP))
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto NeededHelp;
    }

    if (!(pwszLoopFile = pArgs->GetFileName()))
    {
        wprintf(L"%s\n", pArgs->GetUsageString());
        goto ParamError;
    }

    if (!(pLoopFile = new fParse_(pwszLoopFile, MAX_PATH * 2)))
    {
        goto MemoryError;
    }

    pLoopFile->Reset();

    fVerbose    = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));

    if (pArgs->GetValue(IDS_PARAM_SSGUID))
    {
        if (!(wstr2guid((WCHAR *)pArgs->GetValue(IDS_PARAM_SSGUID), &gSS)))
        {
            wprintf(L"%s\n", pArgs->GetUsageString());
            goto ParamError;
        }
    }

    pwszCatFile = (WCHAR *)pArgs->GetValue(IDS_PARAM_ADD2DB);

    //
    //  start our timer
    //
    tStart      = COleDateTime::GetCurrentTime();

    if (!(CryptCATAdminAcquireContext(&hCatAdmin, (pwszCatFile) ? &gSS : NULL, 0)))
    {
        if (GetLastError() != dwExpectedReturn)
        {
            printf("\nERROR: unable to aquire CatAdminContext: return 0x%08X\n", GetLastError());
        }

        goto MSCATError;
    }

    while (pLoopFile->GetNextLine())
    {
        pLoopFile->EOLRemove();

        //
        //  format:
        //          catalog member tag^catalog member file^expected return code
        //
        if (!(pwszMemFile = wcschr(pLoopFile->GetCurrentLine(), STRING_SEPERATOR)))
        {
            if (fVerbose)
            {
                wprintf(L"   parse error at line: %s\n", pLoopFile->GetCurrentLine());
            }
            continue;
        }

        *pwszMemFile = NULL;
        pwszMemFile++;

        if (!(pwsz = wcschr(pwszMemFile, STRING_SEPERATOR)))
        {
            if (fVerbose)
            {
                pwszMemFile--;
                *pwszMemFile = STRING_SEPERATOR;
                wprintf(L"   parse error at line: %s\n", pLoopFile->GetCurrentLine());
            }
            continue;
        }

        *pwsz = NULL;
        pwsz++;
        dwExpectedReturn = (DWORD)_wtol(pwsz);

        _StripQuotes(pwszMemFile);
        _StripQuotes(pLoopFile->GetCurrentLine());

        if (pwszCatFile)
        {
            //
            //  we're adding
            //
            _Add2CatDB(pwszCatFile, pLoopFile->GetCurrentLine(), pwszMemFile, dwExpectedReturn);
        }
        else
        {
            //
            //  we're verifying
            //
            _VerifyMember(pLoopFile->GetCurrentLine(), pwszMemFile, dwExpectedReturn);
        }

        if (fVerbose)
        {
            wprintf(L"processed: %s\n", pwszMemFile);
        }

        dwTotalFiles++;
    }

    tEnd    = COleDateTime::GetCurrentTime();
    tsTotal = tEnd - tStart;

    printf("\n");
    printf("\nTotal files processed:    %ld", dwTotalFiles);
    printf("\nTotal Catalogs loaded:    %ld", dwTotalCatalogs);
    printf("\nTotal errors:             %ld", dwTotalErrors);
    printf("\nProcessing time:          %s", (LPCSTR)tsTotal.Format("%D:%H:%M:%S"));
    printf("\nAverage seconds per file: %f", (double)tsTotal.GetTotalSeconds() / (double)dwTotalFiles);
    printf("\n");

    CommonReturn:
        _CloseWVTHandles();

        if (hCatAdmin)
        {
            CryptCATAdminReleaseContext(hCatAdmin, 0);
        }


        DELETE_OBJECT(pArgs);
        DELETE_OBJECT(pStateHandles);
        DELETE_OBJECT(pLoopFile);

        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MSCATError);
    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
}

void _Add2CatDB(WCHAR *pwszCatFile, WCHAR *pwszMemberTag, WCHAR *pwszMemberFile, DWORD dwExpectedReturn)
{
    if (!(fCatalogAdded))
    {
        HCATINFO    hCatInfo;

        if (!(hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatFile, NULL, 0)))
        {
            if (GetLastError() != dwExpectedReturn)
            {
                wprintf(L"\nERROR: unable to add catalog: %s: return 0x%08X\n", pwszCatFile, GetLastError());
                dwTotalErrors++;
            }

            return;
        }

        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);


        fCatalogAdded = TRUE;
    }
}

void _VerifyMember(WCHAR *pwszMemberTag, WCHAR *pwszMemberFile, DWORD dwExpectedReturn)
{
    HCATINFO                hCatInfo;
    CATALOG_INFO            sCatInfo;
    WCHAR                   wszCatFile[MAX_PATH];
    BYTE                    bHash[40];
    BYTE                    *pbHash;
    DWORD                   cbHash;
    HANDLE                  hFile;


    SetLastError(0);

    hCatInfo    = NULL;
    hFile       = INVALID_HANDLE_VALUE;

    if ((hFile = CreateFileU(pwszMemberFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 NULL)) == INVALID_HANDLE_VALUE)
    {
        goto FailedOpenFile;
    }

    cbHash  = 40;
    pbHash  = &bHash[0];
    if (!(CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, pbHash, 0)))
    {
        goto FailedHashCalc;
    }

    if (!(hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, pbHash, cbHash, 0, NULL)))
    {
        goto FailedEnumCatalog;
    }

    memset(&sCatInfo, 0x00, sizeof(CATALOG_INFO));
    sCatInfo.cbStruct = sizeof(CATALOG_INFO);

    if (!(CryptCATCatalogInfoFromContext(hCatInfo, &sCatInfo, 0)))
    {
        goto FailedEnumCatalog;
    }

    wcscpy(&wszCatFile[0], sCatInfo.wszCatalogFile);

    pwszMemberTag = wcsrchr(pwszMemberFile, L'\\');

    if (pwszMemberTag)
    {
        pwszMemberTag++;
    }
    else
    {
        pwszMemberTag = pwszMemberFile;
    }

    _ToLower(pwszMemberTag);

    WINTRUST_DATA           sWTD;
    WINTRUST_CATALOG_INFO   sWTCI;
    STATEINFO               *psState;
    HRESULT                 hr;

    if (!(psState = _FindStateHandle(&wszCatFile[0])))
    {
        goto MemoryError;
    }

    memset(&sWTD,   0x00,   sizeof(WINTRUST_DATA));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_CATALOG;
    sWTD.pCatalog       = &sWTCI;
    sWTD.dwStateAction  = WTD_STATEACTION_VERIFY;
    sWTD.hWVTStateData  = psState->hState;

    memset(&sWTCI,  0x00,   sizeof(WINTRUST_CATALOG_INFO));

    sWTCI.cbStruct              = sizeof(WINTRUST_CATALOG_INFO);
    sWTCI.pcwszCatalogFilePath  = &wszCatFile[0];
    sWTCI.pcwszMemberTag        = pwszMemberTag;
    sWTCI.pcwszMemberFilePath   = pwszMemberFile;
    sWTCI.hMemberFile           = hFile;
    sWTCI.pbCalculatedFileHash  = pbHash;
    sWTCI.cbCalculatedFileHash  = cbHash;

    hr = WinVerifyTrust(hWnd, &gAction, &sWTD);

    psState->hState = sWTD.hWVTStateData;

    CommonReturn:
        if (hr != (HRESULT)dwExpectedReturn)
        {
            wprintf(L"\nERROR: unexpected error from WVT for %s: return 0x%08X expected: 0x%08X lasterror: 0x%08X\n",
                    pwszMemberTag, hr, dwExpectedReturn, GetLastError());
            dwTotalErrors++;
        }

        if (hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hFile);
        }

        if (hCatInfo)
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }
        return;

    ErrorReturn:
        if (GetLastError() != dwExpectedReturn)
        {
            wprintf(L"\nERROR: unable to find member: %s: return 0x%08X expected: 0x%08X\n",
                    pwszMemberTag, GetLastError(), dwExpectedReturn);
            dwTotalErrors++;
        }

        hr = dwExpectedReturn;
        goto CommonReturn;


    TRACE_ERROR_EX(DBG_SS_APP, FailedHashCalc);
    TRACE_ERROR_EX(DBG_SS_APP, FailedOpenFile);
    TRACE_ERROR_EX(DBG_SS_APP, FailedEnumCatalog);

    SET_ERROR_VAR_EX(DBG_SS_APP, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
}


void _CloseWVTHandles(void)
{
    DWORD                   dwIdx;
    WINTRUST_DATA           sWTD;
    WINTRUST_CATALOG_INFO   sWTCI;
    STATEINFO               *psState;

    dwIdx = 0;

    memset(&sWTD,   0x00,   sizeof(WINTRUST_DATA));

    sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice     = WTD_UI_NONE;
    sWTD.dwUnionChoice  = WTD_CHOICE_CATALOG;
    sWTD.pCatalog       = &sWTCI;
    sWTD.dwStateAction  = WTD_STATEACTION_CLOSE;

    memset(&sWTCI,  0x00,   sizeof(WINTRUST_CATALOG_INFO));

    sWTCI.cbStruct              = sizeof(WINTRUST_CATALOG_INFO);


    while (psState = (STATEINFO *)pStateHandles->Get(dwIdx))
    {
        if (psState->hState)
        {
            sWTD.hWVTStateData  = psState->hState;

            WinVerifyTrust(NULL, &gAction, &sWTD);
        }

        dwIdx++;
    }
}

STATEINFO * _FindStateHandle(WCHAR *pwszCatalogFile)
{
    STATEINFO   *psState;
    DWORD       dwIdx;

    dwIdx = 0;

    while (psState = (STATEINFO *)pStateHandles->Get(dwIdx))
    {
        if (wcscmp(&psState->wszCatalogFile[0], pwszCatalogFile) == 0)
        {
            return(psState);
        }

        dwIdx++;
    }

    if (!(psState = (STATEINFO *)pStateHandles->Add(sizeof(STATEINFO))))
    {
        return(NULL);
    }

    memset(psState, 0x00, sizeof(STATEINFO));

    wcscpy(&psState->wszCatalogFile[0], pwszCatalogFile);

    dwTotalCatalogs++;

    return(psState);
}

void _StripQuotes(WCHAR *pwszIn)
{
    DWORD   dwSrc;
    DWORD   dwDst;
    DWORD   dwLen;

    dwSrc = 0;
    dwDst = 0;
    dwLen = wcslen(pwszIn);

    while (dwSrc < dwLen)
    {
        if (pwszIn[dwSrc] != L'\"')
        {
            pwszIn[dwDst] = pwszIn[dwSrc];
            dwDst++;
        }
        dwSrc++;
    }

    pwszIn[dwDst] = NULL;
}


void _ToLower(WCHAR *pwszInOut)
{
    while (*pwszInOut)
    {
        *pwszInOut = towlower(*pwszInOut);
        pwszInOut++;
    }
}
