/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CStore definition
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

class CStore
{
  private:
    CAtlStringW m_CryptName;
    DWORD m_dwFlags;
    CAtlStringW m_StoreName;
    HCERTSTORE m_hStore = NULL;
    CAtlList<CCert *> m_Certificates;
    bool m_Expanded = false;

    HCERTSTORE
    StoreHandle()
    {
        if (!m_hStore)
        {
            m_hStore = CertOpenStore(
                CERT_STORE_PROV_SYSTEM_W, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, NULL,
                CERT_STORE_SET_LOCALIZED_NAME_FLAG | m_dwFlags | CERT_STORE_MAXIMUM_ALLOWED_FLAG, m_CryptName);
        }
        return m_hStore;
    }

  public:
    CStore(_In_ LPCWSTR cryptName, DWORD dwFlags) : m_CryptName(cryptName), m_dwFlags(dwFlags)
    {
        if (auto localized = CryptFindLocalizedName(cryptName))
        {
            m_StoreName = localized;
        }
        else
        {
            m_StoreName = cryptName;
        }
    }
    ~CStore()
    {
        if (m_hStore)
        {
            CertCloseStore(m_hStore, 0);
        }
    }

    LPCWSTR
    GetStoreName() const
    {
        return m_StoreName;
    }

    void
    Expand()
    {
        if (m_Expanded)
            return;
        m_Expanded = true;
        // EnumCertificateRevocationLists()
        // EnumCertificateTrustLists()
        EnumCertificates();
    }

    void
    EnumCertificates()
    {
        auto handle = StoreHandle();
        if (!handle)
            return;
        PCCERT_CONTEXT pContext = NULL;
        while ((pContext = CertEnumCertificatesInStore(handle, pContext)))
        {
            CCert *newCert = new CCert(pContext);
            m_Certificates.AddTail(newCert);
        }
    }

    template <typename Fn>
    void
    ForEach(Fn callback)
    {
        Expand();

        for (POSITION it = m_Certificates.GetHeadPosition(); it; m_Certificates.GetNext(it))
        {
            auto current = m_Certificates.GetAt(it);
            callback(current);
        }
    }
};
