//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catadd.cpp
//
//  Contents:   performance suite
//
//  History:    04-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

WCHAR   *pwszCatFile    = L"FILESET\\TCATNT.CAT";
WCHAR   *pwszCatFile2   = L"FILESET\\DRIVER.CAT";


DWORD WINAPI TestCatAdd(ThreadData *psData)
{
    HCATADMIN       hCatAdmin;
    HCATINFO        hCatInfo;
    COleDateTime    tStart;
    COleDateTime    tEnd;
    DWORD           i;

    printf("\n  CAT_ADD");

    psData->dwTotalProcessed    = 0;;

    if (!(CryptCATAdminAcquireContext(&hCatAdmin, &gDriver, 0)))
    {
        return(0);
    }

    tStart = COleDateTime::GetCurrentTime();

    if (hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatFile, NULL, 0))
    {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
    }

    for (i = 0; i < cPasses; i++)
    {
        if (hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatFile2, NULL, 0))
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }

        if (hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatFile, NULL, 0))
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }

        if (hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatFile2, NULL, 0))
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }

        if (hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatFile, NULL, 0))
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }

        psData->dwTotalProcessed    += 4;
    }

    tEnd = COleDateTime::GetCurrentTime();

    CryptCATAdminReleaseContext(hCatAdmin, 0);

    psData->tsTotal             = tEnd - tStart;

    return(0);
}
