//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       iih.h
//
//  Contents:   ACUI Invoke Info Helper class definition
//
//  History:    10-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__IIH_H__)
#define __IIH_H__

#include <acui.h>
#include <acuictl.h>

extern HINSTANCE g_hModule;

//
// CInvokeInfoHelper is used to pull various pieces of information out
// of the ACUI_INVOKE_INFO data structure
//

class CInvokeInfoHelper
{
public:

    //
    // Initialization
    //

    CInvokeInfoHelper (
               PACUI_INVOKE_INFO pInvokeInfo,
               HRESULT&          rhr
               );

    ~CInvokeInfoHelper ();

    //
    // Information Retrieval Methods
    //

    LPCWSTR                  Subject()               { return(m_pszSubject); }
    LPCWSTR                  Publisher()             { return(m_pszPublisher); }
    LPCWSTR                  PublisherCertIssuer()   { return(m_pszPublisherCertIssuer); }
    LPCWSTR                  ControlWebPage()        { return(m_pszControlWebPage); }
    LPCWSTR                  CAWebPage()             { return(m_pszCAWebPage); }
    LPCWSTR                  AdvancedLink()          { return(m_pszAdvancedLink); }
    LPCWSTR                  CertTimestamp()         { return(m_pszCertTimestamp); }
    LPCWSTR                  TestCertInChain()       { return(m_pszTestCertInChain); }
    LPCWSTR                  ErrorStatement()        { return(m_pszErrorStatement); }

    PCRYPT_PROVIDER_DATA    ProviderData()          { return(m_pInvokeInfo->pProvData); }

    BOOL                    IsKnownPublisher()      { return(m_fKnownPublisher); }

    BOOL                    IsCertViewPropertiesAvailable() { return(m_pfnCVPA != NULL); }

    //
    // Personal Trust management
    //

    HRESULT AddPublisherToPersonalTrust ();

    //
    // UI control management
    //

    HRESULT GetUIControl (IACUIControl** ppUI);
    VOID ReleaseUIControl (IACUIControl* pUI);

    inline BOOL CallCertViewProperties (HWND hwndParent);
    inline VOID CallAdvancedLink (HWND hwndParent);
    inline VOID CallWebLink(HWND hwndParent, WCHAR *pszLink);

private:

    //
    // Invoke Info holder
    //

    PACUI_INVOKE_INFO      m_pInvokeInfo;

    //
    // Subject, Publisher, Issuer and Error Statement strings
    //

    LPWSTR                  m_pszSubject;
    LPWSTR                  m_pszPublisher;
    LPWSTR                  m_pszPublisherCertIssuer;
    LPWSTR                  m_pszErrorStatement;
    LPWSTR                  m_pszCertTimestamp;
    LPWSTR                  m_pszAdvancedLink;
    LPWSTR                  m_pszTestCertInChain;
    LPWSTR                  m_pszControlWebPage;
    LPWSTR                  m_pszCAWebPage;

    //
    // Known publisher flag
    //

    BOOL                   m_fKnownPublisher;

    //
    // Cert view properties entry point
    //

    HINSTANCE              m_hModCVPA;
    pfnCertViewProperties  m_pfnCVPA;

    //
    // Private methods
    //

    HRESULT InitSubject();
    HRESULT InitPublisher();
    HRESULT InitPublisherCertIssuer();
    HRESULT InitErrorStatement();
    HRESULT InitCertTimestamp();
    VOID    InitCertViewPropertiesEntryPoint();
    LPWSTR  GetFormattedCertTimestamp(LPSYSTEMTIME pst);
    BOOL    IsTestCertInPublisherChain();
    VOID    InitAdvancedLink();
    VOID    InitTestCertInChain();
    VOID    InitControlWebPage();
    VOID    InitCAWebPage();
};

//
// Error mapping helper
//

HRESULT ACUIMapErrorToString (HRESULT hr, LPWSTR* ppsz);

//
// Inline methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::CallCertViewProperties, public
//
//  Synopsis:   calls the cert view properties entry point
//
//  Arguments:  [hwndParent] -- parent window handle
//
//  Returns:    Result of CertViewPropertiesW call
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL
CInvokeInfoHelper::CallCertViewProperties (HWND hwndParent)
{
    CRYPT_PROVIDER_SGNR             *pSgnr;
    CRYPT_PROVIDER_CERT             *pCert;

    //
    // Setup the common dialog call structure
    //

    CVP_STRUCTDEF                   cvsa;

    memset(&cvsa, 0, sizeof(CVP_STRUCTDEF));

    cvsa.dwSize                             = sizeof(CVP_STRUCTDEF);
    cvsa.hwndParent                         = hwndParent;

#   if (USE_IEv4CRYPT32)
        cvsa.hInstance                          = g_hModule;
#   else
        cvsa.pCryptProviderData                 = ProviderData();
        cvsa.fpCryptProviderDataTrustedUsage    = (m_pInvokeInfo->hrInvokeReason == ERROR_SUCCESS) ? TRUE : FALSE;
#   endif

    if (pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0))
    {
        if (pCert = WTHelperGetProvCertFromChain(pSgnr, 0))
        {
            cvsa.pCertContext = pCert->pCert;
        }
    }

    //
    // Bring up the dialog
    //
#   if (USE_IEv4CRYPT32)
        (*m_pfnCVPA)(&cvsa);
#   else
        (*m_pfnCVPA)(&cvsa, NULL);  // TBDTBD: &fRefresh: show dialog
#   endif

    return( TRUE );
}

inline VOID
CInvokeInfoHelper::CallAdvancedLink (HWND hwndParent)
{
    if ((ProviderData()) &&
        (ProviderData()->psPfns->psUIpfns) &&
        (ProviderData()->psPfns->psUIpfns->pfnOnAdvancedClick))
    {
        (*ProviderData()->psPfns->psUIpfns->pfnOnAdvancedClick)(hwndParent, ProviderData());
    }
}

inline VOID 
CInvokeInfoHelper::CallWebLink(HWND hwndParent, WCHAR *pszLink)
{ 
    TUIGoLink(hwndParent, pszLink); 
}

#endif
