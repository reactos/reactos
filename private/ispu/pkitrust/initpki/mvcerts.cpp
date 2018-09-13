//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       mvcerts.cpp
//
//--------------------------------------------------------------------------


#include    "global.hxx"

static HRESULT HError ()
{
    DWORD   dw = GetLastError ();
    HRESULT hr;
    if ( dw <= (DWORD) 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;

    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}


//
// The root of the certificate store that we manage.
//
#define SZIE30CERTROOT       "Software\\Microsoft\\Cryptography\\CertificateStore"
#define SZIE30CERTPARENT     "Software\\Microsoft\\Cryptography"
#define SZIE30CERTSTORE      "CertificateStore"
#define SZIE30CERTBUCKET     "Certificates"
#define SZIE30INDEXISSUER    "IndexByIssuerName"
#define SZIE30INDEXISSUERSER "IndexByIssuerNameAndSerialNumber"
#define SZIE30INDEXSUBJECT   "IndexBySubjectName"
#define SZIE30INDEXKEY       "IndexBySubjectPublicKey"
#define SZIE30CERTCLIENTAUTH "Software\\Microsoft\\Cryptography\\PersonalCertificates\\ClientAuth"
#define SZIE30TAGS           "CertificateTags"
#define SZIE30AUXINFO        "CertificateAuxiliaryInfo"
#define IE30CONVERTEDSTORE   "My"


HRESULT PurgeDuplicateCertificate(HCERTSTORE hStore, PCCERT_CONTEXT pCert)
{
    HRESULT hr = S_OK;
    PCCERT_CONTEXT pExistingCert = NULL;
    BOOL fRes = FALSE;

    // Check for existing certificates.
    pExistingCert = CertGetSubjectCertificateFromStore(hStore,
                                                       X509_ASN_ENCODING,
                                                       pCert->pCertInfo);
    if (pExistingCert) 
    {
        if (CompareFileTime(&pExistingCert->pCertInfo->NotBefore,
                            &pCert->pCertInfo->NotBefore) <= 0) {

            fRes = CertDeleteCertificateFromStore(pExistingCert);  // Delete existing
            pExistingCert = NULL;

            if (!(fRes))
            {
                goto CertDupError;
            }
        }
        else 
        {
            hr = S_FALSE;
        }
    }

    CommonReturn:
        if (pExistingCert)
        {
            CertFreeCertificateContext(pExistingCert);
        }
        return hr;


    ErrorReturn:
        SetLastError((DWORD)hr);
        goto CommonReturn;

    SET_HRESULT_EX(DBG_SS, CertDupError, GetLastError());
}

HRESULT MoveSpcCerts(BOOL fDelete, HCERTSTORE hStore)
// Check for and copy any existing certificates stored in Bob's
// certificate store. Also, delete Bob's certificate keys from the registry.
{
    HRESULT hr = S_OK;
    LONG Status;
    HKEY hKeyRoot = NULL;
    HKEY hKeyParent = NULL;
    HKEY hKeyBucket = NULL;
    LPSTR pszName = NULL;
    BYTE *pbData = NULL;

    PKITRY {
        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          SZIE30CERTROOT,
                                          0,                  // dwReserved
                                          KEY_READ,
                                          &hKeyRoot))
            PKITHROW(S_OK);

        // Copy any existing certificates
        if (ERROR_SUCCESS != RegOpenKeyEx(hKeyRoot,
                                          SZIE30CERTBUCKET,
                                          0,                  // dwReserved
                                          KEY_READ,
                                          &hKeyBucket))
            PKITHROW(HError());

        DWORD   cValues, cchMaxName, cbMaxData;
        // see how many and how big the registry is
        if (ERROR_SUCCESS != RegQueryInfoKey(hKeyBucket,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &cValues,
                                             &cchMaxName,
                                             &cbMaxData,
                                             NULL,
                                             NULL
                                             ))
            PKITHROW(HError());

        // allocate the memory needed to read the reg
        pszName = (LPSTR) LocalAlloc(LMEM_ZEROINIT, cchMaxName + 1);
        pbData = (BYTE *) LocalAlloc(LMEM_ZEROINIT, cbMaxData);
        if (NULL == pszName  || NULL == pbData)
            PKITHROW(E_OUTOFMEMORY);

        // enum the registry getting certs
        for (DWORD i = 0; i < cValues; i++ ) {
            DWORD dwType;
            DWORD cchName = cchMaxName + 1;
            DWORD cbData = cbMaxData;
            PCCERT_CONTEXT pCert = NULL;

            if (ERROR_SUCCESS != RegEnumValueA(hKeyBucket,
                                               i,
                                               pszName,
                                               &cchName,
                                               NULL,
                                               &dwType,
                                               pbData,
                                               &cbData)) {
                if (SUCCEEDED(hr))
                    hr = HError();
            } else if (cchName) {
                if((pCert = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                         pbData,
                                                         cbData)) == NULL) {
                    hr = HError();
                }
                else {
                    HRESULT hr2 = PurgeDuplicateCertificate(hStore, pCert);
                    if(hr2 == S_OK) {
                        if(!CertAddCertificateContextToStore(hStore,
                                                             pCert,
                                                             CERT_STORE_ADD_USE_EXISTING,
                                                             NULL       // ppStoreContext
                                                             )) {
                            /*MessageBox(NULL, "Copy Certificate Failed", NULL,
                                       MB_OK);*/
                            if(SUCCEEDED(hr))
                                hr = HError();
                        }
                    }
                }
                if(pCert)
                    CertFreeCertificateContext(pCert);
                /*
                  if (!CertAddEncodedCertificateToStore(hStore,
                                                      X509_ASN_ENCODING,
                                                      pbData,
                                                      cbData,
                                                      CERT_STORE_ADD_USE_EXISTING,
                                                      NULL)) {               // ppCertContext
                    MessageBox(NULL, "Copy Certificate Failed", NULL,
                               MB_OK);
                               if (SUCCEEDED(hr))
                        hr = HError();
                        */
            }
        }
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if (pszName)
        LocalFree(pszName);
    if (pbData)
        LocalFree(pbData);
    RegCloseKey(hKeyBucket);
    RegCloseKey(hKeyRoot);


    if(SUCCEEDED(hr) && fDelete) {

        Status = ERROR_SUCCESS;
        while (Status == ERROR_SUCCESS) {
            // Re-open registry with write/delete access
            if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                              SZIE30CERTROOT,
                                              0,          // dwReserved
                                              KEY_ALL_ACCESS,
                                              &hKeyRoot))
                return HError();

            // Delete all of the store's subkeys including the certificates
            CHAR szSubKey[MAX_PATH+1];
            if (ERROR_SUCCESS == (Status = RegEnumKey(hKeyRoot,
                                                      0,          // iSubKey
                                                      szSubKey,
                                                      MAX_PATH + 1
                                                      )))
                Status = RegDeleteKey(hKeyRoot, szSubKey);
            RegCloseKey(hKeyRoot);
        }

        // Open the store's parent registry so we can delete the store
        if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          SZIE30CERTPARENT,
                                          0,          // dwReserved
                                          KEY_ALL_ACCESS,
                                          &hKeyParent))
            return HError();

        if (ERROR_SUCCESS != RegDeleteKey(hKeyParent, SZIE30CERTSTORE))
            hr = HError();

        RegCloseKey(hKeyParent);
    }

    return hr;
}

BOOL TestIE30Store(HKEY hRegRoot, LPCSTR psLoc)
{
    HRESULT hr = S_FALSE;
    HKEY hKeyRoot   = NULL;
    HKEY hKeyBucket = NULL;
    char pbValueName[MAX_PATH];
    DWORD cbValueName = MAX_PATH;
    DWORD cSubKeys;
    DWORD dwType;

    // __asm int 3

    PKITRY {
        if (ERROR_SUCCESS != RegOpenKeyExA(hRegRoot,
                                           psLoc,
                                           0,                  // dwReserved
                                           KEY_READ,
                                           &hKeyRoot))
            PKITHROW(S_FALSE);

        if (ERROR_SUCCESS != RegOpenKeyExA(hKeyRoot,
                                           SZIE30CERTBUCKET,
                                           0,                  // dwReserved
                                           KEY_READ,
                                           &hKeyBucket))
            PKITHROW(S_FALSE);

        DWORD   cValues, cchMaxName, cbMaxData;
        // see how many and how big the registry is
        if (ERROR_SUCCESS != RegQueryInfoKey(hKeyBucket,
                                             NULL,  // lpszClasss
                                             NULL,  // lpcchClass
                                             NULL,  // lpdwReserved
                                             &cSubKeys,
                                             NULL,  // lpcchMaxSubkey
                                             NULL,  // lpcchMaxClass
                                             &cValues,
                                             &cchMaxName,
                                             &cbMaxData,
                                             NULL,
                                             NULL
                                             ))
            PKITHROW(HError());

        if(cchMaxName < 40 && cSubKeys == 0)
            hr = S_OK;
    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND

    if(hKeyRoot != NULL)
        RegCloseKey(hKeyRoot);
    if(hKeyBucket != NULL)
        RegCloseKey(hKeyBucket);
    return hr == S_OK ? TRUE : FALSE;
}


HRESULT TransferIE30Certificates(HKEY hRegRoot, LPCSTR psLoc, HCERTSTORE hStore, BOOL fDelete)
// Check for and copy any existing certificates stored in Bob's
// certificate store.
{
    HRESULT hr = S_OK;
    LONG Status;
    HKEY hKeyRoot   = NULL;
    HKEY hKeyBucket = NULL;
    HKEY hKeyTags   = NULL;
    HKEY hKeyAux    = NULL;

    if (ERROR_SUCCESS != RegOpenKeyExA(hRegRoot,
                                       psLoc,
                                       0,                  // dwReserved
                                       KEY_READ,
                                       &hKeyRoot
                                       ))
        return S_OK;

        // Copy any existing certificates
    if (ERROR_SUCCESS == RegOpenKeyExA(hKeyRoot,
                                       SZIE30CERTBUCKET,
                                       0,                  // dwReserved
                                       KEY_READ,
                                       &hKeyBucket
                                       )               &&

        ERROR_SUCCESS == RegOpenKeyExA(hKeyRoot,
                                       SZIE30AUXINFO,
                                       0,                  // dwReserved
                                       KEY_READ,
                                       &hKeyAux
                                       )               &&

        ERROR_SUCCESS == RegOpenKeyExA(hKeyRoot,
                                       SZIE30TAGS,
                                       0,                  // dwReserved
                                       KEY_READ,
                                       &hKeyTags
                                       )) {

        DWORD   cValuesCert, cchMaxNameCert, cbMaxDataCert;
        DWORD   cValuesTag, cchMaxNameTag, cbMaxDataTag;
        DWORD   cValuesAux, cchMaxNameAux, cbMaxDataAux;
        LPSTR   szName = NULL;
        BYTE *pbLoadCert = NULL;
        BYTE *pbFixedCert = NULL;
        BYTE *pbDataCert = NULL;
        BYTE *pbDataAux = NULL;
        BYTE *pbDataTag = NULL;

        // see how many and how big the registry is
        if (ERROR_SUCCESS != RegQueryInfoKey(hKeyBucket,
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
            ERROR_SUCCESS != RegQueryInfoKey(hKeyTags,
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
            ERROR_SUCCESS != RegQueryInfoKey(hKeyAux,
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
            hr = HError();
        else {
            // allocate the memory needed to read the reg
            szName = (LPSTR) LocalAlloc(LMEM_ZEROINIT, cchMaxNameCert + 1);
            pbDataCert = (BYTE *) LocalAlloc(LMEM_ZEROINIT, cbMaxDataCert);
            pbDataTag = (BYTE *) LocalAlloc(LMEM_ZEROINIT, cbMaxDataTag);
            pbDataAux = (BYTE *) LocalAlloc(LMEM_ZEROINIT, cbMaxDataAux);

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
            DWORD cbLoadCert = 0;
            DWORD cbFixedCert = 0;
            DWORD cbDataTag = cbMaxDataTag;
            DWORD cbDataAux = cbMaxDataAux;
            PCCERT_CONTEXT pCertContxt = NULL;
            HRESULT status = S_OK;

            // don't have to worry about errors, just skip
            // sliently just be cause there is an internal
            // error in the registry doesn't mean we should
            // get all upset about it.

            // get the cert
            if (RegEnumValueA(hKeyBucket,
                              i,
                              szName,
                              &cchName,
                              NULL,
                              &dwType,
                              pbDataCert,
                              &cbDataCert
                              ) == ERROR_SUCCESS      &&

                dwType == REG_BINARY) {


                if(Fix7FCert(cbDataCert,
                             pbDataCert,
                             &cbFixedCert,
                             &pbFixedCert) &&
                   cbFixedCert != 0) {
                    pbLoadCert = pbFixedCert;
                    cbLoadCert = cbFixedCert;
                }
                else {
                    pbLoadCert = pbDataCert;
                    cbLoadCert = cbDataCert;
                }

                // get the cert context
                if((pCertContxt = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                               pbLoadCert,
                                                               cbLoadCert)) != NULL) {

                    // See if it has a tag and aux info.
                    // get the tag
                    if(RegQueryValueExA(hKeyTags,
                                        szName,
                                        NULL,
                                        &dwType,
                                        pbDataTag,
                                        &cbDataTag) == ERROR_SUCCESS    &&

                       // get the aux info
                       RegQueryValueExA(hKeyAux,
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
                        if( !CertSetCertificateContextProperty(pCertContxt,
                                                               CERT_KEY_PROV_INFO_PROP_ID,
                                                               0,
                                                               &keyInfo))
                            status = S_FALSE;
                    }

                    if(status == S_OK) {
                        HRESULT hr2 = PurgeDuplicateCertificate(hStore, pCertContxt);
                        if(hr2 == S_OK) {
                            if(!CertAddCertificateContextToStore(hStore,
                                                                 pCertContxt,
                                                                 CERT_STORE_ADD_USE_EXISTING,
                                                                 NULL       // ppStoreContext
                                                                 )) {
                                /*MessageBox(NULL,
                                  "Copy Certificate Failed",
                                  NULL,
                                  MB_OK);*/
                                hr = HError();
                            }
                        }
                    }
                }

            }
            if(pCertContxt != NULL)
                CertFreeCertificateContext(pCertContxt);
            if(pbFixedCert) {
                LocalFree(pbFixedCert);
                pbFixedCert = NULL;
            }
        }

        if (szName)
            LocalFree(szName);
        if (pbDataCert)
            LocalFree(pbDataCert);
        if(pbDataAux)
            LocalFree(pbDataAux);
        if(pbDataTag)
            LocalFree(pbDataTag);
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

    if(SUCCEEDED(hr) && fDelete) {

        // Re-open registry with write/delete access
        if (ERROR_SUCCESS != RegOpenKeyEx(hRegRoot,
                                          psLoc,
                                          0,          // dwRes erved
                                          KEY_ALL_ACCESS,
                                          &hKeyRoot))
            return HError();

        Status = RegDeleteKey(hKeyRoot, SZIE30CERTBUCKET);
        Status = RegDeleteKey(hKeyRoot, SZIE30INDEXISSUER);
        Status = RegDeleteKey(hKeyRoot, SZIE30INDEXISSUERSER);
        Status = RegDeleteKey(hKeyRoot, SZIE30INDEXSUBJECT);
        Status = RegDeleteKey(hKeyRoot, SZIE30INDEXKEY);
        Status = RegDeleteKey(hKeyRoot, SZIE30TAGS);
        RegCloseKey(hKeyRoot);
    }

    return hr;
}


HRESULT MoveCertificates(BOOL fDelete)
{
    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;

    HCERTSTORE hSpcStore = NULL;
    HCERTSTORE hStore = NULL;
    HCRYPTPROV hCrypt = NULL;
    //__asm int 3
    PKITRY {
        /*
        if (!CryptAcquireContext(&hCrypt, NULL, MS_DEF_PROV, PROV_RSA_FULL, 0))
            PKITHROW(HError());
            */
        hSpcStore = CertOpenSystemStore( NULL, TEXT("SPC") );

        if(!hSpcStore)
            PKITHROW(HError());
        hr = MoveSpcCerts(fDelete, hSpcStore);

        hStore = CertOpenSystemStore(NULL, TEXT(IE30CONVERTEDSTORE));
        if(!hStore)
            PKITHROW(HError());

        hr2 = TransferIE30Certificates(HKEY_CURRENT_USER, SZIE30CERTCLIENTAUTH, hStore, fDelete);

    }
    PKICATCH(err) {
        hr = err.pkiError;
    } PKIEND;

    if(SUCCEEDED(hr)) hr = hr2;
    if(hSpcStore)
        CertCloseStore( hSpcStore, 0 );
    if(hStore)
        CertCloseStore(hStore, 0);
    if(hCrypt)
        CryptReleaseContext(hCrypt, 0);
    return hr;
}



