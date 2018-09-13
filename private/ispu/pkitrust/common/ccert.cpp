//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       CCert.cpp
//
//  Contents:   Microsoft Internet Security Certificate Class
//
//  History:    14-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "signer.h"

extern "C"
{
extern BOOL WINAPI GetCryptProvFromCert(HWND hwnd, PCCERT_CONTEXT pCert, HCRYPTPROV	*phCryptProv,
                                        DWORD *pdwKeySpec, BOOL	*pfDidCryptAcquire,	
                                        LPWSTR *ppwszTmpContainer, LPWSTR *ppwszProviderName,
                                        DWORD *pdwProviderType);

extern void WINAPI FreeCryptProvFromCert(BOOL fAcquired, HCRYPTPROV hProv, LPWSTR pwszCapiProvider,
                                        DWORD dwProviderType, LPWSTR pwszTmpContainer);
};


#include    "ccert.hxx"

CCert_::CCert_(PCCERT_CONTEXT pCertContext)
{
    m_pCertContext          = CertDuplicateCertificateContext(pCertContext);
    
    m_pCCert_Issuer         = NULL;
    m_pwszPublisherName     = NULL;
    m_pwszAgencyName        = NULL;
    m_pwszProvider          = NULL;
    m_pwszKeyContainer      = NULL;
    m_dwProviderType        = 0;
    m_chStores              = 0;
    m_fTriedPrivateKey      = FALSE;
    memset(m_pahStores, 0x00, sizeof(HCERTSTORE) * CCERT_MAXSTORES);
}

CCert_::~CCert_(void)
{
    CertFreeCertificateContext(m_pCertContext);

    DELETE_OBJECT(m_pCCert_Issuer);
    DELETE_OBJECT(m_pwszPublisherName);
    DELETE_OBJECT(m_pwszAgencyName);
    DELETE_OBJECT(m_pwszProvider);
    DELETE_OBJECT(m_pwszKeyContainer);

    for (int i = 0; i < (int)m_chStores; i++)
    {
        if (m_pahStores[i])
        {
            CertCloseStore(m_pahStores[i], 0);
        }
    }
}

WCHAR *CCert_::PublisherName(void)
{
    if (m_pwszPublisherName)
    {
        return(m_pwszPublisherName);
    }

    if (!(this->ExtractCommonNameExt(&m_pwszPublisherName)))
    {
        if (!(this->ExtractCommonNameAttr(&m_pwszPublisherName)))
        {
            m_pwszPublisherName = this->GetRDNAttr(szOID_COMMON_NAME,
                                                   &m_pCertContext->pCertInfo->Subject);
        }
    }

    return(m_pwszPublisherName);
}

WCHAR *CCert_::AgencyName(void)
{
    if (m_pwszAgencyName)
    {
        return(m_pwszAgencyName);
    }

    if (!(m_pCertContext))
    {
        return(NULL);
    }

    m_pwszAgencyName = this->GetRDNAttr(szOID_ORGANIZATIONAL_UNIT_NAME,
                                        &m_pCertContext->pCertInfo->Subject);
    if (!(m_pwszAgencyName))
    {
        m_pwszAgencyName = this->GetRDNAttr(szOID_ORGANIZATION_NAME,
                                            &m_pCertContext->pCertInfo->Subject);

        if (!(m_pwszAgencyName))
        {
            m_pwszAgencyName = this->GetRDNAttr(szOID_COMMON_NAME,
                                                &m_pCertContext->pCertInfo->Subject);
        }
    }

    return(m_pwszAgencyName);
}

WCHAR *CCert_::ProviderName(void)
{
    if (m_pwszProvider)
    {
        return(m_pwszProvider);
    }

    if (!(m_pCertContext))
    {
        return(NULL);
    }

    this->FindPrivateKey();

    return(m_pwszProvider);
}

DWORD CCert_::ProviderType(void)
{
    if (m_pwszProvider)
    {
        return(m_dwProviderType);
    }

    this->FindPrivateKey();

    return(m_dwProviderType);
}

WCHAR *CCert_::PrivateKeyContainer(void)
{
    if (m_pwszKeyContainer)
    {
        return(m_pwszKeyContainer);
    }

    this->FindPrivateKey();

    return(m_pwszKeyContainer);
}

BOOL CCert_::BuildChain(FILETIME *psftVerifyAsOf)
{
    FILETIME        sft;

    if (!(m_pCertContext))
    {
        return(FALSE);
    }

    if (m_pCCert_Issuer)
    {
        return(TRUE);
    }

    if (!(psftVerifyAsOf))
    {
        GetSystemTimeAsFileTime(&sft);

        psftVerifyAsOf = &sft;
    }

    this->OpenStores();

    return(this->BuildChainPrivate(m_chStores, m_pahStores, psftVerifyAsOf));
}

//////////////////////////////////////////////////////////////////////////
////
////    protected
////
BOOL CCert_::ExtractCommonNameExt(WCHAR **ppwszRet)
{
    *ppwszRet = NULL;

    if (!(m_pCertContext))
    {
        return(FALSE);
    }

    PCERT_NAME_VALUE    pNameValue;
    PCERT_EXTENSION     pExt;

    pNameValue  = NULL;

    pExt = CertFindExtension(SPC_COMMON_NAME_OBJID,
                             m_pCertContext->pCertInfo->cExtension,
                             m_pCertContext->pCertInfo->rgExtension);
    if (pExt) 
    {
        DWORD                   cbInfo;
        PCERT_RDN_VALUE_BLOB    pValue;
        DWORD                   dwValueType;
        DWORD                   cwsz;

        cbInfo  = 0;

        CryptDecodeObject(  X509_ASN_ENCODING,
                            X509_NAME_VALUE,
                            pExt->Value.pbData,
                            pExt->Value.cbData,
                            0,
                            NULL,
                            &cbInfo);

        if (cbInfo == 0)
        {
            return(FALSE);
        }

        if (!(pNameValue = (PCERT_NAME_VALUE)new BYTE[cbInfo]))
        {
            return(FALSE);
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING,
                                X509_NAME_VALUE,
                                pExt->Value.pbData,
                                pExt->Value.cbData,
                                0,
                                pNameValue,
                                &cbInfo)))
        {
            delete pNameValue;
            return(FALSE);
        }

        dwValueType = pNameValue->dwValueType;
        pValue      = &pNameValue->Value;

        cwsz = CertRDNValueToStrW(dwValueType, pValue, NULL, 0);

        if (cwsz > 1) 
        {
            if (!(*ppwszRet = new WCHAR[cwsz]))
            {
                delete pNameValue;

                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return(FALSE);
            }

            CertRDNValueToStrW(dwValueType, pValue, *ppwszRet, cwsz);

            delete pNameValue;
            return(TRUE);
        }
    }

    DELETE_OBJECT(pNameValue);

    return(FALSE);
}

BOOL CCert_::ExtractCommonNameAttr(WCHAR **ppwszRet)
{
    *ppwszRet = GetRDNAttr(szOID_COMMON_NAME, &m_pCertContext->pCertInfo->Subject);

    if (*ppwszRet)
    {
        return(TRUE);
    }

    return(FALSE);
}

WCHAR *CCert_::GetRDNAttr(char *pszObjId, PCERT_NAME_BLOB pNameBlob)
{
    LPWSTR          pwsz;
    PCERT_NAME_INFO pNameInfo;
    PCERT_RDN_ATTR  pRDNAttr;
    DWORD           cbInfo;

    pwsz        = NULL;
    pNameInfo   = NULL;

    cbInfo      = 0;

    CryptDecodeObject(  X509_ASN_ENCODING,
                        X509_NAME,
                        pNameBlob->pbData,
                        pNameBlob->cbData,
                        0,
                        NULL,
                        &cbInfo);
    
    if (cbInfo == 0) 
    {
        return(NULL);
    }

    if (!(pNameInfo = (PCERT_NAME_INFO)new BYTE[cbInfo]))
    {
        return(NULL);
    }

    if (!(CryptDecodeObject(X509_ASN_ENCODING,
                            X509_NAME,
                            pNameBlob->pbData,
                            pNameBlob->cbData,
                            0,
                            pNameInfo,
                            &cbInfo)))
    {
        delete pNameInfo;
        return(NULL);
    }

    pRDNAttr = CertFindRDNAttr(pszObjId, pNameInfo);

    if (pRDNAttr) 
    {
        PCERT_RDN_VALUE_BLOB    pValue = &pRDNAttr->Value;
        DWORD                   dwValueType = pRDNAttr->dwValueType;
        DWORD                   cwsz;

        pValue      = &pRDNAttr->Value;
        dwValueType = pRDNAttr->dwValueType;

        cwsz = CertRDNValueToStrW(dwValueType,
                                  pValue,
                                  NULL,
                                  0);

        if (cwsz > 1) 
        {
            if (!(pwsz = new WCHAR[cwsz]))
            {
                delete pNameInfo;

                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return(NULL);
            }

            CertRDNValueToStrW(dwValueType, pValue, pwsz, cwsz);
        }
    }

    DELETE_OBJECT(pNameInfo);

    return(pwsz);
}


//////////////////////////////////////////////////////////////////////////
////
////    private
////
BOOL CCert_::BuildChainPrivate(DWORD chStores, HCERTSTORE *pahStores, FILETIME *psftVerifyAsOf)
{
    DWORD           dwError;
    PCCERT_CONTEXT  pIssuerCertContext;

    DELETE_OBJECT(m_pCCert_Issuer);

    if (TrustIsCertificateSelfSigned(m_pCertContext,
                                     m_pCertContext->dwCertEncodingType, 
                                     0))
    {
        return(TRUE);
    }

    pIssuerCertContext = TrustFindIssuerCertificate(m_pCertContext, 
                                                    m_pCertContext->dwCertEncodingType,
                                                    chStores,
                                                    pahStores, 
                                                    psftVerifyAsOf,
                                                    &m_dwConfidence,
                                                    &dwError,
                                                    0);

    if (!(pIssuerCertContext))
    {
        SetLastError(dwError);
        return(FALSE);
    }

    m_pCCert_Issuer = new CCert_(pIssuerCertContext);

    CertFreeCertificateContext(pIssuerCertContext);

    if (!(m_pCCert_Issuer))
    {
        return(FALSE);
    }

    return(m_pCCert_Issuer->BuildChainPrivate(chStores, pahStores, psftVerifyAsOf));
}

    //
    // warning: if you add a store, make sure to add one to the CCERT_MAXSTORES in ccert.hxx!!!
    //
void CCert_::OpenStores(void)
{
    HCERTSTORE  hStore;

    m_chStores = 0;

    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_CURRENT_USER |
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "ROOT"))
    {
        m_pahStores[m_chStores] = hStore;
        m_chStores++;
    }
    else
    {
        return;  // if we can't find the root, FAIL!
    }

    //
    //  open the Trust List store
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_CURRENT_USER | 
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "TRUST"))
    {
        m_pahStores[m_chStores] = hStore;
        m_chStores++;
    }

    //
    //  CA Store
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_CURRENT_USER | 
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "CA"))
    {
        m_pahStores[m_chStores] = hStore;
        m_chStores++;
    }
    
    //
    //  MY Store
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_CURRENT_USER | 
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "MY"))
    {
        m_pahStores[m_chStores] = hStore;
        m_chStores++;
    }
    
    //
    //  SPC Store (historic reasons!)
    //
    if (hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                NULL,
                                CERT_SYSTEM_STORE_LOCAL_MACHINE | 
                                CERT_STORE_READONLY_FLAG |
                                CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                "SPC"))
    {
        m_pahStores[m_chStores] = hStore;
        m_chStores++;
    }
}

BOOL CCert_::FindPrivateKey(void)
{
    if ((m_pwszProvider) || (m_pwszKeyContainer))
    {
        return(TRUE);
    }

    if (m_fTriedPrivateKey)
    {
        return(FALSE);
    }

    m_fTriedPrivateKey = TRUE;

    DELETE_OBJECT(m_pwszProvider);
    DELETE_OBJECT(m_pwszKeyContainer);

    //
    //  try mssign32.dll first
    //
    HCRYPTPROV      signer_hProv;
    WCHAR           *signer_pwszTmpContainer;
    WCHAR           *signer_pwszProviderName;
    DWORD           signer_dwKeySpec;
    DWORD           signer_dwProviderType;
    BOOL            signer_fDidCryptAcquire;

    signer_hProv            = NULL;
    signer_pwszTmpContainer = NULL;
    signer_pwszProviderName = NULL;

    if (GetCryptProvFromCert(NULL, 
                             m_pCertContext, 
                             &signer_hProv, 
                             &signer_dwKeySpec,
                             &signer_fDidCryptAcquire,
                             &signer_pwszTmpContainer,
                             &signer_pwszProviderName,
                             &signer_dwProviderType))
    {
        if (signer_pwszProviderName)
        {
            if (!(m_pwszProvider = new WCHAR[wcslen(signer_pwszProviderName) + 1]))
            {
                return(FALSE);
            }

            wcscpy(m_pwszProvider, signer_pwszProviderName);

            if (signer_pwszTmpContainer)
            {
                if (!(m_pwszKeyContainer  = new WCHAR[wcslen(signer_pwszTmpContainer) + 1]))
                {
                    return(FALSE);
                }

                wcscpy(m_pwszKeyContainer, signer_pwszTmpContainer);
            }

            m_dwProviderType    = signer_dwProviderType;


            FreeCryptProvFromCert(signer_fDidCryptAcquire,
                                  signer_hProv,
                                  signer_pwszProviderName,
                                  signer_dwProviderType,
                                  signer_pwszTmpContainer);
    
            return(TRUE);
        }
     
        FreeCryptProvFromCert(signer_fDidCryptAcquire,
                              signer_hProv,
                              signer_pwszProviderName,
                              signer_dwProviderType,
                              signer_pwszTmpContainer);
    }

    DWORD       dwIndexProv;
    DWORD       dwIndexContainer;
    DWORD       dwProvType;
    DWORD       cbSize;
    WCHAR       wszProv[MAX_PATH + 1];
    WCHAR       *pwszContainer;

    HCRYPTPROV  hProvEnum;

    pwszContainer   = NULL;
    dwIndexProv     = 0;

    for EVER
    {
        cbSize = MAX_PATH;

        if (!(CryptEnumProvidersU(dwIndexProv, NULL, 0, &dwProvType, &wszProv[0], &cbSize)))
        {
            break;
        }

        wszProv[cbSize] = NULL;

        dwIndexContainer    = 0;
        hProvEnum           = NULL;

        for EVER
        {
            if (!(this->EnumContainer(&hProvEnum, dwIndexContainer, dwProvType, &wszProv[0], &pwszContainer)))
            {
                break;
            }
            
            if (this->CheckContainerForKey(&wszProv[0], dwProvType, pwszContainer))
            {
                m_dwProviderType    = dwProvType;
                m_pwszKeyContainer  = pwszContainer;

                if (m_pwszProvider = new WCHAR[wcslen(&wszProv[0]) + 1])
                {
                    wcscpy(m_pwszProvider, &wszProv[0]);
                }

                CryptReleaseContext(hProvEnum, 0);
                return(TRUE);
            }

            DELETE_OBJECT(pwszContainer);

            dwIndexContainer++;
        }

        if (hProvEnum)
        {
            CryptReleaseContext(hProvEnum, 0);
        }

        dwIndexProv++;  // for our enum at the top!
    }

    return(FALSE);
}

BOOL CCert_::EnumContainer(HCRYPTPROV *phProv, DWORD dwIndex, DWORD dwProvType, WCHAR *pwszProv, WCHAR **ppwszContainer)
{
    DWORD       i;
    DWORD       cbSize;
    char        *psz;

    *ppwszContainer = NULL;

    if (!(*phProv))
    {
        if (!(CryptAcquireContextU(phProv, NULL, pwszProv, dwProvType, CRYPT_VERIFYCONTEXT)))
        {
            return(FALSE);
        }
    }
    cbSize = 0;
    CryptGetProvParam(*phProv, PP_ENUMCONTAINERS, NULL, &cbSize, CRYPT_FIRST);

    if (cbSize > 0)
    {
        if (!(psz = new char[cbSize]))
        {
            return(FALSE);
        }

        memset(psz, 0x00, cbSize);

        CryptGetProvParam(*phProv, PP_ENUMCONTAINERS, (BYTE *)psz, &cbSize, CRYPT_FIRST);

        for (i = 1; i <= dwIndex; i++)
        {
            if (!(CryptGetProvParam(*phProv, PP_ENUMCONTAINERS, (BYTE *)psz, &cbSize, 0)))
            {
                delete psz;
                return(FALSE);
            }
        }

        if (!(*ppwszContainer = new WCHAR[strlen(psz) + 1]))
        {
            delete psz;

            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
        
        MultiByteToWideChar(0, 0, psz, -1, *ppwszContainer, (strlen(psz) + 1) * sizeof(WCHAR));

        delete psz;
        return(TRUE);

    }

    return(FALSE);
}

BOOL CCert_::CheckContainerForKey(WCHAR *pwszProv, DWORD dwProvType, WCHAR *pwszContainer)
{
    HCRYPTPROV              hProv;
    DWORD                   cbSize;
    PCERT_PUBLIC_KEY_INFO   pContInfo;

    if (CryptAcquireContextU(&hProv, pwszContainer, pwszProv, dwProvType, 0))
    {
        if (!(CryptExportPublicKeyInfo(hProv, AT_SIGNATURE, X509_ASN_ENCODING, NULL, &cbSize)))
        {
            CryptReleaseContext(hProv, 0);
            return(FALSE);
        }

        if (!(pContInfo = (PCERT_PUBLIC_KEY_INFO)new BYTE[cbSize]))
        {
            CryptReleaseContext(hProv, 0);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        if (!(CryptExportPublicKeyInfo(hProv, AT_SIGNATURE, X509_ASN_ENCODING, pContInfo, &cbSize)))
        {
            delete pContInfo;

            CryptReleaseContext(hProv, 0);

            return(FALSE);
        }

        CryptReleaseContext(hProv, 0);

        if (CertComparePublicKeyInfo(X509_ASN_ENCODING, 
                                        &m_pCertContext->pCertInfo->SubjectPublicKeyInfo,
                                        pContInfo))
        {
            delete pContInfo;

            return(TRUE);
        }

        delete pContInfo;
    }

    return(FALSE);
}
