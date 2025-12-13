/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CCert definition
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

class CCert
{
  private:
    PCCERT_CONTEXT m_CertContext = NULL;
    CAtlStringW m_SubjectName;
    CAtlStringW m_IssuerName;
    CAtlStringW m_FriendlyName;

    void
    GetNameString(DWORD dwFlags, CAtlStringW &out)
    {
        DWORD length = CertGetNameStringW(m_CertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, dwFlags, NULL, NULL, 0);
        if (length > 1)
        {
            WCHAR *buffer = out.GetBuffer(length);
            CertGetNameStringW(m_CertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, dwFlags, NULL, buffer, length);
            out.ReleaseBuffer();
        }
    }

  public:
    explicit CCert(_In_ PCCERT_CONTEXT certContext)
    {
        m_CertContext = CertDuplicateCertificateContext(certContext);
        Init();
    }
    ~CCert()
    {
        if (m_CertContext)
        {
            CertFreeCertificateContext(m_CertContext);
        }
    }

    const CAtlStringW &
    GetSubjectName() const
    {
        return m_SubjectName;
    }

    const CAtlStringW &
    GetIssuerName() const
    {
        return m_IssuerName;
    }

    FILETIME
    GetNotAfter() const
    {
        return (m_CertContext && m_CertContext->pCertInfo) ? m_CertContext->pCertInfo->NotAfter : FILETIME{};
    }

    const CAtlStringW &
    GetFriendlyName() const
    {
        return m_FriendlyName;
    }

    const PCCERT_CONTEXT
    GetCertContext() const
    {
        return m_CertContext;
    }

    void
    Init()
    {
        GetNameString(0, m_SubjectName);
        GetNameString(CERT_NAME_ISSUER_FLAG, m_IssuerName);
        DWORD length = 0;
        if (CertGetCertificateContextProperty(m_CertContext, CERT_FRIENDLY_NAME_PROP_ID, NULL, &length) && length > 0)
        {
            WCHAR *buffer = m_FriendlyName.GetBuffer(length / sizeof(WCHAR));
            CertGetCertificateContextProperty(m_CertContext, CERT_FRIENDLY_NAME_PROP_ID, buffer, &length);
            m_FriendlyName.ReleaseBuffer();
        }
    }
};
