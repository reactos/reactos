//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dumpcat.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  wmain
//
//  History:    21-Nov-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

void _DisplayStore(CRYPTCATSTORE *pStore);
void _DisplayMember(CRYPTCATMEMBER *pMember);
void _DisplayAttribute(CRYPTCATATTRIBUTE *pAttr, BOOL fCatalogLevel);

BOOL        fVerbose        = FALSE;
BOOL        fTesting        = FALSE;
DWORD       dwExpectedError = 0;

DWORD       dwTotal         = 0;

WCHAR       *pwszFile       = NULL;

int         iRet            = 0;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    cWArgv_                 *pArgs;
    HANDLE                  hCatStore;
    COleDateTime            tStart;
    COleDateTime            tEnd;
    COleDateTimeSpan        tsTotal;


    hCatStore  = NULL;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,         IDS_PARAMTEXT_HELP,       WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,      IDS_PARAMTEXT_VERBOSE,    WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_EXPERROR,     IDS_PARAMTEXT_EXPERROR,   WARGV_VALUETYPE_DWORDH, NULL, TRUE);

    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto NeededHelp;
    }

    fVerbose        = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));

    if (pArgs->IsSet(IDS_PARAM_EXPERROR))
    {
        dwExpectedError = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_EXPERROR));
        fTesting        = TRUE;
    }

    if (!(pwszFile = pArgs->GetFileName()))
    {
        wprintf(L"%s", pArgs->GetUsageString());
        goto ParamError;
    }

    SetLastError(0);

    //
    //  start our timer
    //
    tStart      = COleDateTime::GetCurrentTime();

    if ((hCatStore = CryptCATOpen(pwszFile, 0, NULL, 0, 0)) == INVALID_HANDLE_VALUE)
    {
        goto CatOpenError;
    }

    CRYPTCATSTORE       *pStore;
    CRYPTCATMEMBER      *pMember;
    CRYPTCATATTRIBUTE   *pAttr;

    printf("\n");
    wprintf(L"\nCatalog File: %s", pwszFile);

    if (pStore = CryptCATStoreFromHandle(hCatStore))
    {
        if (fVerbose)
        {
            _DisplayStore(pStore);
        }
    }

    pAttr = NULL;
    while (pAttr = CryptCATEnumerateCatAttr(hCatStore, pAttr))
    {
        if (fVerbose)
        {
            _DisplayAttribute(pAttr, TRUE);
        }
    }

    pMember = NULL;
    while (pMember = CryptCATEnumerateMember(hCatStore, pMember))
    {
        dwTotal++;

        if (fVerbose)
        {
            _DisplayMember(pMember);
        }

        pAttr = NULL;
        while (pAttr = CryptCATEnumerateAttr(hCatStore, pMember, pAttr))
        {
            if (fVerbose)
            {
                _DisplayAttribute(pAttr, FALSE);
            }
        }
    }

    //
    //  end timer
    //
    tEnd    = COleDateTime::GetCurrentTime();
    tsTotal = tEnd - tStart;

    printf("\n");
    printf("\nTiming:");
    printf("\n  Processing time:    %s", (LPCSTR)tsTotal.Format("%D:%H:%M:%S"));
    printf("\n  Total members:      %lu", dwTotal);
    printf("\n  Average per member: %f", (double)tsTotal.GetTotalSeconds() / (double)dwTotal);
    printf("\n");

    iRet = 0;

    CommonReturn:
        DELETE_OBJECT(pArgs);

        if (hCatStore)
        {
            CryptCATClose(hCatStore);
        }

        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, ParamError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
    TRACE_ERROR_EX(DBG_SS_APP, CatOpenError);
}
void _DisplayStore(CRYPTCATSTORE *pStore)
{
    wprintf(L"\n    Catalog Store Info:");
    wprintf(L"\n        dwPublicVersion:    0x%08.8lX", pStore->dwPublicVersion);
    wprintf(L"\n        dwEncodingType:     0x%08.8lX", pStore->dwEncodingType);
}

void _DisplayMember(CRYPTCATMEMBER *pMember)
{
    wprintf(L"\n        member:             ");

    if ((pMember->pIndirectData) &&
        (pMember->pIndirectData->Digest.pbData))
    {
        DWORD   i;

        for (i = 0; i < pMember->pIndirectData->Digest.cbData; i++)
        {
            printf("%02.2X", pMember->pIndirectData->Digest.pbData[i]);
        }
    }
    else
    {
        BYTE        bEmpty[21];

        memset(&bEmpty[0], ' ', 20);
        bEmpty[20] = 0x00;

        printf("%s", &bEmpty[0]);
    }

    wprintf(L"  %s", pMember->pwszReferenceTag);
}

void _DisplayAttribute(CRYPTCATATTRIBUTE *pAttr, BOOL fCatalogLevel)
{
    if (fCatalogLevel)
    {
        wprintf(L"\n        attribute:          ");
    }
    else
    {
        wprintf(L"\n            attribute:      ");
    }

    wprintf(L"%s   ", pAttr->pwszReferenceTag);

    DWORD   i;

    for (i = 0; i < pAttr->cbValue; i++)
    {
        printf("%c", pAttr->pbValue[i]);
    }
}
