//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       certhlp.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  WTHelperCertIsSelfSigned
//              WTHelperCertFindIssuerCertificate
//
//              *** local functions ***
//
//  History:    20-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"


BOOL WINAPI WTHelperCertIsSelfSigned(DWORD dwEncoding, CERT_INFO *pCert)
{
    if (!(CertCompareCertificateName(dwEncoding, 
                                     &pCert->Issuer,
                                     &pCert->Subject)))
    {
        return(FALSE);
    }

    return(TRUE);
}

PCCERT_CONTEXT WINAPI WTHelperCertFindIssuerCertificate(PCCERT_CONTEXT pChildContext,
                                                        DWORD chStores,
                                                        HCERTSTORE  *pahStores,
                                                        FILETIME *psftVerifyAsOf,
                                                        DWORD dwEncoding,
                                                        DWORD *pdwConfidence,
                                                        DWORD *pdwError)
{
    return(TrustFindIssuerCertificate(pChildContext, dwEncoding, chStores, pahStores, 
                                      psftVerifyAsOf, pdwConfidence, pdwError, 0));
}

