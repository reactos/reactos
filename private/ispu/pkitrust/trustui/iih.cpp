//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       iih.cpp
//
//  Contents:   ACUI Invoke Info Helper class implementation
//
//  History:    10-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <stdpch.h>

#include    "malloc.h"
#include    "sgnerror.h"
//
// Personal trust database interface id
//

extern "C" const GUID IID_IPersonalTrustDB = IID_IPersonalTrustDB_Data;
//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::CInvokeInfoHelper, public
//
//  Synopsis:   Constructor, initializes member variables from data found
//              in the invoke info data structure
//
//  Arguments:  [pInvokeInfo] -- invoke info
//              [rhr]         -- result of construction
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CInvokeInfoHelper::CInvokeInfoHelper (
                          PACUI_INVOKE_INFO pInvokeInfo,
                          HRESULT&          rhr
                          )
                  : m_pInvokeInfo ( pInvokeInfo ),
                    m_pszSubject ( NULL ),
                    m_pszPublisher ( NULL ),
                    m_pszAdvancedLink ( NULL ),
                    m_pszControlWebPage ( NULL ),
                    m_pszCAWebPage ( NULL ),
                    m_pszPublisherCertIssuer ( NULL ),
                    m_pszErrorStatement ( NULL ),
                    m_pszCertTimestamp ( NULL ),
                    m_pszTestCertInChain ( NULL ),
                    m_fKnownPublisher ( FALSE ),
                    m_hModCVPA ( NULL ),
                    m_pfnCVPA ( NULL )
{
    //
    // Initialize the subject
    //

    rhr = InitSubject();

    //
    //  if there's a test cert, format the text!
    //
    InitTestCertInChain();

    //
    // If we actually have a signature then ...
    //

        //
        // If we need an error statement, initialize it
        //

    if ( ( rhr == S_OK ) && ( pInvokeInfo->hrInvokeReason != S_OK ) )
    {
        rhr = InitErrorStatement();
    }

    if ( ( rhr == S_OK ) &&
         ( pInvokeInfo->hrInvokeReason != TRUST_E_NOSIGNATURE ) )
    {
        //
        // Initialize the publisher
        //

        rhr = InitPublisher();

        //
        // If we have a known publisher, then we initialize the publisher
        // cert issuer
        //

        if ( ( rhr == S_OK ) && ( m_fKnownPublisher == TRUE ) )
        {
            rhr = InitPublisherCertIssuer();
        }

        //
        // Initialize the timestamp string
        //

        if ( rhr == S_OK )
        {
            rhr = InitCertTimestamp();
        }

        //
        //  initialize the "advanced link" text
        //
        InitAdvancedLink();

        //
        //  initialize the Control's Web page link
        //
        InitControlWebPage();

        //
        //  initialize the CA's Web page link
        //
        InitCAWebPage();
    }

    //
    // Initialize the CertViewProperties entry point
    //

    if ( rhr == S_OK )
    {
        InitCertViewPropertiesEntryPoint();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::~CInvokeInfoHelper, public
//
//  Synopsis:   Destructor, frees up member variables
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
CInvokeInfoHelper::~CInvokeInfoHelper ()
{
    DELETE_OBJECT(m_pszSubject);
    DELETE_OBJECT(m_pszPublisher);
    DELETE_OBJECT(m_pszPublisherCertIssuer);
    DELETE_OBJECT(m_pszAdvancedLink);
    DELETE_OBJECT(m_pszControlWebPage);
    DELETE_OBJECT(m_pszCAWebPage);
    DELETE_OBJECT(m_pszTestCertInChain);
    DELETE_OBJECT(m_pszCertTimestamp);
    DELETE_OBJECT(m_pszErrorStatement);

    if ( m_hModCVPA != NULL )
    {
        FreeLibrary(m_hModCVPA);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::AddPublisherToPersonalTrust, public
//
//  Synopsis:   adds the current publisher to the personal trust database
//
//  Arguments:  (none)
//
//  Returns:    hr == S_OK, publisher added to personal trust database
//              hr != S_OK, publisher NOT added to personal trust database
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::AddPublisherToPersonalTrust ()
{
    HRESULT           hr = S_OK;
    IPersonalTrustDB* pTrustDB = NULL;

    //
    // Get the personal trust database interface
    //

    hr = m_pInvokeInfo->pPersonalTrustDB->QueryInterface(
                                               IID_IPersonalTrustDB,
                                               (LPVOID *)&pTrustDB
                                               );

    //
    // Add the publisher cert to the database
    //

    if ( hr == S_OK )
    {
        CRYPT_PROVIDER_SGNR     *pSgnr;
        CRYPT_PROVIDER_CERT     *pCert;

        if (pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0))
        {
            if (pCert = WTHelperGetProvCertFromChain(pSgnr, 0))
            {
                hr = pTrustDB->AddTrustCert(
                                  pCert->pCert,
                                  0,
                                  FALSE
                                  );
            }
        }

        pTrustDB->Release();
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::GetUIControl, public
//
//  Synopsis:   gets the UI control
//
//  Arguments:  [ppUI] -- UI returned here
//
//  Returns:    S_OK for success, any other valid HRESULT otherwise
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::GetUIControl (IACUIControl** ppUI)
{
    HRESULT       hr = S_OK;
    IACUIControl* pUI = NULL;

    //
    // Get the right UI control
    //

    switch (m_pInvokeInfo->hrInvokeReason)
    {
        case S_OK:
            pUI = new CVerifiedTrustUI(*this, hr);
            break;

        case CRYPT_E_FILE_ERROR:
        case TRUST_E_PROVIDER_UNKNOWN:
        case TRUST_E_SUBJECT_FORM_UNKNOWN:
        case TRUST_E_NOSIGNATURE:
            pUI = new CNoSignatureUI(*this, hr);
            break;

        default:
            pUI = new CUnverifiedTrustUI(*this, hr);
            break;
    }

    //
    // Set the out parameter and return value
    //

    if ( ( pUI != NULL ) && ( hr == S_OK ) )
    {
        *ppUI = pUI;
    }
    else if ( pUI == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        delete pUI;
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::ReleaseUIControl, public
//
//  Synopsis:   frees the UI control
//
//  Arguments:  [pUI] -- UI control
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
CInvokeInfoHelper::ReleaseUIControl (IACUIControl* pUI)
{
    delete pUI;
}

VOID CInvokeInfoHelper::InitControlWebPage ()
{
    WCHAR   *pwsz;

    if (!(m_pInvokeInfo->pOpusInfo))
    {
        return;
    }
    if (!(m_pInvokeInfo->pOpusInfo->pMoreInfo))
    {
        return;
    }

    pwsz = GetGoLink(m_pInvokeInfo->pOpusInfo->pMoreInfo);

    if (!(pwsz))
    {
        return;
    }

    m_pszControlWebPage = new WCHAR[wcslen(pwsz) + 1];

    if (m_pszControlWebPage != NULL)
    {
        wcscpy(m_pszControlWebPage, pwsz);
    }
}

VOID CInvokeInfoHelper::InitCAWebPage ()
{

    //
    //  until IE submits....  don't do it!
    //

    return;


    WCHAR                   *pwsz;
    DWORD                   cb;

    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_CERT     *pCert;
    SPC_SP_AGENCY_INFO      *pAgencyInfo;

    if (!(pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0)))
    {
        return;
    }
    if (!(pCert = WTHelperGetProvCertFromChain(pSgnr, 0))) // try the publisher's cert first!
    {
        return;
    }

    cb = 0;
    WTHelperGetAgencyInfo(pCert->pCert, &cb, NULL);

    if (cb < 1)
    {
        if (!(pCert = WTHelperGetProvCertFromChain(pSgnr, 1)))  // try the issuer's next
        {
            return;
        }

        cb = 0;
        WTHelperGetAgencyInfo(pCert->pCert, &cb, NULL);

        if (cb < 1)
        {
            return;
        }
    }

    if (!(pAgencyInfo = (SPC_SP_AGENCY_INFO *)new BYTE[cb]))
    {
        return;
    }

    if (!(WTHelperGetAgencyInfo(pCert->pCert, &cb, pAgencyInfo)))
    {
        delete pAgencyInfo;
        return;
    }

    pwsz = GetGoLink(pAgencyInfo->pPolicyInformation);

    m_pszCAWebPage = new WCHAR[wcslen(pwsz) + 1];
   
    if (m_pszCAWebPage != NULL)
    {
        wcscpy(m_pszCAWebPage, pwsz);
    }
    
    delete pAgencyInfo;
}


//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitSubject, private
//
//  Synopsis:   Initialize m_pszSubject
//
//  Arguments:  (none)
//
//  Returns:    hr == S_OK, initialize succeeded
//              hr != S_OK, initialize failed
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::InitSubject ()
{
    HRESULT hr = S_OK;
    LPCWSTR pwszSubject = NULL;

    //
    // Find out what we will use as the subject name
    //

    if ( ( m_pInvokeInfo->pOpusInfo != NULL ) &&
         ( m_pInvokeInfo->pOpusInfo->pwszProgramName != NULL ) )
    {
        pwszSubject = m_pInvokeInfo->pOpusInfo->pwszProgramName;
    }
    else
    {
        pwszSubject = m_pInvokeInfo->pwcsAltDisplayName;
    }

    //
    // At this point we must have a valid subject name
    //

    assert( pwszSubject != NULL );

    //
    // Fill in the subject member by converting the one we found from
    // UNICODE to MBS
    //

    m_pszSubject = new WCHAR[wcslen(pwszSubject) + 1];

    if ( m_pszSubject != NULL )
    {
        wcscpy(m_pszSubject, pwszSubject);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return( hr );
}

VOID
CInvokeInfoHelper::InitTestCertInChain ()
{
    WCHAR    szTestCertInChain[MAX_LOADSTRING_BUFFER + 1];

    if (IsTestCertInPublisherChain())
    {
        if ( LoadStringU(
                 g_hModule,
                 IDS_TESTCERTINCHAIN,
                 szTestCertInChain,
                 MAX_LOADSTRING_BUFFER
                 ) == 0 )
        {
            return;
        }

        m_pszTestCertInChain = new WCHAR[wcslen(szTestCertInChain) + 1];
        if (m_pszTestCertInChain != NULL)
        {
            wcscpy(m_pszTestCertInChain, szTestCertInChain);
        }
    }
}

VOID
CInvokeInfoHelper::InitAdvancedLink ()
{
    ULONG   cbAL;

    if ((ProviderData()) &&
        (WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(CRYPT_PROVIDER_FUNCTIONS, ProviderData()->psPfns->cbStruct, psUIpfns)) &&
        (ProviderData()->psPfns->psUIpfns) &&
        (ProviderData()->psPfns->psUIpfns->psUIData) &&
        (ProviderData()->psPfns->psUIpfns->psUIData->pAdvancedLinkText))
    {
        m_pszAdvancedLink = new WCHAR[wcslen(ProviderData()->psPfns->psUIpfns->psUIData->pAdvancedLinkText) + 1];
        
        if (m_pszAdvancedLink != NULL)
        {
            wcscpy(m_pszAdvancedLink, ProviderData()->psPfns->psUIpfns->psUIData->pAdvancedLinkText);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitPublisher, private
//
//  Synopsis:   Initialize m_pszPublisher
//
//  Arguments:  (none)
//
//  Returns:    hr == S_OK, initialize succeeded
//              hr != S_OK, initialize failed
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::InitPublisher ()
{
    HRESULT hr = S_OK;
    ULONG   cchPublisher;
    LPWSTR  pwszPublisher = NULL;
    WCHAR   szPublisher[MAX_LOADSTRING_BUFFER];

    //
    // Load the unknown publisher string
    //

    if ( LoadStringU(
             g_hModule,
             IDS_UNKNOWNPUBLISHER,
             szPublisher,
             MAX_LOADSTRING_BUFFER
             ) == 0 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // Since the publisher is the subject of the signer certificate, we try to
    // find the publisher name in the common name extensions of that cert
    //

    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_CERT     *pCert;

    if (pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0))
    {
        if (pCert = WTHelperGetProvCertFromChain(pSgnr, 0))
        {


            cchPublisher = CertGetNameStringW(pCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);

            if (cchPublisher > 1)
            {
                pwszPublisher = new WCHAR[cchPublisher];
                if ( pwszPublisher == NULL )
                {
                    return (E_OUTOFMEMORY);
                }
                cchPublisher = CertGetNameStringW(pCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL,
                                                 pwszPublisher, cchPublisher);
            }
        }
    }


    //
    // If we still don't have a publisher, use the unknown publisher string
    //

    if ( pwszPublisher == NULL )
    {
        m_fKnownPublisher = FALSE;
        cchPublisher = wcslen(szPublisher) + 1;
    }
    else
    {
        m_fKnownPublisher = TRUE;
        cchPublisher = wcslen(pwszPublisher) + 1;
    }

    //
    // Fill in the publisher member by converting from UNICODE to MBS
    // or by copying the unknown publisher string
    //

    m_pszPublisher = new WCHAR[cchPublisher];

    if ( m_pszPublisher != NULL )
    {
        if ( m_fKnownPublisher == FALSE )
        {
            wcscpy(m_pszPublisher, szPublisher);
        }
        else 
        {
            wcscpy(m_pszPublisher, pwszPublisher);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if ( pwszPublisher != NULL )
    {
        delete[] pwszPublisher;
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitPublisherCertIssuer, private
//
//  Synopsis:   Initialize m_pszPublisherCertIssuer
//
//  Arguments:  (none)
//
//  Returns:    hr == S_OK, initialize succeeded
//              hr != S_OK, initialize failed
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::InitPublisherCertIssuer ()
{
    HRESULT hr = S_OK;
    ULONG   cchCertIssuer;
    LPWSTR  pwszCertIssuer = NULL;
    WCHAR   szCertIssuer[MAX_LOADSTRING_BUFFER];
    BOOL    fKnownCertIssuer;

    //
    // Load the unknown cert issuer string
    //

    if ( LoadStringU(
             g_hModule,
             IDS_UNKNOWNPUBLISHERCERTISSUER,
             szCertIssuer,
             MAX_LOADSTRING_BUFFER
             ) == 0 )
    {
        return( HRESULT_FROM_WIN32(GetLastError()) );
    }

    //
    // Since the publisher cert issuer is the issuer of the signer certificate,
    // we try to find the name in the RDN attributes of the cert issuer
    //

    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_CERT     *pCert;

    if (pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0))
    {
        if (pCert = WTHelperGetProvCertFromChain(pSgnr, 0))
        {
            cchCertIssuer = CertGetNameStringW(pCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL,
                                                NULL, 0);

            if (cchCertIssuer > 1)
            {
                pwszCertIssuer = new WCHAR[cchCertIssuer];
                if ( pwszCertIssuer == NULL)
                {
                    return (E_OUTOFMEMORY);
                }
                cchCertIssuer = CertGetNameStringW(pCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL,
                                                   pwszCertIssuer, cchCertIssuer);
            }
        }
    }

    //
    // If we still don't have a name, we set the unknown issuer string
    //

    if ( pwszCertIssuer == NULL )
    {
        fKnownCertIssuer = FALSE;
        cchCertIssuer = wcslen(szCertIssuer) + 1;
    }
    else
    {
        fKnownCertIssuer = TRUE;
        cchCertIssuer = wcslen(pwszCertIssuer) + 1;
    }

    //
    // Fill in the publisher cert issuer member by converting from UNICODE
    // to MBS or by copying the unknown issuer string
    //

    m_pszPublisherCertIssuer = new WCHAR[cchCertIssuer];

    if ( m_pszPublisherCertIssuer != NULL )
    {
        if ( fKnownCertIssuer == FALSE )
        {
            wcscpy(m_pszPublisherCertIssuer, szCertIssuer);
        }
        else
        {
            wcscpy(m_pszPublisherCertIssuer, pwszCertIssuer);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if ( pwszCertIssuer != NULL )
    {
        delete[] pwszCertIssuer;
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitErrorStatement, private
//
//  Synopsis:   Initialize m_pszErrorStatement
//
//  Arguments:  (none)
//
//  Returns:    hr == S_OK, initialize succeeded
//              hr != S_OK, initialize failed
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::InitErrorStatement ()
{
    return( ACUIMapErrorToString(
                        m_pInvokeInfo->hrInvokeReason,
                        &m_pszErrorStatement
                        ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitCertTimestamp, public
//
//  Synopsis:   initialize the certificate timestamp string
//
//----------------------------------------------------------------------------
HRESULT
CInvokeInfoHelper::InitCertTimestamp ()
{
    HRESULT    hr = S_OK;
    WCHAR      szCertTimestamp[MAX_LOADSTRING_BUFFER];
    FILETIME   ftTimestamp;
    SYSTEMTIME stTimestamp;


    //
    // Get the time stamp
    //

    // TBDTBD: change to a loop!!!! pberkman

    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_SGNR     *pTimeSgnr;

    if ((pTimeSgnr =
            WTHelperGetProvSignerFromChain(ProviderData(), 0, TRUE, 0)) &&
        (pTimeSgnr->dwSignerType & SGNR_TYPE_TIMESTAMP) &&
        (pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0)))
    {
        // convert UTC to local
        FileTimeToLocalFileTime(&pSgnr->sftVerifyAsOf, &ftTimestamp);

        // make it system format
        FileTimeToSystemTime(&ftTimestamp, &stTimestamp);

        m_pszCertTimestamp = GetFormattedCertTimestamp(&stTimestamp);

        if ( m_pszCertTimestamp == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        m_pszCertTimestamp = NULL;
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::InitCertViewPropertiesEntryPoint, public
//
//  Synopsis:   initialize the cert view properties entry point
//
//  Arguments:  (none)
//
//  Returns:    (none)
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
CInvokeInfoHelper::InitCertViewPropertiesEntryPoint ()
{
    m_hModCVPA = LoadLibraryA(CVP_DLL);

    if ( m_hModCVPA != NULL )
    {
        m_pfnCVPA = (pfnCertViewProperties)GetProcAddress(m_hModCVPA, CVP_FUNC_NAME);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::GetFormattedCertTimestamp, public
//
//  Synopsis:   gets the formatted cert timestamp string which will be
//              allocated using the new operator
//
//----------------------------------------------------------------------------
LPWSTR
CInvokeInfoHelper::GetFormattedCertTimestamp (LPSYSTEMTIME pst)
{
    LPWSTR  psz;
    int     cDate;
    int     cTime;

    if ( ( cDate = GetDateFormatU(
                          LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          pst,
                          NULL,
                          NULL,
                          0
                          ) ) == 0 )
    {
        return( NULL );
    }

    cDate--;

    if ( ( cTime = GetTimeFormatU(
                          LOCALE_USER_DEFAULT,
                          TIME_NOSECONDS,
                          pst,
                          NULL,
                          NULL,
                          0
                          ) ) == 0 )
    {
        return( NULL );
    }

    cTime--;

    psz = new WCHAR [ cDate + cTime + 2 ];
    if ( psz == NULL )
    {
        return( NULL );
    }

    if ( GetDateFormatU(
                LOCALE_USER_DEFAULT,
                DATE_SHORTDATE,
                pst,
                NULL,
                psz,
                cDate + 1
                ) == 0 )
    {
        delete[] psz;
        return( NULL );
    }

    psz[cDate] = L' ';

    if ( GetTimeFormatU(
                LOCALE_USER_DEFAULT,
                TIME_NOSECONDS,
                pst,
                NULL,
                &psz[cDate+1],
                cTime + 1
                ) == 0 )
    {
        delete[] psz;
        return( NULL );
    }

    return( psz );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInvokeInfoHelper::IsTestCertInChain, public
//
//  Synopsis:   is there a test cert in the publisher's chain
//
//----------------------------------------------------------------------------
BOOL
CInvokeInfoHelper::IsTestCertInPublisherChain ()
{
    ULONG cCount;

    CRYPT_PROVIDER_SGNR     *pSgnr;
    CRYPT_PROVIDER_CERT     *pCert;

    if (pSgnr = WTHelperGetProvSignerFromChain(ProviderData(), 0, FALSE, 0))
    {
        for (cCount = 0; cCount < pSgnr->csCertChain; cCount++)
        {
            if (pCert = WTHelperGetProvCertFromChain(pSgnr, cCount))
            {
                if (pCert->fTestCert)
                {
                    return(TRUE);
                }
            }
        }
    }

    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   ACUIMapErrorToString
//
//  Synopsis:   maps error to string
//
//  Arguments:  [hr]   -- error
//              [ppsz] -- error string goes here
//
//  Returns:    S_OK if successful, any valid HRESULT otherwise
//
//----------------------------------------------------------------------------
HRESULT ACUIMapErrorToString (HRESULT hr, LPWSTR* ppsz)
{
    UINT  ResourceId = 0;
    WCHAR psz[MAX_LOADSTRING_BUFFER];

    //
    // See if it maps to some non system error code
    //

    switch (hr)
    {

        case TRUST_E_SYSTEM_ERROR:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INVALID_PARAMETER:
            //
            //  leave the resourceid zero...  these will be mapped to
            //  IDS_SPC_UNKNOWN and the error code displayed.
            //
            break;

        case CRYPT_E_FILE_ERROR:
            ResourceId = IDS_FILE_NOT_FOUND;
            break;

        case TRUST_E_PROVIDER_UNKNOWN:
            ResourceId = IDS_SPC_PROVIDER;
            break;

        case TRUST_E_SUBJECT_FORM_UNKNOWN:
            ResourceId = IDS_SPC_SUBJECT;
            break;

        case TRUST_E_NOSIGNATURE:
            ResourceId = IDS_SPC_NO_SIGNATURE;
            break;

        case CRYPT_E_BAD_MSG:
            ResourceId = IDS_SPC_BAD_SIGNATURE;
            break;

        case TRUST_E_BAD_DIGEST:
            ResourceId = IDS_SPC_BAD_FILE_DIGEST;
            break;

        case CRYPT_E_NO_SIGNER:
            ResourceId = IDS_SPC_NO_VALID_SIGNER;
            break;

        case TRUST_E_NO_SIGNER_CERT:
            ResourceId = IDS_SPC_SIGNER_CERT;
            break;

        case TRUST_E_COUNTER_SIGNER:
            ResourceId = IDS_SPC_VALID_COUNTERSIGNER;
            break;

        case CERT_E_EXPIRED:
            ResourceId = IDS_SPC_CERT_EXPIRED;
            break;

        case TRUST_E_CERT_SIGNATURE:
            ResourceId = IDS_SPC_CERT_SIGNATURE;
            break;

        case CERT_E_CHAINING:
            ResourceId = IDS_SPC_CHAINING;
            break;

        case CERT_E_UNTRUSTEDROOT:
            ResourceId = IDS_SPC_UNTRUSTED_ROOT;
            break;

        case CERT_E_UNTRUSTEDTESTROOT:
            ResourceId = IDS_SPC_UNTRUSTED_TEST_ROOT;
            break;

        case CERT_E_VALIDITYPERIODNESTING:
            ResourceId = IDS_SPC_INVALID_CERT_NESTING;
            break;

        case CERT_E_PURPOSE:
            ResourceId = IDS_SPC_INVALID_PURPOSE;
            break;

        case TRUST_E_BASIC_CONSTRAINTS:
            ResourceId = IDS_SPC_INVALID_BASIC_CONSTRAINTS;
            break;

        case TRUST_E_FINANCIAL_CRITERIA:
            ResourceId = IDS_SPC_INVALID_FINANCIAL;
            break;

        case TRUST_E_TIME_STAMP:
            ResourceId = IDS_SPC_TIMESTAMP;
            break;

        case CERT_E_REVOKED:
            ResourceId = IDS_SPC_CERT_REVOKED;
            break;

        case CERT_E_REVOCATION_FAILURE:
            ResourceId = IDS_SPC_REVOCATION_ERROR;
            break;

        case CRYPT_E_SECURITY_SETTINGS:
            ResourceId = IDS_SPC_SECURITY_SETTINGS;
            break;

        case CERT_E_MALFORMED:
            ResourceId = IDS_SPC_INVALID_EXTENSION;
            break;

        case CERT_E_WRONG_USAGE:
            ResourceId = IDS_WRONG_USAGE;
            break;
    }

    //
    // If it does, load the string out of our resource string tables and
    // return that. Otherwise, try to format the message from the system
    //
    // BUGBUG: Need to deal with possible OSS errors and unknown errors
    //         0x80093000 - 0x80093999
    //

    DWORD_PTR MessageArgument;
    CHAR  szError[13]; // for good luck
    WCHAR  wszError[13]; // for good luck
    LPVOID  pvMsg;

    pvMsg = NULL;

    if ( ResourceId != 0 )
    {
        if ( LoadStringU(
                 g_hModule,
                 ResourceId,
                 psz,
                 MAX_LOADSTRING_BUFFER
                 ) == 0 )
        {
            return( HRESULT_FROM_WIN32(GetLastError()) );
        }

        *ppsz = new WCHAR[wcslen(psz) + 1];

        if ( *ppsz != NULL )
        {
            wcscpy(*ppsz, psz);
        }
        else
        {
            return( E_OUTOFMEMORY );
        }
    }
    else if ( ( hr >= 0x80093000 ) && ( hr <= 0x80093999 ) )
    {
        if ( LoadStringU(
                 g_hModule,
                 IDS_SPC_OSS_ERROR,
                 psz,
                 MAX_LOADSTRING_BUFFER
                 ) == 0 )
        {
            return( HRESULT_FROM_WIN32(GetLastError()) );
        }

        sprintf(szError, "%lx", hr);
        MultiByteToWideChar(0, 0, szError, -1, &wszError[0], 13);
        MessageArgument = (DWORD_PTR)wszError;

        if ( FormatMessageU(
                   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_STRING |
                   FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   psz,
                   0,
                   0,
                   (LPWSTR)&pvMsg,
                   0,
                   (va_list *)&MessageArgument
                   ) == 0 )
        {
            return( HRESULT_FROM_WIN32(GetLastError()) );
        }
    }
    else
    {
        if ( FormatMessageU(
                   FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   hr,
                   0,
                   (LPWSTR)&pvMsg,
                   0,
                   NULL
                   ) == 0 )
        {
            if ( LoadStringU(
                    g_hModule,
                    IDS_SPC_UNKNOWN,
                    psz,
                    MAX_LOADSTRING_BUFFER
                    ) == 0 )
            {
                return( HRESULT_FROM_WIN32(GetLastError()) );
            }

            sprintf(szError, "%lx", hr);
            MultiByteToWideChar(0, 0, szError, -1, &wszError[0], 13);
            MessageArgument = (DWORD_PTR)wszError;

            if ( FormatMessageU(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    psz,
                    0,
                    0,
                    (LPWSTR)&pvMsg,
                    0,
                    (va_list *)&MessageArgument
                    ) == 0 )
            {
                return( HRESULT_FROM_WIN32(GetLastError()) );
            }
        }
    }

    if (pvMsg)
    {
        *ppsz = new WCHAR[wcslen((WCHAR *)pvMsg) + 1];

        if (*ppsz)
        {
            wcscpy(*ppsz, (WCHAR *)pvMsg);
        }

        LocalFree(pvMsg);
    }

    return( S_OK );
}


