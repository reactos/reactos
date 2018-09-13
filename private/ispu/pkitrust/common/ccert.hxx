//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ccert.hxx
//
//  Contents:   Microsoft Internet Security Certificate Class
//
//  History:    15-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CCERT_HXX
#define CCERT_HXX

#define     CCERT_MAXSTORES     5

class CCert_
{
    public:
            CCert_(PCCERT_CONTEXT pCertContext);
            virtual ~CCert_(void);

            WCHAR           *PublisherName(void);
            WCHAR           *AgencyName(void);

            WCHAR           *ProviderName(void);
            DWORD           ProviderType(void);
            WCHAR           *PrivateKeyContainer(void);

            PCCERT_CONTEXT  CertContext(void) { return(m_pCertContext); }

            BOOL            BuildChain(FILETIME *psftVerifyAsOf = NULL);
            CCert_          *IssuerCCert_(void) { return(m_pCCert_Issuer); } // build the chain first!
            DWORD           IssuersConfidence(void) { return(m_dwConfidence); }

    protected:
            BOOL            ExtractCommonNameExt(WCHAR **pwszRet);
            BOOL            ExtractCommonNameAttr(WCHAR **pwszRet);

    private:
            HCERTSTORE      m_pahStores[CCERT_MAXSTORES];
            DWORD           m_chStores;
            PCCERT_CONTEXT  m_pCertContext;
            CCert_          *m_pCCert_Issuer;
            WCHAR           *m_pwszPublisherName;
            WCHAR           *m_pwszAgencyName;
            DWORD           m_dwConfidence;

            BOOL            m_fTriedPrivateKey;
            WCHAR           *m_pwszProvider;        // if signing cert
            DWORD           m_dwProviderType;
            WCHAR           *m_pwszKeyContainer;    // if signing cert


            WCHAR           *GetRDNAttr(char *pszObjId, PCERT_NAME_BLOB pNameBlob);
            void            OpenStores(void);
            BOOL            BuildChainPrivate(DWORD chStores, HCERTSTORE *pahStores, FILETIME *psftVerifyAsOf);
            BOOL            FindPrivateKey(void);
            BOOL            EnumContainer(HCRYPTPROV *phProv, DWORD dwIndex, DWORD dwProvType, 
                                          WCHAR *pwszProv, WCHAR **ppwszContainer);
            BOOL            CheckContainerForKey(WCHAR *pwszProv, DWORD dwProvType, WCHAR *pwszContainer);
};

#endif // CCERT_HXX


