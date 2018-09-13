//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       chains.cpp
//
//  Contents:   Microsoft Internet Security HTTPS Provider Test
//
//  History:    01-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    <windows.h>
#include    <stdio.h>

#include    <wincrypt.h>
#include    <wintrust.h>
#include    <softpub.h>

#include    "unicode.h"

extern "C" int __cdecl wmain(int argc, WCHAR **wargv)
{
    GUID                    guidActionId = HTTPSPROV_ACTION;
    DWORD                   dwReturn;

    WINTRUST_DATA           sWTD;
    WINTRUST_CERT_INFO      sWTCI;

    HCERTSTORE              hStore;
    PCCERT_CONTEXT          pCert;
    HTTPSPolicyCallbackData CBack;

    if (argc < 3)
    {
        printf("\r\nUsage: thttps cert_fname server_name");
        return(0);
    }

    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));

    sWTD.cbStruct               = sizeof(WINTRUST_DATA);
    sWTD.pPolicyCallbackData    = &CBack;
    sWTD.dwUIChoice             = WTD_UI_ALL;
    sWTD.dwUnionChoice          = WTD_CHOICE_CERT;
    sWTD.pCert                  = &sWTCI;

    memset(&sWTCI, 0x00, sizeof(WINTRUST_CERT_INFO));

    sWTCI.cbStruct              = sizeof(WINTRUST_CERT_INFO);
    sWTCI.pcwszDisplayName      = L"HTTPS Certificate Test";

    hStore = CertOpenStore(CERT_STORE_PROV_FILENAME_W, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           NULL, 0, (const void *)wargv[1]);

    if (!(hStore))
    {
        printf("\r\nError: unable to open certificate store");

        return(0);
    }

    pCert = CertEnumCertificatesInStore(hStore, NULL);

    sWTCI.psCertContext = (PCERT_CONTEXT)pCert;

    memset(&CBack, 0x00, sizeof(HTTPSPolicyCallbackData));
    CBack.cbStruct          = sizeof(HTTPSPolicyCallbackData);
    CBack.dwAuthType        = AUTHTYPE_CLIENT;
    CBack.fdwChecks         = 0;
    CBack.pwszServerName    = wargv[2];

    dwReturn = WinVerifyTrust(NULL, &guidActionId, &sWTD);

    return(0);
}
