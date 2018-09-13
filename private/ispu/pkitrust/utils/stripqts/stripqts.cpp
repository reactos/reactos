//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       inf2cdf.cpp
//
//  Contents:   conversion utility
//
//  History:    01-Oct-1997 pberkman    created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

BOOL    fVerbose = FALSE;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    cWArgv_                 *pArgs;
    WCHAR                   wszTFile[MAX_PATH];
    WCHAR                   *pwszCDFFile;
    WCHAR                   *pwsz;
    HANDLE                  hCDFFile;
    HANDLE                  hTFile;
    int                     iRet;

    hCDFFile        = INVALID_HANDLE_VALUE;
    hTFile          = INVALID_HANDLE_VALUE;
    iRet            = 0;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,     IDS_PARAMTEXT_HELP,     WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,  IDS_PARAMTEXT_VERBOSE,  WARGV_VALUETYPE_BOOL, (void *)FALSE);

    pArgs->Fill(argc, wargv);

    if (pArgs->GetValue(IDS_PARAM_HELP))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto NeededHelp;
    }

    fVerbose    = (BOOL)((ULONG_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));
    pwszCDFFile = pArgs->GetFileName();

    if (!(pwszCDFFile))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto ParamError;
    }

    wcscpy(&wszTFile[0], pwszCDFFile);

    pwsz = wcschr(&wszTFile[0], L'.');

    if (pwsz)
    {
        wcscpy(pwsz, L".{1}");
    }
    else
    {
        wcscat(&wszTFile[0], L".{1}");
    }

    hCDFFile    = CreateFileU(pwszCDFFile, GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, 0, NULL);
    hTFile = CreateFileU(&wszTFile[0], GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                                NULL, CREATE_ALWAYS, 0, NULL);

    if ((hTFile == INVALID_HANDLE_VALUE) || (hCDFFile == INVALID_HANDLE_VALUE))
    {
        goto FileError;
    }

    DWORD       cbRead;
    DWORD       cbWrite;
    DWORD       dwSrc;
    DWORD       dwDest;
    BYTE        bRead[MAX_PATH];

    while ((ReadFile(hCDFFile, &bRead[0], MAX_PATH, &cbRead, NULL)) && (cbRead > 0))
    {
        dwSrc   = 0;
        dwDest  = 0;

        while (dwSrc < cbRead)
        {
            if (bRead[dwSrc] != '\"')
            {
                bRead[dwDest] = (BYTE)tolower(bRead[dwSrc]);
                dwDest++;
            }
            dwSrc++;
        }

        if (dwDest > 0)
        {
            WriteFile(hTFile, &bRead[0], dwDest, &cbWrite, NULL);
        }
    }

    CommonReturn:
        DELETE_OBJECT(pArgs);

        if (hCDFFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hCDFFile);
        }

        if (hTFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hTFile);

            CopyFileU(&wszTFile[0], pwszCDFFile, FALSE);

            DeleteFileU(&wszTFile[0]);
        }

        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
    TRACE_ERROR_EX(DBG_SS_APP, FileError);
}

