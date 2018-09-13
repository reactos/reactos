//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wvtcert.cpp
//
//  Contents:   performance suite
//
//  History:    04-Dec-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

void _LoadCerts(PCERT_CONTEXT *ppCerts);

#define _MAX_CERTS      4

DWORD WINAPI TestWVTCert(ThreadData *psData)
{
    COleDateTime            tStart;
    COleDateTime            tEnd;
    DWORD                   i;
    DWORD                   iCert;
    HRESULT                 hr;
    PCCERT_CONTEXT          pcCerts[_MAX_CERTS];
    WINTRUST_DATA           sWTD;
    WINTRUST_CERT_INFO      sWTCC;

    printf("\n  WVT_CERT");

    psData->dwTotalProcessed    = 0;

    _LoadCerts((PCERT_CONTEXT *)&pcCerts[0]);

    memset(&sWTD,   0x00,   sizeof(WINTRUST_DATA));
    memset(&sWTCC,  0x00,   sizeof(WINTRUST_CERT_INFO));

    sWTD.cbStruct           = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice         = WTD_UI_NONE;
    sWTD.dwUnionChoice      = WTD_CHOICE_CERT;
    sWTD.pCert              = &sWTCC;

    sWTCC.cbStruct          = sizeof(WINTRUST_CERT_INFO);
    sWTCC.pcwszDisplayName  = L"WVTCERT";


    tStart = COleDateTime::GetCurrentTime();

    for (i = 0; i < cPasses; i++)
    {
        for (iCert = 0; iCert < _MAX_CERTS; iCert++)
        {
            if (pcCerts[iCert])
            {
                sWTCC.psCertContext = (CERT_CONTEXT *)pcCerts[iCert];

                hr = WinVerifyTrust(NULL, &gCertProvider, &sWTD);

                psData->dwTotalProcessed++;

                if (fVerbose)
                {
                    printf("\n    cert check returned: 0x%08.8lX", hr);
                }
            }
        }
    }

    tEnd = COleDateTime::GetCurrentTime();

    psData->tsTotal             = tEnd - tStart;

    for (i = 0; i < _MAX_CERTS; i++)
    {
        if (pcCerts[i])
        {
            CertFreeCertificateContext(pcCerts[i]);
        }
    }

    return(0);
}


void _LoadCerts(PCERT_CONTEXT *ppCerts)
{
    HRSRC               hrsrc;
    int                 i;
    CRYPT_DATA_BLOB     sBlob;
    PCCERT_CONTEXT      pCert;
    HGLOBAL     hglobRes;
    HCERTSTORE  hResStore;


    for (i = 0; i < (_MAX_CERTS); i++)
    {
        ppCerts[i] = NULL;
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
                UnlockResource(hglobRes);
                FreeResource(hglobRes);
                return;
            }

            i       = 0;
            pCert   = NULL;
            while (((pCert = CertEnumCertificatesInStore(hResStore, pCert)) !=NULL) &&
                    (i < _MAX_CERTS))
            {
                ppCerts[i] = (PCERT_CONTEXT)CertDuplicateCertificateContext(pCert);
                i++;
            }

            CertCloseStore(hResStore, 0);

            UnlockResource(hglobRes);
            FreeResource(hglobRes);
        }
    }
}
