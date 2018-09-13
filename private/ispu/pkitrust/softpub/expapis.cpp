//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       expapis.cpp
//
//  Contents:   Microsoft Internet Security Trust Provider
//
//  Functions:  FindCertsByIssuer
//
//  History:    01-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
/////////////////////////////////////////////////////////////////////////////

//
// The root of the certificate store that we manage.
//
#define HEAPALLOC(size)  HeapAlloc ( GetProcessHeap(), 0, size )
#define HEAPFREE(data)   HeapFree  ( GetProcessHeap(), 0, data )


#define SZIE30CERTCLIENTAUTH "Software\\Microsoft\\Cryptography\\PersonalCertificates\\ClientAuth"
#define SZIE30TAGS       "CertificateTags"
#define SZIE30AUXINFO        "CertificateAuxiliaryInfo"
#define SZIE30CERTBUCKET     "Certificates"

#define ALIGN_LEN(Len)  ((Len + 7) & ~7)

#define IE30CONVERTEDSTORE  "My"

static LPCSTR rgpszMyStore[] = {
    "My"
};
#define NMYSTORES (sizeof(rgpszMyStore)/sizeof(rgpszMyStore[0]))

static const struct {
    LPCSTR      pszStore;
    DWORD       dwFlags;
} rgCaStoreInfo[] = {
    "ROOT",     CERT_SYSTEM_STORE_CURRENT_USER,
    "CA",       CERT_SYSTEM_STORE_CURRENT_USER,
    "SPC",      CERT_SYSTEM_STORE_LOCAL_MACHINE
};
#define NCASTORES (sizeof(rgCaStoreInfo)/sizeof(rgCaStoreInfo[0]))

#define MAX_CHAIN_LEN   16
typedef struct _CHAIN_INFO CHAIN_INFO, *PCHAIN_INFO;
struct _CHAIN_INFO {
    DWORD           cCert;
    PCCERT_CONTEXT  rgpCert[MAX_CHAIN_LEN];
    DWORD           cbKeyProvInfo;          // aligned
    DWORD           cbCert;                 // aligned
    PCHAIN_INFO     pNext;
};

//+-------------------------------------------------------------------------
//  AuthCert allocation and free functions
//--------------------------------------------------------------------------
static void *ACAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = (void *)new BYTE[cbBytes];
    if (pv == NULL)
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return pv;
}
static void ACFree(
    IN void *pv
    )
{
    if (pv)
    {
        delete pv;
    }
}

static HRESULT GetAndIe30ClientAuthCertificates(HCERTSTORE hStore)
// Check for and copy any existing certificates stored in Bob's
// certificate store.
{
    HRESULT hr = S_OK;
    LONG Status;
    HKEY hKeyRoot   = NULL;
    HKEY hKeyBucket = NULL;
    HKEY hKeyTags   = NULL;
    HKEY hKeyAux    = NULL;

    if (ERROR_SUCCESS != RegOpenKeyExA(
            HKEY_CURRENT_USER,
            SZIE30CERTCLIENTAUTH,
            0,                  // dwReserved
            KEY_READ,
            &hKeyRoot
            ))
        return S_OK;

    // Copy any existing certificates
    if (ERROR_SUCCESS == RegOpenKeyExA(
            hKeyRoot,
            SZIE30CERTBUCKET,
            0,                  // dwReserved
            KEY_READ,
            &hKeyBucket
        )               &&

        ERROR_SUCCESS == RegOpenKeyExA(
            hKeyRoot,
            SZIE30AUXINFO,
            0,                  // dwReserved
            KEY_READ,
            &hKeyAux
            )               &&

        ERROR_SUCCESS == RegOpenKeyExA(
            hKeyRoot,
            SZIE30TAGS,
            0,                  // dwReserved
            KEY_READ,
            &hKeyTags
            )) {

            DWORD   cValuesCert, cchMaxNameCert, cbMaxDataCert;
            DWORD   cValuesTag, cchMaxNameTag, cbMaxDataTag;
            DWORD   cValuesAux, cchMaxNameAux, cbMaxDataAux;
            LPSTR   szName = NULL;
            BYTE *pbDataCert = NULL;
            BYTE *pbDataAux = NULL;
            BYTE *pbDataTag = NULL;


            // see how many and how big the registry is
            if (ERROR_SUCCESS != RegQueryInfoKey(
                        hKeyBucket,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &cValuesCert,
                        &cchMaxNameCert,
                        &cbMaxDataCert,
                        NULL,
                        NULL
                        )           ||
                ERROR_SUCCESS != RegQueryInfoKey(
                        hKeyTags,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &cValuesTag,
                        &cchMaxNameTag,
                        &cbMaxDataTag,
                        NULL,
                        NULL
                        )           ||
                ERROR_SUCCESS != RegQueryInfoKey(
                        hKeyAux,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &cValuesAux,
                        &cchMaxNameAux,
                        &cbMaxDataAux,
                        NULL,
                        NULL
                        ))
                    hr = SignError();
            else {
                // allocate the memory needed to read the reg
                szName = (LPSTR) HEAPALLOC(cchMaxNameCert + 1);
                pbDataCert = (BYTE *) HEAPALLOC(cbMaxDataCert);
                pbDataTag = (BYTE *) HEAPALLOC(cbMaxDataTag);
                pbDataAux = (BYTE *) HEAPALLOC(cbMaxDataAux);
                
                if (NULL == szName      ||
                    NULL == pbDataCert  ||
                    NULL == pbDataAux   ||
                    NULL == pbDataTag   )
                    hr = E_OUTOFMEMORY;
            }

        // enum the registry getting certs
        for (DWORD i = 0; SUCCEEDED(hr) && i < cValuesCert; i++ ) {

            DWORD dwType;
            BYTE *  pb;
            CRYPT_KEY_PROV_INFO   keyInfo;
            DWORD cchName = cchMaxNameCert + 1;
            DWORD cbDataCert = cbMaxDataCert;
            DWORD cbDataTag = cbMaxDataTag;
            DWORD cbDataAux = cbMaxDataAux;

            PCCERT_CONTEXT pCertContxt = NULL;

            // don't have to worry about errors, just skip
            // sliently just be cause there is an internal
            // error in the registry doesn't mean we should
            // get all upset about it.

            // get the cert
            if (RegEnumValueA(
                hKeyBucket,
                i,
                szName,
                &cchName,
                NULL,
                &dwType,
                pbDataCert,
                &cbDataCert
                ) == ERROR_SUCCESS      &&

                dwType == REG_BINARY    &&

            // get the cert context
            (pCertContxt = CertCreateCertificateContext(
                X509_ASN_ENCODING,
                pbDataCert,
                cbDataCert)) != NULL        &&

            // get the tag
            RegQueryValueExA(
                hKeyTags,
                szName,
                NULL,
                &dwType,
                pbDataTag,
                &cbDataTag) == ERROR_SUCCESS    &&

            // get the aux info
            RegQueryValueExA(
                hKeyAux,
                (LPTSTR) pbDataTag,
                NULL,
                &dwType,
                pbDataAux,
                &cbDataAux) == ERROR_SUCCESS ) {

                // aux info is
                // wszPurpose
                // wszProvider
                // wszKeySet
                // wszFilename
                // wszCredentials
                // dwProviderType
                // dwKeySpec

                pb = pbDataAux;
                memset(&keyInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

                // skip purpose, should be client auth
                pb += (lstrlenW((LPWSTR) pb) + 1) * sizeof(WCHAR);

                // get the provider
                keyInfo.pwszProvName = (LPWSTR) pb;
                pb += (lstrlenW((LPWSTR) pb) + 1) * sizeof(WCHAR);

                // get the container name
                keyInfo.pwszContainerName = (LPWSTR) pb;
                pb += (lstrlenW((LPWSTR) pb) + 1) * sizeof(WCHAR);

                // skip filename, should be '\0'
                pb += (lstrlenW((LPWSTR) pb) + 1) * sizeof(WCHAR);

                // skip credential, don't really know what it is?
                pb += (lstrlenW((LPWSTR) pb) + 1) * sizeof(WCHAR);

                // get the provider type
                keyInfo.dwProvType = *((DWORD *) pb);
                pb += sizeof(DWORD);

                // get the key spec
                keyInfo.dwKeySpec  = *((DWORD *) pb);

                // add the property to the certificate
                if( !CertSetCertificateContextProperty(
                    pCertContxt,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    0,
                    &keyInfo)           ||

                !CertAddCertificateContextToStore(
                    hStore,
                    pCertContxt,
                    CERT_STORE_ADD_USE_EXISTING,
                    NULL                            // ppStoreContext
                    )) {

                    MessageBox(
                        NULL,
                        "Copy Certificate Failed",
                        NULL,
                        MB_OK);


                   hr = SignError();
                }
            }

            if(pCertContxt != NULL)
                CertFreeCertificateContext(pCertContxt);
        }

        if (szName)
            HEAPFREE(szName);
        if (pbDataCert)
            HEAPFREE(pbDataCert);
        if(pbDataAux)
            HEAPFREE(pbDataAux);
        if(pbDataTag)
            HEAPFREE(pbDataTag);
    }


    if(hKeyRoot != NULL)
        RegCloseKey(hKeyRoot);
    if(hKeyBucket != NULL)
        RegCloseKey(hKeyBucket);
    if(hKeyTags != NULL)
        RegCloseKey(hKeyTags);
    if(hKeyAux != NULL)
        RegCloseKey(hKeyAux);
    if (FAILED(hr))
        return hr;

    return hr;
}


// Return List is Null terminated
static HCERTSTORE * GetMyStoreList()
{
    int i;
    HCERTSTORE *phStoreList;
    if (NULL == (phStoreList = (HCERTSTORE *) ACAlloc(
            sizeof(HCERTSTORE) * (NMYSTORES + 1))))
        return NULL;
    memset(phStoreList, 0, sizeof(HCERTSTORE) * (NMYSTORES + 1));
    for (i = 0; i < NMYSTORES; i++) {
    if (NULL == (phStoreList[i] = CertOpenSystemStore(
        NULL,
                rgpszMyStore[i])))
            goto ErrorReturn;
    }
    goto CommonReturn;

ErrorReturn:
    for (i = 0; i < NMYSTORES; i++) {
        if (phStoreList[i])
            CertCloseStore(phStoreList[i], 0);
    }

    ACFree(phStoreList);
    phStoreList = NULL;

CommonReturn:
    return phStoreList;
}

static HCERTSTORE * GetCaStoreList()
{
    int i;
    int cStore;
    HCERTSTORE *phStoreList;
    if (NULL == (phStoreList = (HCERTSTORE *) ACAlloc(
            sizeof(HCERTSTORE) * (NCASTORES + 1))))
        return NULL;
    memset(phStoreList, 0, sizeof(HCERTSTORE) * (NCASTORES + 1));

    cStore = 0;
    for (i = 0; i < NCASTORES; i++) {
        DWORD dwFlags;

        dwFlags = rgCaStoreInfo[i].dwFlags | CERT_STORE_READONLY_FLAG;
        if (phStoreList[cStore] = CertOpenStore(
                CERT_STORE_PROV_SYSTEM_A,
                0,                          // dwEncodingType
                0,                          // hCryptProv
                dwFlags,
                (const void *) rgCaStoreInfo[i].pszStore
                ))
            cStore++;
    }
    return phStoreList;
}

// Find first Issuer match. Don't verify anything. Returns TRUE if an
// issuer was found. For a self-signed issuer returns TRUE with *ppIssuer
// set to NULL.
static BOOL GetIssuer(
    IN PCCERT_CONTEXT pSubject,
    IN HCERTSTORE *phCaStoreList,
    OUT PCCERT_CONTEXT *ppIssuer
    )
{
    BOOL fResult = FALSE;
    PCCERT_CONTEXT pIssuer = NULL;
    HCERTSTORE hStore;
    while (hStore = *phCaStoreList++) {
        DWORD dwFlags = 0;
        pIssuer = CertGetIssuerCertificateFromStore(
            hStore,
            pSubject,
            NULL,       // pPrevIssuer,
            &dwFlags
            );
        if (pIssuer || GetLastError() == CRYPT_E_SELF_SIGNED) {
            fResult = TRUE;
            break;
        }
    }

    *ppIssuer = pIssuer;
    return fResult;
}

//+-------------------------------------------------------------------------
// If issuer name matches any cert in the chain, return allocated
// chain info. Otherwise, return NULL.
//
// If pbEncodedIssuerName == NULL || cbEncodedIssuerName = 0, match any
// issuer.
//--------------------------------------------------------------------------
static PCHAIN_INFO CreateChainInfo(
    IN PCCERT_CONTEXT pCert,
    IN BYTE *pbEncodedIssuerName,
    IN DWORD cbEncodedIssuerName,
    IN HCERTSTORE *phCaStoreList,
    IN HCERTSTORE *phMyStoreList
    )
{
    BOOL fIssuerMatch = FALSE;
    DWORD cCert = 1;
    DWORD cbCert = 0;
    PCHAIN_INFO pChainInfo;
    if (NULL == (pChainInfo = (PCHAIN_INFO) ACAlloc(sizeof(CHAIN_INFO))))
        return NULL;
    memset(pChainInfo, 0, sizeof(CHAIN_INFO));
    pChainInfo->rgpCert[0] = CertDuplicateCertificateContext(pCert);

    if (pbEncodedIssuerName == NULL)
        cbEncodedIssuerName = 0;

    while (pCert) {
        PCCERT_CONTEXT pIssuer;
        cbCert += ALIGN_LEN(pCert->cbCertEncoded);
        if (!fIssuerMatch) {
            if (cbEncodedIssuerName == 0 ||
                (cbEncodedIssuerName == pCert->pCertInfo->Issuer.cbData &&
                    memcmp(pbEncodedIssuerName,
                        pCert->pCertInfo->Issuer.pbData,
                        cbEncodedIssuerName) == 0))
                fIssuerMatch = TRUE;
        }
        if (GetIssuer(pCert, phCaStoreList, &pIssuer) ||
                GetIssuer(pCert, phMyStoreList, &pIssuer)) {
            pCert = pIssuer;
            if (pCert) {
                assert (cCert < MAX_CHAIN_LEN);
                if (cCert < MAX_CHAIN_LEN)
                    pChainInfo->rgpCert[cCert++] = pCert;
                else {
                    CertFreeCertificateContext(pCert);
                    pCert = NULL;
                }
            }
            // else
            //  Self-signed
        }
        else
            pCert = NULL;
    }

    if (fIssuerMatch) {
        pChainInfo->cCert = cCert;
        pChainInfo->cbCert = cbCert;
        return pChainInfo;
    } else {
        while (cCert--)
            CertFreeCertificateContext(pChainInfo->rgpCert[cCert]);
        ACFree(pChainInfo);
        return NULL;
    }
}

//+-------------------------------------------------------------------------
//  Check if the certificate has key provider information.
//  If dwKeySpec != 0, also check that the provider's public key matches the
//  public key in the certificate.
//--------------------------------------------------------------------------
static BOOL CheckKeyProvInfo(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwKeySpec,
    OUT DWORD *pcbKeyProvInfo
    )
{
    BOOL fResult = FALSE;
    HCRYPTPROV hCryptProv = 0;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbKeyProvInfo;
    DWORD cbData;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;

    cbKeyProvInfo = 0;
    CertGetCertificateContextProperty(
        pCert,
        CERT_KEY_PROV_INFO_PROP_ID,
        NULL,                           // pvData
        &cbKeyProvInfo
        );
    if (cbKeyProvInfo) {
        if (dwKeySpec == 0)
            fResult = TRUE;
        else {
            DWORD dwIdx;
            if (NULL == (pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) ACAlloc(cbKeyProvInfo)))
                goto CommonReturn;
            if (!CertGetCertificateContextProperty(
                    pCert,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    pKeyProvInfo,
                    &cbKeyProvInfo
                    )) goto CommonReturn;
            if (!CryptAcquireContextU(
                    &hCryptProv,
                    pKeyProvInfo->pwszContainerName,
                    pKeyProvInfo->pwszProvName,
                    pKeyProvInfo->dwProvType,
                    pKeyProvInfo->dwFlags & ~CERT_SET_KEY_PROV_HANDLE_PROP_ID
                    )) {
                hCryptProv = NULL;
                goto CommonReturn;
            }
            for (dwIdx = 0; dwIdx < pKeyProvInfo->cProvParam; dwIdx++) {
                PCRYPT_KEY_PROV_PARAM pKeyProvParam =
                    &pKeyProvInfo->rgProvParam[dwIdx];
                if (!CryptSetProvParam(
                        hCryptProv,
                        pKeyProvParam->dwParam,
                        pKeyProvParam->pbData,
                        pKeyProvParam->dwFlags
                        )) goto CommonReturn;
            }

            // Get public key to compare certificate with
            cbPubKeyInfo = 0;
            CryptExportPublicKeyInfo(
                hCryptProv,
                dwKeySpec,
                pCert->dwCertEncodingType,
                NULL,               // pPubKeyInfo
                &cbPubKeyInfo
                );
            if (cbPubKeyInfo == 0) goto CommonReturn;
            if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) ACAlloc(
                    cbPubKeyInfo)))
                goto CommonReturn;
            if (!CryptExportPublicKeyInfo(
                    hCryptProv,
                    dwKeySpec,
                    pCert->dwCertEncodingType,
                    pPubKeyInfo,
                    &cbPubKeyInfo
                    )) goto CommonReturn;
            fResult = CertComparePublicKeyInfo(
                    pCert->dwCertEncodingType,
                    &pCert->pCertInfo->SubjectPublicKeyInfo,
                    pPubKeyInfo);
        }
    }
CommonReturn:
    if (hCryptProv) {
        DWORD dwErr = GetLastError();
        CryptReleaseContext(hCryptProv, 0);
        SetLastError(dwErr);
    }
    if (pKeyProvInfo)
        ACFree(pKeyProvInfo);
    if (pPubKeyInfo)
        ACFree(pPubKeyInfo);
    *pcbKeyProvInfo = cbKeyProvInfo;
    return fResult;
}


//+-------------------------------------------------------------------------
//  Find all certificate chains tying the given issuer name to any certificate
//  that the current user has a private key for.
//
//  If pbEncodedIssuerName == NULL || cbEncodedIssuerName = 0, match any
//  issuer.
//--------------------------------------------------------------------------
HRESULT
WINAPI
FindCertsByIssuer(
    OUT PCERT_CHAIN pCertChains,
    IN OUT DWORD *pcbCertChains,
    OUT DWORD *pcCertChains,        // count of certificates chains returned
    IN BYTE* pbEncodedIssuerName,   // DER encoded issuer name
    IN DWORD cbEncodedIssuerName,   // count in bytes of encoded issuer name
    IN LPCWSTR pwszPurpose,         // "ClientAuth" or "CodeSigning"
    IN DWORD dwKeySpec              // only return signers supporting this
                                    // keyspec

    )
{
    HRESULT hr;
    HCERTSTORE *phMyStoreList = NULL;
    HCERTSTORE *phCaStoreList = NULL;
    HCERTSTORE *phStore;
    HCERTSTORE hStore;

    DWORD cChain = 0;
    DWORD cbChain;
    DWORD cTotalCert = 0;
    PCHAIN_INFO pChainInfoHead = NULL;
    LONG cbExtra = 0;

    // get the certs out of the IE30 tree and put it in ours
    // open the IE30 store

    if (NULL != (hStore = CertOpenSystemStore(
    NULL,
    IE30CONVERTEDSTORE))) {

    // don't care about errors, and we don't
    // want to delete the old store just yet.
    GetAndIe30ClientAuthCertificates(hStore);
    CertCloseStore(hStore, 0);
    }


    // copy the IE30 certs


    if (NULL == (phMyStoreList = GetMyStoreList()))
        goto ErrorReturn;
    if (NULL == (phCaStoreList = GetCaStoreList()))
        goto ErrorReturn;

    // Iterate through all "My" cert stores to find certificates having a
    // CRYPT_KEY_PROV_INFO property
    phStore = phMyStoreList;
    while (hStore = *phStore++) {
        PCCERT_CONTEXT pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
            DWORD cbKeyProvInfo;
            if (CheckKeyProvInfo(pCert, dwKeySpec, &cbKeyProvInfo)) {
                // Create a cert chain and check for an issuer name match
                // of any cert in the chain.
                PCHAIN_INFO pChainInfo;
                if (pChainInfo = CreateChainInfo(
                        pCert,
                        pbEncodedIssuerName,
                        cbEncodedIssuerName,
                        phCaStoreList,
                        phMyStoreList
                        )) {
                    // Add to list of chains
                    pChainInfo->pNext = pChainInfoHead;
                    pChainInfoHead = pChainInfo;

                    // Update bytes needed for KeyProvInfo
                    pChainInfo->cbKeyProvInfo = ALIGN_LEN(cbKeyProvInfo);

                    // Update totals
                    cbExtra += pChainInfo->cbKeyProvInfo + pChainInfo->cbCert;
                    cChain++;
                    cTotalCert += pChainInfo->cCert;
                }
            }
        }
    }

    cbChain = sizeof(CERT_CHAIN) * cChain +
        sizeof(CERT_BLOB) * cTotalCert + cbExtra;

    {
        // Check and update output lengths and counts
        DWORD cbIn;

        if (cChain == 0) {
            hr = CRYPT_E_NOT_FOUND;
            goto HrError;
        }
        if (pCertChains == NULL)
            *pcbCertChains = 0;
        cbIn = *pcbCertChains;
        *pcCertChains = cChain;
        *pcbCertChains = cbChain;

        if (cbIn == 0) {
            hr = S_OK;
            goto CommonReturn;
        } else if (cbIn < cbChain) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_LENGTH);
            goto CommonReturn;
        }
    }

    {
        // Copy cert chains to output

        PCERT_CHAIN pOutChain;
        PCERT_BLOB pCertBlob;
        BYTE *pbExtra;
        PCHAIN_INFO pChainInfo;

        pOutChain = pCertChains;
        pCertBlob = (PCERT_BLOB) (((BYTE *) pOutChain) +
            sizeof(CERT_CHAIN) * cChain);
        pbExtra = ((BYTE *) pCertBlob) + sizeof(CERT_BLOB) * cTotalCert;
        pChainInfo = pChainInfoHead;
        for ( ;  pChainInfo != NULL;
                                pChainInfo = pChainInfo->pNext, pOutChain++) {
            DWORD cb;
            DWORD cCert = pChainInfo->cCert;
            PCCERT_CONTEXT *ppCert = pChainInfo->rgpCert;
    
            pOutChain->cCerts = cCert;
            pOutChain->certs = pCertBlob;
            cb = pChainInfo->cbKeyProvInfo;
            cbExtra -= cb;
            assert(cbExtra >= 0);
            if (cbExtra < 0) goto UnexpectedError;
            if (!CertGetCertificateContextProperty(
                    *ppCert,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    pbExtra,
                    &cb
                    ))
                goto UnexpectedError;
            pOutChain->keyLocatorInfo = * ((PCRYPT_KEY_PROV_INFO) pbExtra);
            pbExtra += pChainInfo->cbKeyProvInfo;
    
            for ( ; cCert > 0; cCert--, ppCert++, pCertBlob++) {
                cb = (*ppCert)->cbCertEncoded;
                cbExtra -= ALIGN_LEN(cb);
                assert(cbExtra >= 0);
                if (cbExtra < 0) goto UnexpectedError;

                pCertBlob->cbData = cb;
                pCertBlob->pbData = pbExtra;
                memcpy(pbExtra, (*ppCert)->pbCertEncoded, cb);
                pbExtra += ALIGN_LEN(cb);
            }
        }
        assert(cbExtra == 0);
        assert(pCertBlob == (PCERT_BLOB) ((BYTE *) pCertChains +
            sizeof(CERT_CHAIN) * cChain +
            sizeof(CERT_BLOB) * cTotalCert));
    }

    hr = S_OK;
    goto CommonReturn;

UnexpectedError:
    hr = E_UNEXPECTED;
    goto HrError;
ErrorReturn:
    hr = SignError();
HrError:
    *pcbCertChains = 0;
    *pcCertChains = 0;
CommonReturn:
    while (pChainInfoHead) {
        PCHAIN_INFO pChainInfo = pChainInfoHead;
        DWORD cCert = pChainInfo->cCert;
        while (cCert--)
            CertFreeCertificateContext(pChainInfo->rgpCert[cCert]);
        pChainInfoHead = pChainInfo->pNext;
        ACFree(pChainInfo);
    }

    if (phMyStoreList) {
        phStore = phMyStoreList;
        while (hStore = *phStore++)
            CertCloseStore(hStore, 0);
        ACFree(phMyStoreList);
    }
    if (phCaStoreList) {
        phStore = phCaStoreList;
        while (hStore = *phStore++)
            CertCloseStore(hStore, 0);
        ACFree(phCaStoreList);
    }

    return hr;
}
