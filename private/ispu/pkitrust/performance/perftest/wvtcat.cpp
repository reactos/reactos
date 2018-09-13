//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wvtcat.cpp
//
//  Contents:   performance suite
//
//  History:    04-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


WVTLOOPDATA saDriverLoopData[] = 
{
    L"FILESET\\SIGNED\\cert_pcb.cab", &gDriver, L"FILESET\\DRIVER.CAT",  L"cert_pcb.cab", WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\good_pcb.cab", &gDriver, L"FILESET\\DRIVER.CAT",  L"good_pcb.cab", WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\sig_pcb.cab",  &gDriver, L"FILESET\\DRIVER.CAT",  L"sig_pcb.cab",  WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\cert_pcb.exe", &gDriver, L"FILESET\\DRIVER.CAT",  L"cert_pcb.exe", WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\good_pcb.exe", &gDriver, L"FILESET\\DRIVER.CAT",  L"good_pcb.exe", WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\sig2_pcb.exe", &gDriver, L"FILESET\\DRIVER.CAT",  L"sig2_pcb.exe", WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\sig3_pcb.exe", &gDriver, L"FILESET\\DRIVER.CAT",  L"sig3_pcb.exe", WTD_STATEACTION_VERIFY,
    L"FILESET\\SIGNED\\sig3_pcb.exe", &gDriver, L"FILESET\\DRIVER.CAT",  L"Handle",       WTD_STATEACTION_CLOSE,

    NULL, NULL, NULL, NULL, 0, NULL, NULL
};

DWORD WINAPI TestWVTCat(ThreadData *psData)
{
    COleDateTime            tStart;
    COleDateTime            tEnd;
    DWORD                   i;

    HRESULT                 hr;
    WINTRUST_DATA           sWTD;
    WINTRUST_CATALOG_INFO   sWTCI;

    WVTLOOPDATA             *psLoop;

    psData->dwTotalProcessed = 0;

    printf("\n  WVT_CAT");

    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));

    sWTD.cbStruct               = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice             = WTD_UI_NONE;
    sWTD.dwUnionChoice          = WTD_CHOICE_CATALOG;
    sWTD.pCatalog               = &sWTCI;

    memset(&sWTCI, 0x00, sizeof(WINTRUST_CATALOG_INFO));
    sWTCI.cbStruct              = sizeof(WINTRUST_CATALOG_INFO);

    tStart = COleDateTime::GetCurrentTime();

    for (i = 0; i < cPasses; i++)
    {
        psLoop = &saDriverLoopData[0];

        while (psLoop->pwszFileName)
        {
            sWTD.dwStateAction          = psLoop->dwStateControl;

            sWTCI.pcwszCatalogFilePath  = psLoop->pwszCatalogFile;
            sWTCI.pcwszMemberTag        = psLoop->pwszTag;
            sWTCI.pcwszMemberFilePath   = psLoop->pwszFileName;
            
            hr = WinVerifyTrust(NULL, psLoop->pgProvider, &sWTD);

            if (fVerbose)
            {
                wprintf(L"\nWVT returned 0x%08.8x: %s", hr, psLoop->pwszFileName);
            }

            psData->dwTotalProcessed++;

            psLoop++;
        }
    }

    tEnd = COleDateTime::GetCurrentTime();

    psData->tsTotal             = tEnd - tStart;

    return(0);
}
