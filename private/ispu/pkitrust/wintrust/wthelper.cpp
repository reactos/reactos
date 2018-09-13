//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wthelper.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  WTHelperGetProvPrivateDataFromChain
//              WTHelperGetProvSignerFromChain
//              WTHelperGetFileHandle
//              WTHelperGetFileName
//              WTHelperOpenKnownStores
//              WTHelperGetProvCertFromChain
//              WTHelperCheckCertUsage
//              WTHelperIsInRootStore
//              WTHelperProvDataFromStateData
//              WTHelperGetAgencyInfo
//
//              *** local functions ***
//              _FindKeyUsage
//
//  History:    01-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "pkialloc.h"

BOOL _FindKeyUsage(PCERT_ENHKEY_USAGE  pUsage, LPCSTR pszRequestedUsageOID);

CRYPT_PROVIDER_PRIVDATA * WINAPI WTHelperGetProvPrivateDataFromChain(CRYPT_PROVIDER_DATA *pProvData,
                                                                            GUID *pgProviderID)
{
    if (!(pProvData) ||
        !(pgProviderID))
    {
        return(NULL);
    }

    for (int i = 0; i < (int)pProvData->csProvPrivData; i++)
    {
        if (memcmp(&pProvData->pasProvPrivData[i].gProviderID, pgProviderID, sizeof(GUID)) == 0)
        {
            return(&pProvData->pasProvPrivData[i]);
        }
    }

    return(NULL);
}


CRYPT_PROVIDER_SGNR * WINAPI WTHelperGetProvSignerFromChain(CRYPT_PROVIDER_DATA *pProvData,
                                                            DWORD idxSigner,
                                                            BOOL fCounterSigner,
                                                            DWORD idxCounterSigner)
{
    if (!(pProvData) ||
        (idxSigner >= pProvData->csSigners))
    {
        return(NULL);
    }

    if (fCounterSigner)
    {
        if (idxCounterSigner >= pProvData->pasSigners[idxSigner].csCounterSigners)
        {
            return(NULL);
        }

        return(&pProvData->pasSigners[idxSigner].pasCounterSigners[idxCounterSigner]);
    }

    return(&pProvData->pasSigners[idxSigner]);
}

CRYPT_PROVIDER_CERT * WINAPI WTHelperGetProvCertFromChain(CRYPT_PROVIDER_SGNR *pSgnr,
                                                          DWORD idxCert)
{
    if (!(pSgnr) ||
        (idxCert >= pSgnr->csCertChain))
    {
        return(NULL);
    }

    return(&pSgnr->pasCertChain[idxCert]);
}

HANDLE WINAPI WTHelperGetFileHandle(WINTRUST_DATA *pWintrustData)
{
    switch (pWintrustData->dwUnionChoice)
    {
        case WTD_CHOICE_FILE:
                return(pWintrustData->pFile->hFile);

        case WTD_CHOICE_CATALOG:
                return(pWintrustData->pCatalog->hMemberFile);
    }

    return(INVALID_HANDLE_VALUE);
}


WCHAR * WINAPI WTHelperGetFileName(WINTRUST_DATA *pWintrustData)
{
    switch (pWintrustData->dwUnionChoice)
    {
        case WTD_CHOICE_FILE:
                return((WCHAR *)pWintrustData->pFile->pcwszFilePath);

        case WTD_CHOICE_CATALOG:
                if (!(pWintrustData->pCatalog->pcwszCatalogFilePath) ||
                    !(pWintrustData->pCatalog->pcwszMemberTag))
                {
                    return(NULL);
                }
                return((WCHAR *)pWintrustData->pCatalog->pcwszMemberFilePath);
        case WTD_CHOICE_CERT:
                if (pWintrustData->pCert->pcwszDisplayName)
                    return (WCHAR *) pWintrustData->pCert->pcwszDisplayName;
                else
                    return L"Certificate";
    }

    return(NULL);
}

BOOL WINAPI WTHelperOpenKnownStores(CRYPT_PROVIDER_DATA *pProvData)
{
    DWORD       i;
    DWORD       cs;
    HCERTSTORE  *pas;

    if ((pProvData->pWintrustData) &&
        (pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CERT) &&
        (_ISINSTRUCT(WINTRUST_CERT_INFO, pProvData->pWintrustData->pCert->cbStruct, dwFlags)))
    {
        HCERTSTORE  hStore;

        if (pProvData->pWintrustData->pCert->dwFlags & WTCI_DONT_OPEN_STORES)
        {
            if (hStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL))
            {
                AddToStoreChain(hStore, &pProvData->chStores, &pProvData->pahStores);
                return(TRUE);
            }

            return(FALSE);
        }

        if (pProvData->pWintrustData->pCert->dwFlags & WTCI_OPEN_ONLY_ROOT)
        {
            if (hStore = StoreProviderGetStore(pProvData->hProv, WVT_STOREID_ROOT))
            {
                AddToStoreChain(hStore, &pProvData->chStores, &pProvData->pahStores);
                return(TRUE);
            }

            return(FALSE);
        }
    }

    cs = 0;
    TrustOpenStores(pProvData->hProv, &cs, NULL, 0);

    if (cs > 0)
    {
        if (!(pas = new HCERTSTORE[cs]))
        {
            pProvData->dwError = ERROR_NOT_ENOUGH_MEMORY;
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_SYSTEM_ERROR;
            return(FALSE);
        }

        if (TrustOpenStores(pProvData->hProv, &cs, pas, 0))
        {
            
            for (i = 0; i < cs; i++)
            {
                AddToStoreChain(pas[i], &pProvData->chStores, &pProvData->pahStores);
            }
        }
        else
        {
            cs = 0;
        }

        delete pas;
    }

    if (cs > 0)
    {
        return(TRUE);
    }

    return(FALSE);
}

BOOL WINAPI WTHelperGetAgencyInfo(PCCERT_CONTEXT pCert, DWORD *pcbAgencyInfo, SPC_SP_AGENCY_INFO *pAgencyInfo)
{
    PCERT_EXTENSION     pExt;
    PSPC_SP_AGENCY_INFO pInfo;
    DWORD               cbInfo;


    if (!(pCert) || !(pcbAgencyInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    *pcbAgencyInfo = 0;

    if (!(pExt = CertFindExtension(SPC_SP_AGENCY_INFO_OBJID, pCert->pCertInfo->cExtension,
                                   pCert->pCertInfo->rgExtension)))
    {
        return(FALSE);
    }

    CryptDecodeObject(X509_ASN_ENCODING, SPC_SP_AGENCY_INFO_STRUCT,
                     pExt->Value.pbData, pExt->Value.cbData, 0, NULL,
                     pcbAgencyInfo);

    if (*pcbAgencyInfo == 0) 
    {
        return(FALSE);
    }

    if (!(pAgencyInfo))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    if (!(CryptDecodeObject(X509_ASN_ENCODING, SPC_SP_AGENCY_INFO_STRUCT,
                            pExt->Value.pbData, pExt->Value.cbData, 0, pAgencyInfo,
                            pcbAgencyInfo)))
    {
        return(FALSE);
    } 

    return(TRUE);
}

BOOL WINAPI WTHelperCheckCertUsage(PCCERT_CONTEXT pCertContext, LPCSTR pszRequestedUsageOID)
{
    PCERT_ENHKEY_USAGE  pUsage;
    DWORD               cbUsage;
    int                 i;

    cbUsage = 0;

    CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, NULL, &cbUsage);

    if (cbUsage > 0)
    {
        if (!(pUsage = (PCERT_ENHKEY_USAGE)new BYTE[cbUsage]))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        if (!(CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                      pUsage, &cbUsage)))
        {
            delete pUsage;
            return(FALSE);
        }

        //
        // the cert has enhanced key usage extensions, check if we find ours
        //
        if (!(_FindKeyUsage(pUsage, pszRequestedUsageOID)))
        {
            SetLastError(CERT_E_WRONG_USAGE);
    
            delete pUsage;
            return(FALSE);
        }

        delete pUsage;
    }


    //
    //  OK... either we have NO EXTENSION or we found our OID in the list in the EXTENSION.
    //  now, make sure if we have properties that it has been enabled.
    //
    cbUsage = 0;
    CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, NULL, &cbUsage);

    if (cbUsage > 0)
    {
        if (!(pUsage = (PCERT_ENHKEY_USAGE)new BYTE[cbUsage]))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        if (!(CertGetEnhancedKeyUsage(pCertContext, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                                      pUsage, &cbUsage)))
        {
            delete pUsage;

            return(FALSE);
        }

        //
        //  the cert has properties, first check if we're disabled
        //
        if (_FindKeyUsage(pUsage, szOID_YESNO_TRUST_ATTR))
        {
            SetLastError(CERT_E_WRONG_USAGE);
    
            delete pUsage;
            return(FALSE);
        }

        if (!(_FindKeyUsage(pUsage, pszRequestedUsageOID)))
        {
            SetLastError(CERT_E_WRONG_USAGE);
    
            delete pUsage;
            return(FALSE);
        }
        
        delete pUsage;
    }

    return(TRUE);
}

BOOL _FindKeyUsage(PCERT_ENHKEY_USAGE  pUsage, LPCSTR pszRequestedUsageOID)
{
    int     i;

    for (i = 0; i < (int)pUsage->cUsageIdentifier; i++)
    {
        if (strcmp(pUsage->rgpszUsageIdentifier[i], pszRequestedUsageOID) == 0)
        {
            return(TRUE);   // OK found it!
        }
    }

    return(FALSE);
}

BOOL WINAPI WTHelperIsInRootStore(CRYPT_PROVIDER_DATA *pProvData, PCCERT_CONTEXT pCertContext)
{
    if (pProvData->chStores < 1)
    {
        return(FALSE);
    }

    //
    //  check the fast way first!
    //
    if (pCertContext->hCertStore == pProvData->pahStores[0])
    {
        //
        //  it's in the root store!
        //
        return(TRUE);
    }

    //
    //  can't do it the fast way -- do it the slow way!
    //
    BYTE            *pbHash;
    DWORD           cbHash;
    CRYPT_HASH_BLOB sBlob;
    PCCERT_CONTEXT  pWorkContext;

    cbHash = 0;

	if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, NULL, &cbHash)))
    {
        return(FALSE);
    }

    if (cbHash < 1)
    {
        return(FALSE);
    }

    if (!(pbHash = (BYTE *)WVTNew(cbHash)))
    {
        return(FALSE);
    }

	if (!(CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, pbHash, &cbHash)))
    {
        delete pbHash;
        return(FALSE);
    }

    sBlob.cbData    = cbHash;
    sBlob.pbData    = pbHash;

    pWorkContext = CertFindCertificateInStore(pProvData->pahStores[0], pProvData->dwEncoding, 0,
                                              CERT_FIND_SHA1_HASH, &sBlob, NULL);

    delete pbHash;

    if (pWorkContext)
    {
        CertFreeCertificateContext(pWorkContext);
        return(TRUE);
    }

    return(FALSE);
}


typedef struct _ENUM_OID_INFO_ARG {
    DWORD               cOidInfo;
    PCCRYPT_OID_INFO    *ppOidInfo; 
} ENUM_OID_INFO_ARG, *PENUM_OID_INFO_ARG;

static BOOL WINAPI EnumOidInfoCallback(
    IN PCCRYPT_OID_INFO pOidInfo,
    IN void *pvArg
    )
{
    PENUM_OID_INFO_ARG pEnumOidInfoArg = (PENUM_OID_INFO_ARG) pvArg;

    PCCRYPT_OID_INFO *ppNewOidInfo;
    DWORD cOidInfo = pEnumOidInfoArg->cOidInfo;

    if (ppNewOidInfo = (PCCRYPT_OID_INFO *) PkiRealloc(
            pEnumOidInfoArg->ppOidInfo,
            (cOidInfo + 2) * sizeof(PCCRYPT_OID_INFO))) {
        ppNewOidInfo[cOidInfo] = pOidInfo;
        ppNewOidInfo[cOidInfo + 1] = NULL;
        pEnumOidInfoArg->cOidInfo = cOidInfo + 1;
        pEnumOidInfoArg->ppOidInfo = ppNewOidInfo;
    }

    return TRUE;
}

BOOL WINAPI WTHelperGetKnownUsages(DWORD fdwAction, PCCRYPT_OID_INFO **pppOidInfo)
{


    if (!(pppOidInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (fdwAction == WTH_FREE)
    {
        PkiFree(*pppOidInfo);
        *pppOidInfo = NULL;

        return(TRUE);
    }

    if (fdwAction == WTH_ALLOC)
    {
        ENUM_OID_INFO_ARG EnumOidInfoArg;
        memset(&EnumOidInfoArg, 0, sizeof(EnumOidInfoArg));

        CryptEnumOIDInfo(
            CRYPT_ENHKEY_USAGE_OID_GROUP_ID,
            0,              // dwFlags
            &EnumOidInfoArg,
            EnumOidInfoCallback
            );

        return (NULL != (*pppOidInfo = EnumOidInfoArg.ppOidInfo));
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    *pppOidInfo = NULL;
    return(FALSE);
}

CRYPT_PROVIDER_DATA * WINAPI WTHelperProvDataFromStateData(HANDLE hStateData)
{
    return((CRYPT_PROVIDER_DATA *)hStateData);
}
