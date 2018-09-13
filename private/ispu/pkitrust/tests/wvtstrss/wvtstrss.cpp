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

typedef struct LOOPDATA_
{
    WCHAR       *pwszFileName;
    GUID        *pgProvider;

    WCHAR       *pwszCatalogFile;
    WCHAR       *pwszTag;

    DWORD       dwExpectedError;

    DWORD       dwStateControl;

} LOOPDATA;

typedef struct CERTDATA_
{
    PCCERT_CONTEXT  pContext;

} CERTDATA;

#define WVTSTRSS_MAX_CERTS      4

CERTDATA    sCerts[WVTSTRSS_MAX_CERTS + 1];

GUID            gAuthCode       = WINTRUST_ACTION_GENERIC_VERIFY_V2;
GUID            gDriver         = DRIVER_ACTION_VERIFY;
GUID            gCertProvider   = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

LOOPDATA    sGeneralTest[] =
{
    L"signing\\bad\\b_dig.cab",     &gAuthCode, NULL,                   NULL,          0x80096010, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\b_dig.exe",     &gAuthCode, NULL,                   NULL,          0x80096010, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\b_dig.ocx",     &gAuthCode, NULL,                   NULL,          0x80096010, WTD_STATEACTION_IGNORE,

    L"signing\\good\\brill.cab",    &gAuthCode, NULL,                   NULL,          0x800b0101, WTD_STATEACTION_IGNORE,
    L"signing\\good\\good.cab",     &gAuthCode, NULL,                   NULL,          0x800b0101, WTD_STATEACTION_IGNORE,
    L"signing\\good\\timstamp.cab", &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,
    L"signing\\good\\b_ok.exe",     &gAuthCode, NULL,                   NULL,          0x800b0101, WTD_STATEACTION_IGNORE,
    L"signing\\good\\signwold.exe", &gAuthCode, NULL,                   NULL,          0x800b0101, WTD_STATEACTION_IGNORE,
    L"signing\\good\\wz_named.exe", &gAuthCode, NULL,                   NULL,          0x800b0101, WTD_STATEACTION_IGNORE,

    L"signing\\good\\b_ok.doc",     &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,
    L"signing\\good\\b_ok.xls",     &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,
    L"signing\\good\\b_ok.ppt",     &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,

    L"signing\\good\\good_pcb.exe", &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,
    L"signing\\good\\good_pcb.cat", &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,
    L"signing\\good\\good_pcb.cab", &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,

    L"signing\\bad\\cert_pcb.cab",  &gAuthCode, NULL,                   NULL,          0x80096004, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\cert_pcb.cat",  &gAuthCode, NULL,                   NULL,          0x80096004, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\cert_pcb.exe",  &gAuthCode, NULL,                   NULL,          0x80096004, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\cert_pcb.doc",  &gAuthCode, NULL,                   NULL,          0x80096004, WTD_STATEACTION_IGNORE,

    L"signing\\bad\\sig_pcb.cab",   &gAuthCode, NULL,                   NULL,          0x80096010, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\sig_pcb.cat",   &gAuthCode, NULL,                   NULL,          0x8009200e, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\sig_pcb.exe",   &gAuthCode, NULL,                   NULL,          0x80096010, WTD_STATEACTION_IGNORE,
    L"signing\\bad\\sig_pcb.doc",   &gAuthCode, NULL,                   NULL,          0x80096010, WTD_STATEACTION_IGNORE,

    L"catalogs\\test.p7s",          &gAuthCode, NULL,                   NULL,                   0, WTD_STATEACTION_IGNORE,

    L"catalogs\\testrev.exe",       &gAuthCode, L"catalogs\\test.p7s",  L"TestSignedEXE",       0, WTD_STATEACTION_VERIFY,
    L"catalogs\\test2.exe",         &gAuthCode, L"catalogs\\test.p7s",  L"TestSignedEXENoAttr", 0, WTD_STATEACTION_VERIFY,
    L"catalogs\\nosntest.cab",      &gAuthCode, L"catalogs\\test.p7s",  L"TestUnsignedCAB",     0, WTD_STATEACTION_VERIFY,
    L"catalogs\\signtest.cab",      &gAuthCode, L"catalogs\\test.p7s",  L"TestSignedCAB",       0, WTD_STATEACTION_VERIFY,
    L"catalogs\\create.bat",        &gAuthCode, L"catalogs\\test.p7s",  L"TestFlat",            0, WTD_STATEACTION_VERIFY,
    L"catalogs\\create.bat",        &gAuthCode, L"catalogs\\test.p7s",  L"TestFlatNotThere", 0x800b0100, WTD_STATEACTION_VERIFY,
    L"catalogs\\create.bat",        &gAuthCode, L"catalogs\\test.p7s",  L"CloseTheHandle",      0, WTD_STATEACTION_CLOSE,

    NULL, NULL, NULL, NULL, 0, NULL, NULL
};

LOOPDATA    sCatalogTest[] =
{
    L"catalogs\\publish.spc",       &gAuthCode, L"catalogs\\test.p7s",  L"publish.spc",     0, WTD_STATEACTION_VERIFY,
    L"catalogs\\publish.pvk",       &gAuthCode, L"catalogs\\test.p7s",  L"publish.pvk",     0, WTD_STATEACTION_VERIFY,
    L"catalogs\\regress.cdf",       &gAuthCode, L"catalogs\\test.p7s",  L"regress.cdf",     0, WTD_STATEACTION_VERIFY,
    L"catalogs\\regress2.cdf",      &gAuthCode, L"catalogs\\test.p7s",  L"regress2.cdf",    0, WTD_STATEACTION_VERIFY,
    L"catalogs\\testrev.exe",       &gAuthCode, L"catalogs\\test.p7s",  L"testrev.exe",     0, WTD_STATEACTION_VERIFY,
    L"catalogs\\test2.exe",         &gAuthCode, L"catalogs\\test.p7s",  L"test2.exe",       0, WTD_STATEACTION_VERIFY,
    L"catalogs\\nosntest.cab",      &gAuthCode, L"catalogs\\test.p7s",  L"nosntest.cab",    0, WTD_STATEACTION_VERIFY,
    L"catalogs\\signtest.cab",      &gAuthCode, L"catalogs\\test.p7s",  L"signtest.cab",    0, WTD_STATEACTION_VERIFY,
    L"catalogs\\create.bat",        &gAuthCode, L"catalogs\\test.p7s",  L"create.bat",      0, WTD_STATEACTION_VERIFY,
    L"catalogs\\create.bat",        &gAuthCode, L"catalogs\\test.p7s",  L"TestFlatNotThere",0, WTD_STATEACTION_VERIFY,
    L"catalogs\\create.bat",        &gAuthCode, L"catalogs\\test.p7s",  L"CloseTheHandle",  0, WTD_STATEACTION_CLOSE,

    NULL, NULL, NULL, NULL, 0, NULL, NULL
};

LOOPDATA    sDriverTest[] =
{
    L"calc.cnt",                    &gDriver,   L"wvtstrss\\dtest.cat", L"calc.cnt",        0, WTD_STATEACTION_VERIFY,
    L"calc.exe",                    &gDriver,   L"wvtstrss\\dtest.cat", L"calc.exe",        0, WTD_STATEACTION_VERIFY,
    L"cmd.exe",                     &gDriver,   L"wvtstrss\\dtest.cat", L"cmd.exe",         0, WTD_STATEACTION_VERIFY,
    L"close",                       &gDriver,   L"close",               L"cmd.exe",         0, WTD_STATEACTION_CLOSE,

    NULL, NULL, NULL, NULL, 0, NULL, NULL
};

void _LoadCerts(void);

HGLOBAL     hglobRes    = NULL;
HCERTSTORE  hResStore   = NULL;

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    cWArgv_                 *pArgs;
    LOOPDATA                *psData;
    LOOPDATA                *psUseTest;
    CERTDATA                *psCerts;
    WINTRUST_DATA           sWTD;
    WINTRUST_FILE_INFO      sWTFI;
    WINTRUST_CATALOG_INFO   sWTCI;
    WINTRUST_CERT_INFO      sWTCC;
    WCHAR                   wszPrePath[MAX_PATH];
    WCHAR                   wszFile[MAX_PATH];
    DWORD                   dwCount;
    HRESULT                 hResult;
    DWORD                   dwTotalFiles;
    int                     i;
    int                     iRet;
    BOOL                    fVerbose;
    BOOL                    fCheckCerts;

    COleDateTime            tStart;
    COleDateTime            tEnd;
    COleDateTimeSpan        tsTotal;

    iRet                = 0;

    dwTotalFiles        = 0;
    dwCount             = 1;
    psUseTest           = &sGeneralTest[0];
    fCheckCerts         = FALSE;
    wszPrePath[0]       = NULL;

    if (!(pArgs = new cWArgv_((HINSTANCE)GetModuleHandle(NULL))))
    {
        goto MemoryError;
    }

    pArgs->AddUsageText(IDS_USAGETEXT_USAGE, IDS_USAGETEXT_OPTIONS,
                        IDS_USAGETEXT_CMDFILE, IDS_USAGETEXT_ADD,
                        IDS_USAGETEXT_OPTPARAM);

    pArgs->Add2List(IDS_PARAM_HELP,         IDS_PARAMTEXT_HELP,         WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_VERBOSE,      IDS_PARAMTEXT_VERBOSE,      WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_COUNT,        IDS_PARAMTEXT_COUNT,        WARGV_VALUETYPE_DWORDD, NULL);
    pArgs->Add2List(IDS_PARAM_CATPREPATH,   IDS_PARAMTEXT_CATPREPATH,   WARGV_VALUETYPE_WCHAR, NULL);
    pArgs->Add2List(IDS_PARAM_TESTCAT,      IDS_PARAMTEXT_TESTCAT,      WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_TESTDRIVER,   IDS_PARAMTEXT_TESTDRIVER,   WARGV_VALUETYPE_BOOL, (void *)FALSE);
    pArgs->Add2List(IDS_PARAM_TESTCERT,     IDS_PARAMTEXT_TESTCERT,     WARGV_VALUETYPE_BOOL, (void *)FALSE);

    if (!(pArgs->Fill(argc, wargv)) ||
        (pArgs->GetValue(IDS_PARAM_HELP)))
    {
        wprintf(L"%s", pArgs->GetUsageString());

        goto NeededHelp;
    }


    fVerbose            = (BOOL)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_VERBOSE));

    if (pArgs->GetValue(IDS_PARAM_CATPREPATH))
    {
        wcscpy(&wszPrePath[0], (WCHAR *)pArgs->GetValue(IDS_PARAM_CATPREPATH));

        if (wszPrePath[wcslen(&wszPrePath[0]) - 1] != L'\\')
        {
            wcscat(&wszPrePath[0], L"\\");
        }
    }

    if (pArgs->GetValue(IDS_PARAM_TESTCAT))
    {
        psUseTest       = &sCatalogTest[0];
    }
    else if (pArgs->GetValue(IDS_PARAM_TESTDRIVER))
    {
        psUseTest       = &sDriverTest[0];
    }
    else if (pArgs->GetValue(IDS_PARAM_TESTCERT))
    {
        psUseTest       = NULL;
        fCheckCerts     = TRUE;

        _LoadCerts();
    }

    if (pArgs->GetValue(IDS_PARAM_COUNT))
    {
        dwCount = (DWORD)((DWORD_PTR)pArgs->GetValue(IDS_PARAM_COUNT));
    }


    memset(&sWTD,   0x00,   sizeof(WINTRUST_DATA));
    memset(&sWTFI,  0x00,   sizeof(WINTRUST_FILE_INFO));
    memset(&sWTCI,  0x00,   sizeof(WINTRUST_CATALOG_INFO));
    memset(&sWTCC,  0x00,   sizeof(WINTRUST_CERT_INFO));

    sWTD.cbStruct           = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice         = WTD_UI_NONE;

    sWTFI.cbStruct          = sizeof(WINTRUST_FILE_INFO);

    sWTCI.cbStruct          = sizeof(WINTRUST_CATALOG_INFO);

    sWTCC.cbStruct          = sizeof(WINTRUST_CERT_INFO);
    sWTCC.pcwszDisplayName  = L"WVTSTRSS";

    //
    //  start our timer
    //
    tStart              = COleDateTime::GetCurrentTime();

    for (i = 0; i < (int)dwCount; i++)
    {
        psData  = psUseTest;

        if (psData)
        {
            while (psData->pwszFileName)
            {
                wcscpy(&wszFile[0], &wszPrePath[0]);
                wcscat(&wszFile[0], psData->pwszFileName);

                sWTD.dwStateAction              = psData->dwStateControl;

                if (psData->pwszCatalogFile)
                {
                    sWTD.dwUnionChoice          = WTD_CHOICE_CATALOG;
                    sWTD.pCatalog               = &sWTCI;
                    sWTCI.pcwszCatalogFilePath  = psData->pwszCatalogFile;
                    sWTCI.pcwszMemberTag        = psData->pwszTag;
                    sWTCI.pcwszMemberFilePath   = &wszFile[0];
                }
                else
                {
                    sWTD.dwUnionChoice          = WTD_CHOICE_FILE;
                    sWTD.pFile                  = &sWTFI;
                    sWTFI.pcwszFilePath         = &wszFile[0];
                }

                hResult = WinVerifyTrust(NULL, psData->pgProvider, &sWTD);

                if (fVerbose)
                {
                    wprintf(L"\nround %d: 0x%08.8x: %s", i, hResult, &wszFile[0]);
                }

                dwTotalFiles++;

                psData++;
            }
        }
        else if (fCheckCerts)
        {
            psCerts = &sCerts[0];

            while (psCerts->pContext)
            {
                sWTD.dwUnionChoice          = WTD_CHOICE_CERT;
                sWTD.pCert                  = &sWTCC;
                sWTCC.psCertContext         = (CERT_CONTEXT *)psCerts->pContext;

                hResult = WinVerifyTrust(NULL, &gCertProvider, &sWTD);

                if (fVerbose)
                {
                    wprintf(L"\nround %d: 0x%08.8x", i, hResult);
                }

                dwTotalFiles++;

                psCerts++;
            }
        }
    }

    tEnd    = COleDateTime::GetCurrentTime();
    tsTotal = tEnd - tStart;

    printf("\n\nTotal files verified:   %ld", dwTotalFiles);
    printf("\nProcessing time:          %s", (LPCSTR)tsTotal.Format("%D:%H:%M:%S"));
    printf("\nAverage seconds per file: %f", (double)tsTotal.GetTotalSeconds() / (double)dwTotalFiles);
    printf("\n");

    CommonReturn:
        DELETE_OBJECT(pArgs);

        for (i = 0; i < WVTSTRSS_MAX_CERTS; i++)
        {
            if (sCerts[i].pContext)
            {
                CertFreeCertificateContext(sCerts[i].pContext);
            }
        }

        if (hResStore)
        {
            CertCloseStore(hResStore, 0);
        }

        if (hglobRes)
        {
            UnlockResource(hglobRes);
            FreeResource(hglobRes);
        }


        return(iRet);

    ErrorReturn:
        iRet = 1;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS_APP, MemoryError);
    TRACE_ERROR_EX(DBG_SS_APP, NeededHelp);
}

void _LoadCerts(void)
{
    HRSRC               hrsrc;
    int                 i;
    CRYPT_DATA_BLOB     sBlob;
    PCCERT_CONTEXT      pCert;

    for (i = 0; i < (WVTSTRSS_MAX_CERTS + 1); i++)
    {
        sCerts[i].pContext = NULL;
    }

    if (hrsrc = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_CERTS), TEXT("CERTS")))
    {
        if (hglobRes = LoadResource(GetModuleHandle(NULL), hrsrc))
        {
            sBlob.cbData = SizeofResource(GetModuleHandle(NULL), hrsrc);
            sBlob.pbData = (BYTE *)LockResource(hglobRes);

            hResStore = CertOpenStore(CERT_STORE_PROV_SERIALIZED,
                                      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                      NULL,
                                      CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                      &sBlob);

            if (!(hResStore))
            {
                return;
            }

            i       = 0;
            pCert   = NULL;
            while ((pCert = CertEnumCertificatesInStore(hResStore, pCert)) !=NULL)
            {
                sCerts[i].pContext = CertDuplicateCertificateContext(pCert);
                i++;
            }
        }
    }
}
