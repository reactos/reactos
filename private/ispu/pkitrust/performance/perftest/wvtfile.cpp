//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wvtfile.cpp
//
//  Contents:   performance suite
//
//  History:    04-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

DWORD WINAPI TestWVTFile(ThreadData *psData)
{
    COleDateTime            tStart;
    COleDateTime            tEnd;
    DWORD                   i;
    char                    szFile[MAX_PATH];

    HRESULT                 hr;
    BOOL                    fFind;
    HANDLE                  hFind;
    WIN32_FIND_DATA         sFindData;
    WCHAR                   *pwszLastSlash;
    WCHAR                   wszDir[MAX_PATH];
    WCHAR                   wszFile[MAX_PATH];
    DWORD                   dwDirLen;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;


    hFind   = INVALID_HANDLE_VALUE;

    psData->dwTotalProcessed = 0;

    printf("\n  WVT_FILE");

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

    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));

    sWTD.cbStruct           = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice         = WTD_UI_NONE;
    sWTD.dwUnionChoice      = WTD_CHOICE_FILE;
    sWTD.pFile              = &sWTFI;

    memset(&sWTFI, 0x00, sizeof(WINTRUST_FILE_INFO));

    sWTFI.cbStruct          = sizeof(WINTRUST_FILE_INFO);

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

                sWTFI.pcwszFilePath     = &wszFile[0];

                hr = WinVerifyTrust(NULL, &gAuthCode, &sWTD);

                if (fVerbose)
                {
                    printf("\n   WVT return: 0x%08.8lX - %s", hr, &sFindData.cFileName[0]);
                }

                psData->dwTotalProcessed++;
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

