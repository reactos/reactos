//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cnet.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <iapp.h>
#include <winineti.h>   // contains bogus INTERNET_FLAG_NO_UI
#include <shlwapip.h>

CMutexSem g_mxsSession; // single access to InternetOpen


PerfDbgTag(tagCINet,    "Urlmon", "Log CINet",         DEB_PROT);
PerfDbgTag(tagCINetErr, "Urlmon", "Log CINet Errors",  DEB_PROT|DEB_ERROR);
LONG UrlMonInvokeExceptionFilter( DWORD lCode, LPEXCEPTION_POINTERS lpep );
DWORD StrLenMultiByteWithMlang(LPCWSTR, DWORD);
BOOL  ConvertUnicodeUrl(LPCWSTR, LPSTR, INT, DWORD, BOOL);
DWORD CountUnicodeToUtf8(LPCWSTR, DWORD, BOOL); 
extern BOOL g_bGlobalUTF8Enabled;

#define INTERNET_POLICIES_KEY    "SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

// Need this in GetUserAgentString.
HRESULT CallRegInstall(LPSTR szSection);

class CInetState
{
    enum WHAT_STATE
    {
         INETSTATE_NONE      = 0
        ,INETSTATE_SET       = 1
        ,INETSTATE_HANDLED   = 2
    };

public:
    void SetState(DWORD dwState)
    {
        CLock lck(_mxs);
        _dwState = dwState;
        _What = INETSTATE_SET;

    }
    HRESULT HandleState()
    {
        HRESULT hr = NOERROR;
        DWORD dwStateNow;
        BOOL fPropagate;

        {
            CLock lck(_mxs);
            dwStateNow = _dwState;
            fPropagate = (_What == INETSTATE_SET) ? TRUE : FALSE;
            _What = INETSTATE_HANDLED;
        }

        if (fPropagate)
        {
            hr = PropagateStateChange(dwStateNow);
        }

        return hr;
    }

    HRESULT PropagateStateChange(DWORD dwState);

    CInetState()
    {
        _dwState = 0;
        _What = INETSTATE_NONE;
    }

    ~CInetState()
    {
    }

private:
    CMutexSem   _mxs;
    DWORD       _dwState;
    WHAT_STATE  _What;
};

//+---------------------------------------------------------------------------
//
//  Method:     CInetState::PropagateStateChange
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-22-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CInetState::PropagateStateChange(DWORD dwWhat)
{
    PerfDbgLog(tagCINet, this, "+CInetState::PropagateStateChange");
    HRESULT hr = NOERROR;

//  BUGBUG: IE5 need to implement these!
#if 0
    if (dwWhat & INTERNET_STATE_CONNECTED)
    {
        NotfDeliverNotification(
                                NOTIFICATIONTYPE_CONNECT_TO_INTERNET  // REFNOTIFICATIONTYPE rNotificationType
                               ,CLSID_PROCESS_BROADCAST     // REFCLSID            rClsidDest
                               ,(DELIVERMODE)0              // DELIVERMODE         deliverMode
                               ,0                           // DWORD               dwReserved
                               );

    }
    if (dwWhat & INTERNET_STATE_DISCONNECTED)
    {
        NotfDeliverNotification(
                                NOTIFICATIONTYPE_DISCONNECT_FROM_INTERNET  // REFNOTIFICATIONTYPE rNotificationType
                               ,CLSID_PROCESS_BROADCAST     // REFCLSID            rClsidDest
                               ,(DELIVERMODE)0              // DELIVERMODE         deliverMode
                               ,0                           // DWORD               dwReserved
                               );
    }
    if (dwWhat & INTERNET_STATE_DISCONNECTED_BY_USER)
    {
        NotfDeliverNotification(
                                NOTIFICATIONTYPE_CONFIG_CHANGED  // REFNOTIFICATIONTYPE rNotificationType
                               ,CLSID_PROCESS_BROADCAST     // REFCLSID            rClsidDest
                               ,(DELIVERMODE)0              // DELIVERMODE         deliverMode
                               ,0                           // DWORD               dwReserved
                               );

    }

    #ifdef _with_idle_and_busy_
        //
        // NOTE: wininet will send idle with every state change
        //       will be fixed by RFirth
        if (dwWhat & INTERNET_STATE_IDLE)
        {
            NotfDeliverNotification(
                                    NOTIFICATIONTYPE_INET_IDLE  // REFNOTIFICATIONTYPE rNotificationType
                                   ,CLSID_PROCESS_BROADCAST     // REFCLSID            rClsidDest
                                   ,(DELIVERMODE)0              // DELIVERMODE         deliverMode
                                   ,0                           // DWORD               dwReserved
                                   );
        }
        if (dwWhat & INTERNET_STATE_BUSY)
        {
            NotfDeliverNotification(
                                    NOTIFICATIONTYPE_INET_BUSY  // REFNOTIFICATIONTYPE rNotificationType
                                   ,CLSID_PROCESS_BROADCAST     // REFCLSID            rClsidDest
                                   ,(DELIVERMODE)0              // DELIVERMODE         deliverMode
                                   ,0                           // DWORD               dwReserved
                                   );

        }
    #endif //_with_idle_and_busy_
    
#endif // 0
    PerfDbgLog1(tagCINet, this, "-CInetState:PropagateStateChange (hr:%lx)", hr);
    return hr;
}

//
// REMOVE THIS joHANNP
//

extern  LPSTR               g_pszUserAgentString;
HINTERNET                   g_hSession;
CInetState                  g_cInetState;

char vszLocationTag[] = "Location: ";

static DWORD dwLstError;

typedef struct tagProtocolInfo
{
    LPWSTR          pwzProtocol;
    LPSTR           pszProtocol;
    DWORD           dwId;
    CLSID           *pClsID;
    int             protlen;
} ProtocolInfo;


//+---------------------------------------------------------------------------
//
//  Function:   CreateKnownProtocolInstance
//
//  Synopsis:
//
//  Arguments:  [dwProtId] --
//              [rclsid] --
//              [pUnkOuter] --
//              [riid] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    11-01-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CreateKnownProtocolInstance(DWORD dwProtId, REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk)
{
    PerfDbgLog(tagCINet, NULL, "+CreateKnownProtocolInstance");
    HRESULT hr = NOERROR;
    CINet *pCINet = NULL;

    PProtAssert((ppUnk));

    if (!ppUnk || (pUnkOuter && (riid != IID_IUnknown)) )
    {
        // Note: aggregation only works if asked for IUnknown
        PProtAssert((FALSE && "Dude, look up aggregation rules - need to ask for IUnknown"));
        hr = E_INVALIDARG;
    }
    #if 0
    else if (riid == IID_IOInetProtocolInfo)
    {
        PProtAssert((dwProtId != DLD_PROTOCOL_NONE));
        COInetProtocolInfo *pProtInfo = new COInetProtocolInfo(dwProtId);
        if (pProtInfo)
        {
            hr = pProtInfo->QueryInterface(IID_IOInetProtocolInfo, (void **)ppUnk);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    #endif // 0
    else if (dwProtId != DLD_PROTOCOL_NONE)
    {
        CINet *pCINet = NULL;

        switch (dwProtId)
        {
        case  DLD_PROTOCOL_LOCAL  :
        case  DLD_PROTOCOL_FILE   :
            pCINet = new CINetFile(CLSID_FileProtocol,pUnkOuter);
        break;
        case  DLD_PROTOCOL_HTTP   :
            pCINet = new CINetHttp(CLSID_HttpProtocol,pUnkOuter);
        break;
        case  DLD_PROTOCOL_FTP    :
            pCINet = new CINetFtp(CLSID_FtpProtocol,pUnkOuter);
        break;
        case  DLD_PROTOCOL_GOPHER :
            pCINet = new CINetGopher(CLSID_GopherProtocol,pUnkOuter);
        break;
        case  DLD_PROTOCOL_HTTPS  :
            pCINet = new CINetHttpS(CLSID_HttpSProtocol,pUnkOuter);
        break;
        case  DLD_PROTOCOL_STREAM :
            pCINet = new CINetStream(CLSID_MkProtocol,pUnkOuter);
        break;
        default:
            PProtAssert((FALSE));
            hr = E_FAIL;
        }

        if (hr == NOERROR)
        {
            if (pCINet)
            {
                if (riid == IID_IUnknown)
                {
                    // pCINet has refcount of 1 now

                    // get the pUnkInner
                    // pUnkInner does not addref pUnkOuter
                    if (pUnkOuter && ppUnk)
                    {
                        *ppUnk = pCINet->GetIUnkInner(TRUE);
                        // addref the outer object since releasing pCINet will go cause a release on pUnkOuter
                        TransAssert((*ppUnk));
                    }
                }
                else
                {
                    if (riid == IID_IOInetProtocol)
                    {
                        // ok, got the right interface already
                        *ppUnk = (IOInetProtocol *)pCINet;
                    }
                    else
                    {
                        hr = pCINet->QueryInterface(riid, (void **)ppUnk);
                        // remove extra refcount
                        pCINet->Release();
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
    }
    else
    {
        //load the protocol by looking up the registry
        hr = E_FAIL;
    }


    PerfDbgLog1(tagCINet, NULL, "-CreateKnownProtocolInstance(hr:%lx)", hr);
    return hr;
}

ProtocolInfo rgKnownProts[] =
{
     { L"http"  , "http"   , DLD_PROTOCOL_HTTP   , (CLSID *) &CLSID_HttpProtocol  , sizeof("http")   - 1 }
    ,{ L"file"  , "file"   , DLD_PROTOCOL_FILE   , (CLSID *) &CLSID_FileProtocol  , sizeof("file")   - 1 }
    ,{ L"ftp"   , "ftp"    , DLD_PROTOCOL_FTP    , (CLSID *) &CLSID_FtpProtocol   , sizeof("ftp")    - 1 }
    ,{ L"https" , "https"  , DLD_PROTOCOL_HTTPS  , (CLSID *) &CLSID_HttpSProtocol , sizeof("http")   - 1 }
    ,{ L"mk"    , "mk"     , DLD_PROTOCOL_STREAM , (CLSID *) &CLSID_MkProtocol    , sizeof("mk")     - 1 }
    ,{ L"gopher", "Gopher" , DLD_PROTOCOL_GOPHER , (CLSID *) &CLSID_GopherProtocol, sizeof("gopher") - 1 }
    ,{ L"local" , "local"  , DLD_PROTOCOL_LOCAL  , (CLSID *) &CLSID_FileProtocol  , sizeof("local")  - 1 }
};

typedef struct tagInterErrorToHResult
{
    DWORD   dwError;
    HRESULT hresult;
} InterErrorToHResult;

InterErrorToHResult INetError[] =
{
     0                                        ,NOERROR
    ,ERROR_INTERNET_OUT_OF_HANDLES            ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_TIMEOUT                   ,INET_E_CONNECTION_TIMEOUT
    ,ERROR_INTERNET_EXTENDED_ERROR            ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INTERNAL_ERROR            ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INVALID_URL               ,INET_E_INVALID_URL
    ,ERROR_INTERNET_UNRECOGNIZED_SCHEME       ,INET_E_UNKNOWN_PROTOCOL
    ,ERROR_INTERNET_NAME_NOT_RESOLVED         ,INET_E_RESOURCE_NOT_FOUND
    ,ERROR_INTERNET_PROTOCOL_NOT_FOUND        ,INET_E_UNKNOWN_PROTOCOL
    ,ERROR_INTERNET_INVALID_OPTION            ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_BAD_OPTION_LENGTH         ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_OPTION_NOT_SETTABLE       ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_SHUTDOWN                  ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INCORRECT_USER_NAME       ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INCORRECT_PASSWORD        ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_LOGIN_FAILURE             ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INVALID_OPERATION         ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_OPERATION_CANCELLED       ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INCORRECT_HANDLE_TYPE     ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INCORRECT_HANDLE_STATE    ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_NOT_PROXY_REQUEST         ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND  ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_BAD_REGISTRY_PARAMETER    ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_NO_DIRECT_ACCESS          ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_NO_CONTEXT                ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_NO_CALLBACK               ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_REQUEST_PENDING           ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INCORRECT_FORMAT          ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_ITEM_NOT_FOUND            ,INET_E_OBJECT_NOT_FOUND
    ,ERROR_INTERNET_CANNOT_CONNECT            ,INET_E_RESOURCE_NOT_FOUND
    ,ERROR_INTERNET_CONNECTION_ABORTED        ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_CONNECTION_RESET          ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_FORCE_RETRY               ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_INVALID_PROXY_REQUEST     ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_NEED_UI                   ,INET_E_DOWNLOAD_FAILURE
    ,0                                        ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_HANDLE_EXISTS             ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_SEC_CERT_DATE_INVALID     ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_SEC_CERT_CN_INVALID       ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR    ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR    ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_MIXED_SECURITY            ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_CHG_POST_IS_NON_SECURE    ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_POST_IS_NON_SECURE        ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED   ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_INVALID_CA                ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP     ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_SEC_CERT_REV_FAILED       ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_SEC_CERT_REVOKED          ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED     ,INET_E_SECURITY_PROBLEM
    ,ERROR_INTERNET_ASYNC_THREAD_FAILED       ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_REDIRECT_SCHEME_CHANGE    ,INET_E_DOWNLOAD_FAILURE
};

// list of non-sequential errors
InterErrorToHResult INetErrorExtended[] =
{
     ERROR_FTP_TRANSFER_IN_PROGRESS           ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_FTP_DROPPED                        ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_PROTOCOL_ERROR              ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_NOT_FILE                    ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_DATA_ERROR                  ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_END_OF_DATA                 ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_INVALID_LOCATOR             ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_INCORRECT_LOCATOR_TYPE      ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_NOT_GOPHER_PLUS             ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_ATTRIBUTE_NOT_FOUND         ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_GOPHER_UNKNOWN_LOCATOR             ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_HEADER_NOT_FOUND              ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_DOWNLEVEL_SERVER              ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_INVALID_SERVER_RESPONSE       ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_INVALID_HEADER                ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_INVALID_QUERY_REQUEST         ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_HEADER_ALREADY_EXISTS         ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_REDIRECT_FAILED               ,INET_E_REDIRECT_FAILED
    ,ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION   ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_SECURITY_CHANNEL_ERROR    ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_UNABLE_TO_CACHE_FILE      ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_TCPIP_NOT_INSTALLED       ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_HTTP_NOT_REDIRECTED                ,INET_E_DOWNLOAD_FAILURE
    ,ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR   ,INET_E_DOWNLOAD_FAILURE
};

//+---------------------------------------------------------------------------
//
//  Function:   IsKnownOInetProtocolClass
//
//  Synopsis:
//
//  Arguments:  [pclsid] --
//
//  Returns:
//
//  History:    11-01-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD IsKnownOInetProtocolClass(CLSID *pclsid)
{
    PProtAssert((pclsid));
    DWORD dwRet = DLD_PROTOCOL_NONE;
    int i = 0;
    int cSize = sizeof(rgKnownProts)/sizeof(ProtocolInfo);

    if (pclsid)
    {
        for (i = 0; i < cSize; ++i)
        {
            if (*pclsid == *rgKnownProts[i].pClsID)
            {
                dwRet = rgKnownProts[i].dwId;
                i = cSize;
            }
        }
    }

    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsKnownProtocol
//
//  Synopsis:   looks up if an protocol handler exist for a given url
//
//  Arguments:  [wzProtocol] --
//
//  Returns:
//
//  History:    11-01-1996   JohannP (Johann Posch)   Created
//
//  Notes:      used at many place inside urlmon
//
//----------------------------------------------------------------------------
DWORD IsKnownProtocol(LPCWSTR wzProtocol)
{
    DWORD dwRet = DLD_PROTOCOL_NONE;
    int i = 0;
    int cSize = sizeof(rgKnownProts)/sizeof(ProtocolInfo);

    for (i = 0; i < cSize; ++i)
    {
        if (!StrCmpNICW(wzProtocol, rgKnownProts[i].pwzProtocol, rgKnownProts[i].protlen))
        {
            dwRet = rgKnownProts[i].dwId;

            if ((DLD_PROTOCOL_HTTP == dwRet) && 
                ((wzProtocol[4] == L's') ||
                 (wzProtocol[4] == L'S')))

            {
                dwRet = DLD_PROTOCOL_HTTPS;
            }
            break;
        }
    }
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetKnownOInetProtocolClsID
//
//  Synopsis:
//
//  Arguments:  [dwProtoId] --
//
//  Returns:
//
//  History:    11-01-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CLSID *GetKnownOInetProtocolClsID(DWORD dwProtoId)
{
    PerfDbgLog1(tagCINet, NULL, "GetKnownOInetProtocolClsID (dwProtoId:%lx)", dwProtoId);
    CLSID *pclsid = 0;

    int cSize = sizeof(rgKnownProts)/sizeof(ProtocolInfo);

    for (int i = 0; i < cSize; ++i)
    {
        if (dwProtoId == rgKnownProts[i].dwId )
        {
            pclsid = rgKnownProts[i].pClsID;
            i = cSize;
        }
    }

    return pclsid;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCINet, this, "+CINet::QueryInterface");

    if (riid == IID_IOInetPriority)
    {
        *ppvObj = (IOInetPriority *) this;
        AddRef();
    }
    else
    {
        hr = _pUnkOuter->QueryInterface(riid, ppvObj);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CINet::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CINet::AddRef(void)
{
    PerfDbgLog(tagCINet, this, "+CINet::AddRef");

    LONG lRet = _pUnkOuter->AddRef();

    PerfDbgLog1(tagCINet, this, "-CINet::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CINet::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CINet::Release(void)
{
    PerfDbgLog(tagCINet, this, "+CINet::Release");

    LONG lRet = _pUnkOuter->Release();

    PerfDbgLog1(tagCINet, this, "-CINet::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Start
//
//  Synopsis:
//
//  Arguments:  [pwzUrl] --
//              [pTrans] --
//              [pOIBindInfo] --
//              [grfSTI] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Start(LPCWSTR pwzUrl, IOInetProtocolSink *pTrans, IOInetBindInfo *pOIBindInfo,
                          DWORD grfSTI, DWORD_PTR dwReserved)
{
    PerfDbgLog(tagCINet, this, "+CINet::Start");
    HRESULT hr = NOERROR;
    //char    szURL[MAX_URL_SIZE];
    DWORD   dwUrlSize = 0; 
    DWORD dwMaxUTF8Len = 0; 
    BOOL    fUTF8Enabled = FALSE;


    PProtAssert((!_pCTrans && pOIBindInfo && pTrans));
    PProtAssert((_pwzUrl == NULL));


    if ( !(grfSTI & PI_PARSE_URL))
    {
        _pCTrans = pTrans;
        _pCTrans->AddRef();

        _pOIBindInfo =  pOIBindInfo;
        _pOIBindInfo->AddRef();

        _pwzUrl = OLESTRDuplicate((LPWSTR)pwzUrl);
    }

    _BndInfo.cbSize = sizeof(BINDINFO);

    hr = pOIBindInfo->GetBindInfo(&_grfBindF, &_BndInfo);
    if( hr != NOERROR )
    {
        goto End;
    }

    /************************************************************************
    // szURL does not seem to be used, am I missing something?
    // szURL is 2K buffer on stack! Let's remove this code at all.
    // DanpoZ (98/02/20)

    // Do we need to append the extra data to the url?
    if (_BndInfo.szExtraInfo)
    {
        // append extra info at the end of the url
        // Make sure we don't overflow the URL
        if (CchWzLen(_BndInfo.szExtraInfo) + CchWzLen(pwzUrl) >= MAX_URL_SIZE)
        {
            hr = E_INVALIDARG;
            goto End;
        }

        W2A(pwzUrl, szURL, MAX_URL_SIZE, _BndInfo.dwCodePage);

        // Append the extra data to the url.  Note that we have already
        // checked for overflow, so we need not worry about it here.
        W2A(_BndInfo.szExtraInfo, szURL + CchSzLen(szURL), MAX_URL_SIZE, _BndInfo.dwCodePage);
    }
    else
    {
        // Make sure we don't overflow the URL
        if (CchWzLen(pwzUrl) + 1 > MAX_URL_SIZE)
        {
            hr = E_INVALIDARG;
            goto End;
        }
        W2A(pwzUrl, szURL, MAX_URL_SIZE, _BndInfo.dwCodePage);
    }
    ************************************************************************/

    // no need to check the length of pwzUrl, this has been done already
    if( !_BndInfo.dwCodePage )
        _BndInfo.dwCodePage = GetACP();

    
    // utf8 enabled?
    fUTF8Enabled = UTF8Enabled();

    if( fUTF8Enabled )
    { 
        dwUrlSize = CountUnicodeToUtf8(pwzUrl, wcslen(pwzUrl), TRUE);
        DWORD dwMB = StrLenMultiByteWithMlang(pwzUrl, _BndInfo.dwCodePage);
        if( dwMB > dwUrlSize )
        {
            dwUrlSize = dwMB;
        }
    }
    else
    {
        dwUrlSize = StrLenMultiByteWithMlang(pwzUrl, _BndInfo.dwCodePage);
    }

    if( !dwUrlSize )
    {
        hr = E_INVALIDARG;
        goto End;
    }


    if( CUrlInitBasic(dwUrlSize) )
    {
        //W2A(pwzUrl, _pszBaseURL, dwUrlSize, _BndInfo.dwCodePage);
        ConvertUnicodeUrl(
                pwzUrl,                 // (IN)  unicode URL
                _pszBaseURL,            // (OUT) multibyte URL
                dwUrlSize,              // (IN)  multibyte URL length
                _BndInfo.dwCodePage,    // (IN)  codepage 
                fUTF8Enabled            // (IN)  UTF-8 conversion enabled?
                );
    }
    else
    {
        hr = E_OUTOFMEMORY;
        goto End;
    }

    {
        // Init the Embedded Filter 
        if( _pEmbdFilter )
        {
            delete _pEmbdFilter;
            _pEmbdFilter = NULL;
        }
        _pEmbdFilter = new CINetEmbdFilter( this, _pCTrans );
        if( !_pEmbdFilter )
        {
            hr = E_OUTOFMEMORY;
            goto End;
        }
    }

    if (!ParseUrl())
    {
        _hrError = INET_E_INVALID_URL;
        hr = MK_E_SYNTAX;
    }
    else if ( !(grfSTI & PI_PARSE_URL))
    {
        g_cInetState.HandleState();
        PProtAssert((_pCTrans));
        if( !(_grfBindF & BINDF_FROMURLMON) && _pEmbdFilter ) 
        {
            _pEmbdFilter->ReportProgress(BINDSTATUS_DIRECTBIND, 0);
        }
        hr = INetAsyncStart();
    }

End:

    PerfDbgLog1(tagCINet, this, "-CINet::Start (hr:%lx)", hr);
    if( hr == E_PENDING )
    {
        // hack, CTransact will warp E_PENDING with NOERROR and return
        // it to client, if we do not have CTrans wrapper (not aggregrated)
        // we should return NOERROR. 
        if( _pUnkOuter == &_Unknown )
        {
            hr = NOERROR;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Continue
//
//  Synopsis:
//
//  Arguments:  [pStateInfoIn] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Continue(PROTOCOLDATA *pStateInfoIn)
{
    return _pEmbdFilter->Continue(pStateInfoIn);
}

STDMETHODIMP CINet::MyContinue(PROTOCOLDATA *pStateInfoIn)
{
    PerfDbgLog(tagCINet, this, "+CINet::Continue");
    HRESULT hr = NOERROR;

    PProtAssert((pStateInfoIn->pData && !pStateInfoIn->cbData));

    // check if the inete state changed
    g_cInetState.HandleState();

    OnINetInternal((DWORD_PTR) pStateInfoIn->pData);

    if( !(_grfBindF & BINDF_FROMURLMON) )
    {
        //
        // if the BindInfo is created from heap by CINet
        // we need to delete it from here
        //
        delete pStateInfoIn;
    }

    PerfDbgLog1(tagCINet, this, "-CINet::Continue (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::Abort
//
//  Synopsis:
//
//  Arguments:  [hrReason] --
//              [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Abort(HRESULT hrReason, DWORD dwOptions)
{
    HRESULT hr = NOERROR;
    if( _pEmbdFilter )
    {
        hr = _pEmbdFilter->Abort(hrReason, dwOptions);
    }
    return hr;
}

STDMETHODIMP CINet::MyAbort(HRESULT hrReason, DWORD dwOptions)
{
    PerfDbgLog(tagCINet, this, "+CINet::Abort");
    HRESULT hr = NOERROR;

    PProtAssert((_pCTrans));

    hr = ReportResultAndStop(hrReason);

    PerfDbgLog1(tagCINet, this, "-CINet::Abort (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Terminate
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Terminate(DWORD dwOptions)
{
    // in case we failed on :Start, _pEmbdFilter is 
    // not in place, however, Terminate might be
    // called so we have to call MyTerminate directly 
    HRESULT hr = NOERROR;
    if( _pEmbdFilter )
    {
        hr = _pEmbdFilter->Terminate(dwOptions);
    }
    else
    {
        hr = MyTerminate(dwOptions);
    }
    return hr;
}

STDMETHODIMP CINet::MyTerminate(DWORD dwOptions)
{
    PerfDbgLog(tagCINet, this, "+CINet::Terminate");
    HRESULT hr = NOERROR;

    //should be called once
    PProtAssert((_pCTrans));

    if (_pOIBindInfo)
    {
        _pOIBindInfo->Release();
        _pOIBindInfo = NULL;
    }

    if (_pServiceProvider)
    {
        _pServiceProvider->Release();
        _pServiceProvider = NULL;
    }

    if (_pCTrans)
    {
        // release all the handles if unless
        // the request was locked
        if (!_fLocked && !_hLockHandle)
        {
            TerminateRequest();
        }

        // sync block
        {
            CLock lck(_mxs);    // only one thread should be in here

            if ((_dwState != INetState_DONE) && (_dwState != INetState_ERROR))
            {
                _dwState = INetState_DONE;
            }

            _pCTrans->Release();
            _pCTrans = NULL;
        }

    }

#if DBG == 1
    if ( _BndInfo.stgmedData.tymed != TYMED_NULL )
        PerfDbgLog1(tagCINet, this, "--- CINet::Stop ReleaseStgMedium (%lx)", _BndInfo.stgmedData);
#endif

    ReleaseBindInfo(&_BndInfo);


    PerfDbgLog1(tagCINet, this, "-CINet::Terminate (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Suspend
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Suspend()
{
    return _pEmbdFilter->Suspend();
}

STDMETHODIMP CINet::MySuspend()
{
    PerfDbgLog(tagCINet, this, "+CINet::Suspend");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagCINet, this, "-CINet::Suspend (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Resume
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Resume()
{
    return _pEmbdFilter->Resume();    
}

STDMETHODIMP CINet::MyResume()
{
    PerfDbgLog(tagCINet, this, "+CINet::Resume");

    HRESULT hr = E_NOTIMPL;

    PerfDbgLog1(tagCINet, this, "-CINet::Resume (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::SetPriority
//
//  Synopsis:
//
//  Arguments:  [nPriority] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::SetPriority(LONG nPriority)
{
    PerfDbgLog1(tagCINet, this, "+CINet::SetPriority (%ld)", nPriority);

    HRESULT hr = S_OK;

    _nPriority = nPriority;

    PerfDbgLog1(tagCINet, this, "-CINet::SetPriority (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetPriority
//
//  Synopsis:
//
//  Arguments:  [pnPriority] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::GetPriority(LONG * pnPriority)
{
    PerfDbgLog(tagCINet, this, "+CINet::GetPriority");

    HRESULT hr = S_OK;

    *pnPriority = _nPriority;

    PerfDbgLog1(tagCINet, this, "-CINet::GetPriority (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Read
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//              [ULONG] --
//              [pcbRead] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
    return  _pEmbdFilter->Read(pv, cb, pcbRead);
}

STDMETHODIMP CINet::MyRead(void *pv,ULONG cb,ULONG *pcbRead)
{
    PerfDbgLog(tagCINet, this, "+CINet::Read");
    HRESULT hr = NOERROR;

    if(GetBindFlags() & BINDF_DIRECT_READ)
    {
        hr = ReadDirect((BYTE *)pv, cb, pcbRead);
    }
    else
    {
        hr = ReadDataHere((BYTE *)pv, cb, pcbRead);
    }
    
    PerfDbgLog1(tagCINet, this, "-CINet::Read (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::Seek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    return _pEmbdFilter->Seek(dlibMove, dwOrigin, plibNewPosition);
}

STDMETHODIMP CINet::MySeek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagCINet, this, "+CINet::Seek");
    HRESULT hr;

    hr = INetSeek(dlibMove, dwOrigin,plibNewPosition);

    PerfDbgLog1(tagCINet, this, "-CINet::Seek (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::LockRequest
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::LockRequest(DWORD dwOptions)
{
    return _pEmbdFilter->LockRequest(dwOptions);
}

STDMETHODIMP CINet::MyLockRequest(DWORD dwOptions)
{
    PerfDbgLog(tagCINet, this, "+CINet::LockRequest");
    HRESULT hr = NOERROR;

    hr = LockFile(FALSE);

    PerfDbgLog1(tagCINet, this, "-CINet::LockRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::UnlockRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::UnlockRequest()
{
    return _pEmbdFilter->UnlockRequest();
}

STDMETHODIMP CINet::MyUnlockRequest()
{
    PerfDbgLog(tagCINet, this, "+CINet::UnlockRequest");
    HRESULT hr = NOERROR;

    TerminateRequest();
    hr = UnlockFile();

    PerfDbgLog1(tagCINet, this, "-CINet::UnlockRequest (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryOption
//
//  Synopsis:
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//
//  Returns:
//
//  History:    4-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::QueryOption(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf)
{
    PerfDbgLog1(tagCINet, this, "+CINet::QueryOptions dwOption:%ld", dwOption);
    HRESULT hr;

    if (!pcbBuf || (*pcbBuf == 0))
    {
        *pcbBuf = 0;
        hr = E_INVALIDARG;
    }
    else if (!_hRequest)
    {
        *pcbBuf = 0;
        hr = E_FAIL;
    }
    else if (dwOption == WININETINFO_OPTION_LOCK_HANDLE)
    {
        if (    *pcbBuf >= sizeof(HANDLE)
            &&  InternetLockRequestFile(_hRequest, (HANDLE *)pBuffer))
        {
            *pcbBuf = sizeof(HANDLE);
            hr = S_OK;
        }
        else
        {
            *pcbBuf = 0;
            hr = E_FAIL;
        }
    }
    else
    {
        if (InternetQueryOption(_hRequest, dwOption, pBuffer, pcbBuf))
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }

    PerfDbgLog2(tagCINet, this, "-CINet::QueryOptions (hr:%lx,szStr:%s)", hr, pBuffer);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryInfo
//
//  Synopsis:
//
//  Arguments:  [dwOption] --
//              [pBuffer] --
//              [pcbBuf] --
//              [pdwFlag] --
//
//  Returns:
//
//  History:    4-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::QueryInfo(DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf, DWORD *pdwFlags, DWORD *pdwReserved)
{
    PerfDbgLog1(tagCINet, this, "+CINet::QueryInfo dwOption:%ld", dwOption);
    HRESULT hr;

    if (!pcbBuf)
    {
        *pcbBuf = 0;
        hr = E_INVALIDARG;
    }
    else if (!_hRequest)
    {
        *pcbBuf = 0;
        hr = E_FAIL;
    }
    else
    {
        if (HttpQueryInfo(_hRequest, dwOption, pBuffer, pcbBuf, pdwFlags))
        {
            hr = S_OK;
        }
        else
        {
            if (pBuffer  && (*pcbBuf >= sizeof(HRESULT)) )
            {
                HRESULT *phr = (HRESULT *)pBuffer;
                *phr = HRESULT_FROM_WIN32(GetLastError());
                *pcbBuf = sizeof(HRESULT);
            }
            hr = S_FALSE;
        }
    }

    PerfDbgLog2(tagCINet, this, "-CINet::QueryInfo (hr:%lx,szStr:%s)", hr, XDBG(pBuffer&&!hr?pBuffer:"",""));
    return hr;
}

STDMETHODIMP CINet::Prepare()
{
    PerfDbgLog(tagCINet, this, "+CINet::Prepare");
    HRESULT hr = NOERROR;

    PerfDbgLog1(tagCINet, this, "-CINet::Prepare (hr:%lx)", hr);
    return hr;
}

STDMETHODIMP CINet::Continue()
{
    PerfDbgLog(tagCINet, this, "+CINet::Continue");
    HRESULT hr = NOERROR;

    _dwThreadID = GetCurrentThreadId();

    PerfDbgLog1(tagCINet, this, "-CINet::Continue (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::CINet
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINet::CINet(REFCLSID rclsid, IUnknown *pUnkOuter) : _CRefs(), _CRefsHandles(0), _cReadCount(0), _pclsidProtocol(rclsid), _Unknown()
{
    PerfDbgLog(tagCINet, this, "+CINet::CINet");
    _dwState = INetState_START;
    _dwIsA = DLD_PROTOCOL_NONE;
    _fRedirected = FALSE;
    _fP2PRedirected = FALSE;
    _fLocked = FALSE;
    _fFilenameReported = FALSE;
    _fSendAgain = FALSE;
    _fSendRequestAgain = FALSE;
    _hLockHandle = NULL;
    _hFile = NULL;
    _dwThreadID = GetCurrentThreadId();
    _fDone = 0;
    _hwndAuth = NULL;
    _bscf = BSCF_FIRSTDATANOTIFICATION;
    _pOIBindInfo = 0;
    _pszUserAgentStr = 0;
    _nPriority = THREAD_PRIORITY_NORMAL;
    _cbSizeLastReportData = 0;
    _fForceSwitch = FALSE;
    _cbAuthenticate = 0;
    _cbProxyAuthenticate = 0;

    _fDoSimpleRetry = FALSE;
    
    if (!pUnkOuter)
    {
        pUnkOuter = &_Unknown;
    }
    _pUnkOuter = pUnkOuter;
    _pWindow = 0;      

    _pEmbdFilter = NULL;

    _hServer = 0;
    _hRequest = 0; 


    PerfDbgLog(tagCINet, this, "-CINet::CINet");
}

CINet::~CINet()
{
    delete _pwzUrl;
    delete _pszUserAgentStr;

    // release Embedded Filter
    if( _pEmbdFilter )
    {
        delete _pEmbdFilter;
        _pEmbdFilter = NULL;
    }

    PerfDbgLog(tagCINet, this, "CINet::~CINet");
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::ReportResultAndStop
//
//  Synopsis:   Post the termination package
//
//  Arguments:  [hr] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::ReportResultAndStop(HRESULT hr, ULONG ulProgress,ULONG ulProgressMax, LPWSTR pwzStr)
{
    PerfDbgLog1(tagCINet, this, "+CINet::ReportResultAndStop (hr:%lx)", hr);

    HRESULT hrOut = NOERROR;
    BOOL fReportResult = FALSE;
    BOOL fReportData = FALSE;
    
    
    {
        CLock lck(_mxs);    // only one thread should be in here

        // set the state to error and report error
        // must go in queue since other messages might be ahead
        if ((_dwState != INetState_DONE) && (_dwState != INetState_ERROR))
        {
            _hrINet = hr;
            _dwState = (hr != NOERROR) ? INetState_ERROR : INetState_DONE;
            if (_dwState == INetState_DONE)
            {
                if (ulProgress == 0)
                {
                    ulProgress = _cbTotalBytesRead;
                    ulProgressMax = _cbDataSize;
                }
                if ( ( (ulProgress != _cbSizeLastReportData ) || 
                       (!ulProgress && !ulProgressMax)           ) && 
                     ( _grfBindF & BINDF_FROMURLMON ) )          
                {
                    //
                    // last notification
                    // NOTE: we need to report data for empty page
                    //       we might have report this data already, 
                    //       so check for the _cbSizeLastReportData
                    //       if they are same, do not report data 
                    //       again. (this might give hard time for
                    //       IEFeatuer handler )
                    //
                    _bscf |= BSCF_LASTDATANOTIFICATION;
                    fReportData = TRUE;
                }

                //
                // HACK: for prodegy's ocx 
                // if we have not send out ReportData(BSCF_LAST...) and
                // we are doing BindToStorage but the data has been
                // reported, we have to report it again with LAST flag
                // tuned on
                //
                if( !fReportData &&
                    !(_bscf & BSCF_LASTDATANOTIFICATION ) && 
                    !(_BndInfo.dwOptions & BINDINFO_OPTIONS_BINDTOOBJECT) )
                {
                    // need to send out last notifiation for whatever
                    // client depend on it...
                    _bscf |= BSCF_LASTDATANOTIFICATION; 
                    fReportData = TRUE;
                }
            }
            else if (_dwState == INetState_ERROR)
            {
                SetINetState(INetState_ERROR);
            }
            
            PProtAssert((_pCTrans));
            fReportResult = TRUE;
        }
    }
    
    if (_pCTrans)
    {
        if (fReportData)
        {
            _cbSizeLastReportData = ulProgress;
            _pEmbdFilter->ReportData(_bscf, ulProgress, ulProgressMax);
        }
        // teminate might have occured on ReportData
        if (fReportResult && _pCTrans)
        {
            _pEmbdFilter->ReportResult(hr,_dwResult,pwzStr);
        }

        if( !fReportResult )
        {
            hrOut = INET_E_RESULT_DISPATCHED;
        }
    }

    PerfDbgLog1(tagCINet, this, "-CINet::ReportResultAndStop (hrOut:%lx)", hrOut);
    return hrOut;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::ReportNotification
//
//  Synopsis:
//
//  Arguments:  [NMsg] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::ReportNotification(BINDSTATUS NMsg, LPSTR szStr)
{
    PerfDbgLog1(tagCINet, this, "+CINet::ReportNotification (NMsg:%lx)", NMsg);
    HRESULT hr = E_FAIL;

    BOOL fReport = FALSE;
    LPWSTR pwzStr = 0;

    { // ssync block begin
        CLock lck(_mxs);    // only one thread should be in here

        if ((_dwState != INetState_DONE) && (_dwState != INetState_ERROR))
        {
            if (szStr)
            {
                pwzStr = DupA2W(szStr);
            }
            if (szStr && !pwzStr)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                fReport = TRUE;
            }
        }
    } // sync block end
    
    if (fReport)
    {
        if ( _pCTrans && _pEmbdFilter ) 
        {
            hr = _pEmbdFilter->ReportProgress((ULONG)NMsg, pwzStr);
        }
    }
    
    delete pwzStr;

    PerfDbgLog(tagCINet, this, "-CINet::ReportNotification");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::SetPending
//
//  Synopsis:
//
//  Arguments:  [hrNew] --
//
//  Returns:
//
//  History:    3-28-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::SetStatePending(HRESULT hrNew)
{
    CLock lck(_mxs);    // only one thread should be in here
    HRESULT hr;

    PerfDbgLog2(tagCINet, this, "CINet::SetStatePending (hrOld:%lx, hrNew:%lx)", _hrPending, hrNew);

    //BUGBUG: turn this assertion on again
    PProtAssert(( (   ((_hrPending != E_PENDING) && (hrNew == E_PENDING))
                   || ((_hrPending == E_PENDING) && (hrNew != E_PENDING))) ));

    hr = _hrPending;
    _hrPending = hrNew;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetStatePending
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-08-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::GetStatePending()
{
    CLock lck(_mxs);    // only one thread should be in here
    //PerfDbgLog1(tagCINet, this, "CINet::GetStatePending (hr:%lx)", _hrPending);
    return _hrPending;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::SetByteCountReadyToRead
//
//  Synopsis:
//
//  Arguments:  [cbReadyReadNow] --
//
//  Returns:
//
//  History:    6-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CINet::SetByteCountReadyToRead(LONG cbReadyReadNow)
{
    CLock lck(_mxs);    // only one thread should be in here
    PerfDbgLog3(tagCINet, this, "CINet::SetByteCountReadyToRead (cbReadReturn:%ld, cbReadyRead:%ld, cbReadyLeft:%ld)",
                    _cbReadyToRead, cbReadyReadNow, _cbReadyToRead + cbReadyReadNow);
    _cbReadyToRead += cbReadyReadNow ;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetByteCountReadyToRead
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    6-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG CINet::GetByteCountReadyToRead()
{
    CLock lck(_mxs);    // only one thread should be in here
    PerfDbgLog1(tagCINet, this, "CINet::GetByteCountReadyToRead (_cbReadyToRead:%ld)", _cbReadyToRead);
    return _cbReadyToRead;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::SetINetState
//
//  Synopsis:
//
//  Arguments:  [inState] --
//
//  Returns:
//
//  History:    2-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INetState CINet::SetINetState(INetState inState)
{
    CLock lck(_mxs);    // only one thread should be in here
    PerfDbgLog1(tagCINet, this, "+CINet::SetINetState (State:%lx)", inState);

    INetState in = _INState;
    _INState = inState;

    PerfDbgLog1(tagCINet, this, "-CINet::SetINetState (hr:%lx)", in);
    return in;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetINetState
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-25-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INetState CINet::GetINetState()
{
    CLock lck(_mxs);    // only one thread should be in here
    PerfDbgLog(tagCINet, this, "+CINet::GetINetState");

    INetState in = _INState;

    PerfDbgLog1(tagCINet, this, "-CINet::GetINetState (hr:%lx)", in);
    return in;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetAsyncStart
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetAsyncStart()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetAsyncStart");
    HRESULT hr = NOERROR;
    BOOL fAsyncStart = FALSE;

    // guard the object
    PrivAddRef();

    if (fAsyncStart)
    {
        // post notification for next step
        SetINetState(INetState_START);
        TransitState(INetState_START);
    }
    else
    {
        hr = INetAsyncOpen();
    }

    if (_hrError != INET_E_OK)
    {
        if (hr != S_OK && hr != E_PENDING && hr != INET_E_DONE)
        {
            PProtAssert((  (hr >= INET_E_ERROR_FIRST && hr <= INET_E_ERROR_LAST)
                         ||(hr == E_OUTOFMEMORY)
                         ||(hr == E_ABORT)
                         ));

            ReportResultAndStop(hr);
        }
        else
        {
            // he will do inetdone notifications
            ReportResultAndStop(NOERROR);
        }
    }

    PrivRelease();

    PerfDbgLog1(tagCINet, this, "-CINet::INetAsyncStart (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetStart
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetStart()
{
    HRESULT hr;
    PerfDbgLog(tagCINet, this, "+CINet::OnINetStart");

    // nothing to do - just call
    hr = INetAsyncOpen();

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetStart (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetAsyncOpen
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetAsyncOpen()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetAsyncOpen");
    PProtAssert((GetStatePending() == NOERROR));
    DWORD dwFlags = INTERNET_FLAG_ASYNC;
    DWORD dwBindF = 0;

    HRESULT hr = NOERROR;

    if (g_hSession == NULL)
    {
        // Only 1 thread should be in here, this is to protect
        // two global variables g_hSession and g_pszUserAgentString
        {
            CLock lck(g_mxsSession); 
            if( g_hSession == NULL )
            {
                SetINetState(INetState_OPEN_REQUEST);

                PerfDbgLog1(tagCINet, this, "___ INetAysncOpen calling InternetOpen %ld", GetTickCount());
                SetStatePending(E_PENDING);

                g_hSession = InternetOpen(
                    GetUserAgentString()
                    , INTERNET_OPEN_TYPE_PRECONFIG
                    , NULL
                    , NULL
                    , dwFlags);
                PerfDbgLog1(tagCINet, this, "___ INetAysncOpen done InternetOpen %ld", GetTickCount());

                if (g_hSession == NULL)
                {
                    dwLstError = GetLastError();
                    if (dwLstError == ERROR_IO_PENDING)
                    {
                        hr = E_PENDING;
                    }
                    else
                    {
                        SetStatePending(NOERROR);
                        hr = _hrError = INET_E_NO_SESSION;
                        SetBindResult(dwLstError, hr);
                    }
                }
                else
                {
                    if (g_pszUserAgentString)
                    {
                        // Open was successful, so we don't need the replacement
                        // User Agent string anymore.

                        delete g_pszUserAgentString;
                        g_pszUserAgentString = NULL;
                    }

                    SetStatePending(NOERROR);
                    InternetSetStatusCallback(g_hSession, CINetCallback);
                    hr = INetAsyncConnect();
                }
            }
            else
            {
                hr = INetAsyncConnect();
            }
        } // single access block
    }
    else
    {
        hr = INetAsyncConnect();
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetAsyncOpen (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetAsyncOpen
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetAsyncOpen(DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetAsyncOpen");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);

    if (dwResult)
    {
        // set the handle and the callback
        g_hSession = (HINTERNET) dwResult;
        InternetSetStatusCallback(g_hSession, CINetCallback);
    }
    // notification for next request
    TransitState(INetState_OPEN_REQUEST);

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetAsyncOpen (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetAsyncConnect
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetAsyncConnect()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetAsyncConnect");
    PProtAssert((GetStatePending() == NOERROR));

    HRESULT hr = NOERROR;
    BOOL fRestarted;
    DWORD dwBindF = 0;
    DWORD dwService = INTERNET_SERVICE_HTTP;

    // get the open flags

    dwBindF = GetBindFlags();

    if (dwBindF & BINDF_GETNEWESTVERSION)
    {
        _dwOpenFlags |= INTERNET_FLAG_RELOAD;
        PerfDbgLog1(tagCINet, this, "***CINet::****RELOAD***** %lx", dwBindF);
        
    }
    else
    {
        PerfDbgLog1(tagCINet, this, "---CINet::----NO-RELOAD %lx --", dwBindF);
    }

    if (dwBindF & BINDF_NOWRITECACHE)
    {
        _dwOpenFlags |= INTERNET_FLAG_DONT_CACHE ;
    }

    if (dwBindF & BINDF_NEEDFILE)
    {
        PerfDbgLog(tagCINet, this, "CINet::INetAsyncConnect: turn on: INTERNET_FLAG_NEED_FILE");
        _dwOpenFlags |= INTERNET_FLAG_NEED_FILE;
    }

    if (dwBindF & (BINDF_NO_UI | BINDF_SILENTOPERATION))
    {
        _dwOpenFlags |= INTERNET_FLAG_NO_UI;
    }

    // BUGBUG OFFLINE, RELOAD, RESYNCHRONIZE and HYPERLINK are mutually
    // exclusive. But inside wininet there is priority, so
    // the priority is OFFLINE, RELOAD, RESYNCHRONIZE, HYPERLINK in that order

    if (dwBindF & BINDF_RESYNCHRONIZE)
    {
        // caller asking to do if-modified-since

        _dwOpenFlags |= INTERNET_FLAG_RESYNCHRONIZE;
    }

    if (dwBindF & BINDF_HYPERLINK)
    {
        // caller says this is a hyperlink access

        _dwOpenFlags |= INTERNET_FLAG_HYPERLINK;
    }

    if (dwBindF & BINDF_FORMS_SUBMIT)
    {
        // caller says this is a forms submit.
        _dwOpenFlags |= INTERNET_FLAG_FORMS_SUBMIT;
    }

    if (dwBindF & BINDF_OFFLINEOPERATION )
    {
        _dwOpenFlags |= INTERNET_FLAG_OFFLINE;
    }

    // connect flags
    if (dwBindF & BINDF_OFFLINEOPERATION )
    {
        _dwConnectFlags |= INTERNET_FLAG_OFFLINE;
    }

    if (dwBindF & BINDF_PRAGMA_NO_CACHE )
    {
        _dwOpenFlags |= INTERNET_FLAG_PRAGMA_NOCACHE;
    }

    if( dwBindF & BINDF_GETFROMCACHE_IF_NET_FAIL)
    {
        _dwOpenFlags |= INTERNET_FLAG_CACHE_IF_NET_FAIL;
    }

    if( dwBindF & BINDF_FWD_BACK )
    {
        _dwOpenFlags |= INTERNET_FLAG_FWD_BACK;
    }

    // additional wininet flags are passed with bindinfo
    if( _BndInfo.dwOptions == BINDINFO_OPTIONS_WININETFLAG )
    {
        _dwOpenFlags |= _BndInfo.dwOptionsFlags;
    }


    SetINetState(INetState_CONNECT_REQUEST);

    PrivAddRef(TRUE);

    _HandleStateServer = HandleState_Pending;
    SetStatePending(E_PENDING);

    HINTERNET hServerTmp = InternetConnect(
            g_hSession,                             // hInternetSession
            GetServerName(),                        // lpszServerName
            _ipPort,                                // nServerPort
            (_pszUserName[0])?_pszUserName:NULL,    // lpszUserName
            (_pszPassword[0])?_pszPassword:NULL,    // lpszPassword
            dwService,                              // INTERNET_SERVICE_HTTP 
            _dwConnectFlags,                        // dwFlags
            (DWORD_PTR) this
    );
    //
    // Note: do not remove this state setting here!
    //       there is a timing window - needs to
    //       be fixed in wininet/urlmon!!!
    SetINetState(INetState_CONNECT_REQUEST);

    if ( hServerTmp == 0)
    {
        dwLstError = GetLastError();
        if (dwLstError == ERROR_IO_PENDING)
        {
            // wait async for the handle
            hr = E_PENDING;
        }
        else
        {
            PrivRelease(TRUE);
            SetStatePending(NOERROR);
            hr = _hrError = INET_E_CANNOT_CONNECT;
            SetBindResult(dwLstError, hr);
        }
    }
    else
    {
        _hServer = hServerTmp;
        SetStatePending(NOERROR);
        // wininet holds on to CINet - Release called on the callback on close handle
        _HandleStateServer = HandleState_Initialized;
        PerfDbgLog1(tagCINet, this, "=== CINet::INetAsyncConnect (hServer:%lx)", _hServer);
        PProtAssert((_hServer));
        hr = INetAsyncOpenRequest();
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetAsyncConnect (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetConnect
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetConnect(DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetConnect");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);
    TransAssert((_hServer == 0));

    if (dwResult)
    {
        CLock lck(_mxs);    // only one thread should be in here
        if (_HandleStateServer == HandleState_Pending)
        {
            TransAssert((_hServer == 0));
            // set the server handle
            _HandleStateServer = HandleState_Initialized;
            _hServer = (HANDLE)dwResult;
        }

        // wininet holds on to CINet - Release called on the callback on close handle
        PerfDbgLog1(tagCINet, this, "=== CINet::OnINetConnect (hServer:%lx)", _hServer);
    }

    if (_hServer)
    {
        TransitState(INetState_CONNECT_REQUEST);
    }
    else
    {
        PProtAssert((_HandleStateServer == HandleState_Aborted));
        PrivRelease(TRUE);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetConnect (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetAsyncOpenRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetAsyncOpenRequest()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetAsyncOpenRequest");

    HRESULT hr = E_FAIL;

    PerfDbgLog1(tagCINet, this, "-CINet::INetAsyncOpenRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetOpenRequest
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetOpenRequest(DWORD dwResult)
{
    PerfDbgLog1(tagCINet, this, "+CINet::OnINetOpenRequest (dwResult:%lx)", dwResult);
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);
    TransAssert((_hRequest == 0));

    if (dwResult)
    {
        CLock lck(_mxs);    // only one thread should be in here

        if (_HandleStateRequest == HandleState_Pending)
        {
            // set the request handle
            _HandleStateRequest = HandleState_Initialized;
            _hRequest = (HANDLE)dwResult;
            PProtAssert((_hServer != _hRequest));
        }
    }

    if (_hRequest)
    {
        hr = INetAsyncSendRequest();
    }
    else
    {
        PProtAssert((_HandleStateRequest == HandleState_Aborted));
        PrivRelease(TRUE);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetOpenRequest (hr:%lx)", hr);
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetAsyncSendRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetAsyncSendRequest()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetAsyncSendRequest");

    HRESULT hr = E_FAIL;

    PerfDbgLog1(tagCINet, this, "-CINet::INetAsyncSendRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetSendRequest
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetSendRequest( DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetSendRequest");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);

    _dwSendRequestResult = dwResult;
    _lpvExtraSendRequestResult = NULL;

    if( dwResult == ERROR_INTERNET_FORCE_RETRY )
    {
        PerfDbgLog(tagCINet, this, "  --dwResult = FORCE_RETRY! ");
        _fSendRequestAgain = TRUE;
    }

    if (OperationOnAparmentThread(INetState_SEND_REQUEST))
    {
        // query for content-encoding header, if we find one, 
        // we will have to force TransitState to do thread switching
        // since the compression filter can not be loaded on worker 
        // thread
        char szEncType[SZMIMESIZE_MAX] = "";
        DWORD cbLen = sizeof(szEncType); 
        if( _hRequest && 
            HttpQueryInfo(_hRequest, HTTP_QUERY_CONTENT_ENCODING, 
                          szEncType, &cbLen, NULL) )
        {
            if( cbLen && szEncType[0] )
                _fForceSwitch = TRUE;
        }

        // if sec problem shows up on UI thread, we need to 
        // force switch
        switch(_dwSendRequestResult)
        {
            case ERROR_INTERNET_SEC_CERT_DATE_INVALID     :
            case ERROR_INTERNET_SEC_CERT_CN_INVALID       :
            case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR    :
            case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR    :
            case ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR   :
            case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION   :
            case ERROR_INTERNET_INVALID_CA                :
            case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED   :
            case ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED     :
            case ERROR_INTERNET_FORCE_RETRY               : 
            case ERROR_INTERNET_SEC_CERT_ERRORS           :
            case ERROR_INTERNET_SEC_CERT_REV_FAILED       :
            case ERROR_INTERNET_SEC_CERT_REVOKED          :
                _fForceSwitch = TRUE;
        }


        TransitState(INetState_SEND_REQUEST);
    }
    else if (!IsUpLoad())
    {
        hr = INetQueryInfo();
    }
    else 
    {
        if( _fSendRequestAgain )
        {
            _fCompleted = FALSE;
            _fSendAgain = TRUE;
            _fSendRequestAgain = FALSE;
            hr = INetAsyncSendRequest();
        }
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetSendRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetSuspendSendRequest
//
//  Synopsis:   called on a wininet callback to indicate the suspentition 
//               of request processing until UI is displayed to the user.
//
//  Arguments:  [dwResult] -- error code to generate dialog for
//              [lpvExtraResult] -- extra void pointer used pass dialog specific data
//
//  Returns:
//
//  History:    5-24-98   ArthurBi (Arthur Bierer)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetSuspendSendRequest(DWORD dwResult, LPVOID lpvExtraResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetSuspendSendRequest");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));

    // set state to normal - no pending transaction
    SetStatePending(NOERROR);

    _dwSendRequestResult = dwResult;
    _lpvExtraSendRequestResult = lpvExtraResult;

    //if (OperationOnAparmentThread(INetState_SEND_REQUEST))

    // even though we're not doing auth, we need to do UI
    _hrINet = INET_E_AUTHENTICATION_REQUIRED;
    TransitState(INetState_DISPLAY_UI, TRUE);

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetSuspendSendRequest (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetStateChange
//
//  Synopsis:   called on the apartment thread
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-22-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetStateChange()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetStateChange");

    HRESULT hr = NOERROR;

    g_cInetState.HandleState();

    PerfDbgLog1(tagCINet, this, "-CINet::INetStateChange (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetStateChange
//
//  Synopsis:   called on the wininet worker thread whenever the
//              wininet state changes
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    1-22-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetStateChange( DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetStateChange");
    HRESULT hr = NOERROR;

    // set the new state and ping the apartment thread
    g_cInetState.SetState(dwResult);

    TransitState(INetState_INETSTATE_CHANGE);

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetStateChange (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetQueryInfo
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetQueryInfo()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetQueryInfo");

    HRESULT hr = NOERROR;

    // Here we check if we need to do redirection, or
    // whether authentication is needed etc.
    if (!IsUpLoad())
    {
        hr = QueryInfoOnResponse();
    }
    if (hr == NOERROR)
    {
        // read more data from wininet
        hr = INetRead();
    }
    else if (hr == S_FALSE)
    {
        // S_FALSE means successful redirecting occured
        hr = NOERROR;
    }

    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop(hr);
    }
    PerfDbgLog1(tagCINet, this, "-CINet::INetQueryInfo (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetRead
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetRead(DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetRead");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);

    if (OperationOnAparmentThread(INetState_SEND_REQUEST))
    {
        TransitState(INetState_READ);
    }
    else
    {
        hr = INetRead();
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetRead (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetRead
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetRead()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetRead");
    HRESULT hr = NOERROR;

    if (IsUpLoad())
    {
        hr = INetWrite();
    }
    else if(GetBindFlags() & BINDF_DIRECT_READ)
    {
        hr = INetReadDirect();
    }
    else
    {
        // this is the no-copy case
        // read data to users buffer
        hr = INetDataAvailable();
    }

    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop((_hrError == INET_E_DONE) ? NOERROR : _hrError);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetRead (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetwrite
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetWrite()
{
    HRESULT hr = E_FAIL;

    TransAssert((FALSE && "HAS TO BE OVERWRITTEN"));

    return hr;
}

#define HRESULT_FROM_WININET(pv)     HRESULT_FROM_WIN32( (((LPINTERNET_ASYNC_RESULT) (pv) )->dwError) )

//+---------------------------------------------------------------------------
//
//  Method:     CINet::CINetCallback
//
//  Synopsis:
//
//  Arguments:  [hInternet] --
//              [dwContext] --
//              [dwStatus] --
//              [pvInfo] --
//              [dwStatusLen] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID CALLBACK CINet::CINetCallback(IN HINTERNET hInternet, IN DWORD_PTR dwContext,
                           IN DWORD dwStatus, IN LPVOID pvInfo, IN DWORD dwStatusLen)
{
    // If this is a request, then we know the cookie type
    CINet *pCINet = (CINet *) dwContext;
    HRESULT hrError = INET_E_OK;
    
    //
    // handle callback without a context
    if (!dwContext)
    {
        switch (dwStatus)
        {
        default:
            //PProtAssert((FALSE));
        break;

        case INTERNET_STATUS_STATE_CHANGE :
        {
            DWORD dwState = *(DWORD *) pvInfo;

            g_cInetState.SetState(dwState);
        }
        break;
        }   // end switch
    }
    else
    {
        PerfDbgLog2(tagCINet, pCINet, "+CINet::CINetCallback Status:%ld, State:%ld",
            dwStatus, pCINet->_INState);

        DWORD dwAsyncResult;

        // from here the original thread needs to be told of various things
        // such as errors, operation done etc.
        PProtAssert((pCINet));

        // guard this call - request might be aborted
        //pCINet->AddRef();
        
        DWORD dwFault;
#ifdef INET_CALLBACK_EXCEPTION
        _try
#endif // INET_CALLBACK_EXCEPTION
        {

            switch (dwStatus)
            {
            // the net connection state changed
            case INTERNET_STATUS_STATE_CHANGE :
            {
                DWORD dwState = *(DWORD *) pvInfo;
                pCINet->OnINetStateChange(dwState);
            }
            break;

            // callback to put up UI
            case INTERNET_STATUS_USER_INPUT_REQUIRED:
            {
                // guard this call - request might be aborted
                pCINet->PrivAddRef();

                PProtAssert(pCINet->_INState == INetState_SEND_REQUEST);
                //PProtAssert(!((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult);

                LPVOID lpvSendRequestResultData = (LPVOID) ((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult;
                DWORD dwSendRequestResult = ((LPINTERNET_ASYNC_RESULT) (pvInfo) )->dwError;

                // handle the error here in particular pass on info for zone crossing
                pCINet->OnINetSuspendSendRequest(dwSendRequestResult, lpvSendRequestResultData);

                // unguard - release
                pCINet->PrivRelease();
            }
            break;

            // request completed
            case INTERNET_STATUS_REQUEST_COMPLETE:
            {
                // guard this call - request might be aborted
                pCINet->PrivAddRef();
                if (pCINet->_INState != INetState_ERROR)
                {
                    PProtAssert((pCINet->GetStatePending() == E_PENDING));
                }

                switch (pCINet->_INState)
                {
                case INetState_OPEN_REQUEST:
                    // the internet session handle is supposed to be returned
                    dwAsyncResult = ((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult;
                    if (dwAsyncResult)
                    {
                        // got the internet session handle back
                        pCINet->OnINetAsyncOpen(dwAsyncResult);
                    }
                    else
                    {
                        hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError, INET_E_NO_SESSION);
                    }
                break;

                case INetState_CONNECT_REQUEST:
                    // the server handle is supposed to be returned
                    dwAsyncResult = ((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult;
                    if (dwAsyncResult)
                    {
                        pCINet->OnINetConnect(dwAsyncResult);
                    }
                    else
                    {
                        hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError,INET_E_CANNOT_CONNECT);
                    }
                break;

                case INetState_PROTOPEN_REQUEST:
                    // the request handle is suppost to be returned
                    dwAsyncResult = ((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult;
                    if (dwAsyncResult)
                    {
                        pCINet->OnINetOpenRequest(dwAsyncResult);
                    }
                    else
                    {
                        hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError,INET_E_OBJECT_NOT_FOUND);
                    }
                break;

                case INetState_SEND_REQUEST:
                {
                    // SendRequest returns a BOOL
                    dwAsyncResult = ((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult;
                    if (dwAsyncResult)
                    {
                        // pass on 0 and look up the status code with HttpQueryInfo
                        pCINet->OnINetSendRequest(0);
                    }
                    else
                    {
                        DWORD dwSendRequestResult = ((LPINTERNET_ASYNC_RESULT) (pvInfo) )->dwError;
                        // handle the error here in particular pass on info for zone crossing
                        if (dwSendRequestResult)
                        {
                            // handle the sendrequest result
                            // zone crossing
                            switch (dwSendRequestResult)
                            {
                            case ERROR_INTERNET_SEC_CERT_DATE_INVALID     :
                            case ERROR_INTERNET_SEC_CERT_CN_INVALID       :
                            case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR    :
                            case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR    :
                            case ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR   :
                            case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION   :
                            case ERROR_INTERNET_INVALID_CA                :
                            case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED   :
                            case ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED     :
                            case ERROR_INTERNET_FORCE_RETRY               : 
                            case ERROR_INTERNET_SEC_CERT_ERRORS           :
                            case ERROR_INTERNET_SEC_CERT_REV_FAILED       :
                            case ERROR_INTERNET_SEC_CERT_REVOKED          :
                            case ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY:

                                pCINet->OnINetSendRequest(dwSendRequestResult);
                            break;
                            default:
                                hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError);
                            break;
                            }
                        }
                        else
                        {
                            hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError);
                        }
                    }
                }
                break;

                case INetState_READ:
                    // InternetRead returns TRUE of FALSE
                    dwAsyncResult = ((LPINTERNET_ASYNC_RESULT)pvInfo)->dwResult;
                    if (dwAsyncResult)
                    {
                        pCINet->OnINetRead(dwAsyncResult);
                    }
                    else
                    {
                        hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError);
                    }
                break;

                case INetState_DATA_AVAILABLE:
                {
                    DWORD dwResult = ((LPINTERNET_ASYNC_RESULT)(pvInfo))->dwResult;

                    if (dwResult)
                    {
                        DWORD dwBytes = ((LPINTERNET_ASYNC_RESULT) (pvInfo) )->dwError;
                        pCINet->OnINetDataAvailable(dwBytes);
                    }
                    else
                    {
                        hrError = pCINet->SetBindResult(((LPINTERNET_ASYNC_RESULT) (pvInfo))->dwError);
                    }
                }
                break;

                case INetState_READ_DIRECT:
                {
                    pCINet->OnINetReadDirect(0);
                }
                break;

                case INetState_DATA_AVAILABLE_DIRECT:
                {
                    PProtAssert((FALSE));
                }
                break;

                default:
                break;
                }

                // unguard - release
                pCINet->PrivRelease();

            }
            break;

            case INTERNET_STATUS_RESOLVING_NAME          :
            {
                // get server name or proxy as string
                //pCINet->ReportNotification(Notify_FindingServer, (LPSTR) pvInfo);
                pCINet->ReportNotification(BINDSTATUS_FINDINGRESOURCE, (LPSTR) pvInfo);
            }
            break;

            case INTERNET_STATUS_DETECTING_PROXY         :
            {
                // indicate that auto-proxy detection is in progress
                pCINet->ReportNotification(BINDSTATUS_PROXYDETECTING, (LPSTR) NULL);
            }
            break;
            
            case INTERNET_STATUS_CONNECTING_TO_SERVER    :
            {
                // get ip address as string
                //pCINet->ReportNotification(Notify_Connecting, (LPSTR) pvInfo);
                pCINet->ReportNotification(BINDSTATUS_CONNECTING, (LPSTR) pvInfo);
            }
            break;

            case INTERNET_STATUS_SENDING_REQUEST         :
            {
                // no data passed back
                //pCINet->ReportNotification(Notify_SendingRequest);
                pCINet->ReportNotification(BINDSTATUS_SENDINGREQUEST);
            }
            break;

            case INTERNET_STATUS_REDIRECT                :
            {
                PerfDbgLog1(tagCINet, pCINet, "+CINet::CINetCallback Redirected by WinINet (szRedirectUrl:%s)", (LPSTR) pvInfo);

                // pvinfo contains the new url
                pCINet->OnRedirect((LPSTR) pvInfo);
            }
            break;

            case INTERNET_STATUS_HANDLE_CLOSING          :
            {
                if ((*(LPHINTERNET)pvInfo) == pCINet->_hServer)
                {
                    hrError = INET_E_OK;
                    PerfDbgLog1(tagCINet, pCINet, "=== CINet::CINetCallback (Close Service Handle:%lx)", (*(LPHINTERNET) pvInfo));
                    PProtAssert((pCINet->_HandleStateServer == HandleState_Closed));
                    // this is the connect handle - call Release
                    pCINet->_hServer = 0;
                    pCINet->PrivRelease(TRUE);

                }
                else if ((*(LPHINTERNET)pvInfo) == pCINet->_hRequest)
                {
                    hrError = INET_E_OK;
                    PerfDbgLog1(tagCINet, pCINet, "=== CINet::CINetCallback (Close Request Handle:%lx)", (*(LPHINTERNET) pvInfo));
                    PProtAssert(( pCINet->_HandleStateRequest == HandleState_Closed));
                    // this is the connect handle - call Release
                    pCINet->_hRequest = 0;
                    pCINet->PrivRelease(TRUE);
                }

            }
            break;

            case INTERNET_STATUS_HANDLE_CREATED          :
            case INTERNET_STATUS_NAME_RESOLVED           :
            case INTERNET_STATUS_CONNECTED_TO_SERVER     :
            case INTERNET_STATUS_REQUEST_SENT            :
            case INTERNET_STATUS_RECEIVING_RESPONSE      :
            case INTERNET_STATUS_RESPONSE_RECEIVED       :
            case INTERNET_STATUS_CTL_RESPONSE_RECEIVED   :
            case INTERNET_STATUS_PREFETCH                :
            case INTERNET_STATUS_CLOSING_CONNECTION      :
            case INTERNET_STATUS_CONNECTION_CLOSED       :


            default:
            {
                //handle other status here
            }

            } // end switch

            if (hrError != INET_E_OK)
            {
                PerfDbgLog2(tagCINet, pCINet, "=== CINet::CINetCallback _hrINet:%lx, ERROR: %lx",
                    pCINet->_hrINet, hrError);
                // we need to terminate here
                pCINet->ReportResultAndStop(pCINet->_hrINet);
            }
            // unguard - release
            //pCINet->Release();

        }
#ifdef INET_CALLBACK_EXCEPTION
        _except(UrlMonInvokeExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            dwFault = GetExceptionCode();

            #if DBG == 1
            //
            // UrlMon catches exceptions when the client generates them. This is so we can
            // cleanup properly, and allow urlmon to continue.
            //
            if (   dwFault == STATUS_ACCESS_VIOLATION
                || dwFault == 0xC0000194 /*STATUS_POSSIBLE_DEADLOCK*/
                || dwFault == 0xC00000AA /*STATUS_INSTRUCTION_MISALIGNMENT*/
                || dwFault == 0x80000002 /*STATUS_DATATYPE_MISALIGNMENT*/ )
            {
                WCHAR iidName[256];
                iidName[0] = 0;
                char achProgname[256];
                achProgname[0] = 0;

                GetModuleFileNameA(NULL,achProgname,sizeof(achProgname));
                DbgLog2(tagCINetErr, NULL,
                               "NotificationMgr has caught a fault 0x%08x on behalf of application %s",
                               dwFault, achProgname);
                //TransAssert((!"The application has faulted processing. Check the kernel debugger for useful output.URLMon can continue but you probably want to stop and debug the application."));

            }
            #endif
        }
#ifdef unix
        __endexcept
#endif /* unix */
#endif INET_CALLBACK_EXCEPTION
    }

    PerfDbgLog1(tagCINet, pCINet, "-CINet::CINetCallback (hrError:%lx)", hrError);
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::TransitState
//
//  Synopsis:
//
//  Arguments:  [dwState] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CINet::TransitState(DWORD dwState, BOOL fAsync)
{
    PerfDbgLog(tagCINet, this, "+CINet::TransitState");

    if ((_dwState != INetState_DONE) && (_dwState != INetState_ERROR))
    {
        BINDSTATUS NMsg = (BINDSTATUS) ((fAsync) ? BINDSTATUS_INTERNALASYNC : BINDSTATUS_INTERNAL);
        DWORD dwFlags = 0;

        if (   NMsg == BINDSTATUS_INTERNALASYNC
            || NMsg == BINDSTATUS_ERROR
            || NMsg == BINDSTATUS_INTERNALASYNC)
        {
            dwFlags |= PI_FORCE_ASYNC;
        }
        if (   (dwState == INetState_AUTHENTICATE)
            || (dwState == INetState_DISPLAY_UI))
        {
            dwFlags |= PD_FORCE_SWITCH;
        }


        if( _grfBindF & BINDF_FROMURLMON )
        {
            CStateInfo CSI = CStateInfo(NMsg, dwFlags, (LPVOID) dwState);
            if( _pCTrans )    
            {
                _pCTrans->Switch(&CSI);
            }
        }
        else 
        {
            CStateInfo* pCSI = new CStateInfo(NMsg, dwFlags, (LPVOID) dwState);
            if( !pCSI ) 
            {
                ReportResultAndStop(E_OUTOFMEMORY);
            }
            else
            {
                if( dwFlags & PD_FORCE_SWITCH || _fForceSwitch )
                {
                    if( _pCTrans )
                    {
                        _pCTrans->Switch(pCSI);
                    }
                }
                else
                {
                    Continue(pCSI);
                }
            }
        }
    }

    PerfDbgLog(tagCINet, this, "-CINet::TransitState");
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetInternal
//
//  Synopsis:
//
//  Arguments:  [dwState] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CINet::OnINetInternal(DWORD_PTR dwState)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetInternal");

    HRESULT hr = NOERROR;

    if ((_dwState != INetState_DONE) && (_dwState != INetState_ERROR))
    {
        switch (dwState)
        {
        case INetState_START           :
            // is requested
            hr = INetAsyncOpen();
            break;
        case INetState_OPEN_REQUEST    :
            hr = INetAsyncConnect();
            break;
        case INetState_CONNECT_REQUEST :
            hr = INetAsyncOpenRequest();
            break;
        case INetState_PROTOPEN_REQUEST:
            hr = INetAsyncSendRequest();
            break;
        case INetState_SEND_REQUEST    :
            if( _fSendRequestAgain )
            {
                _fCompleted = FALSE;
                _fSendAgain = TRUE;
                _fSendRequestAgain = FALSE;
                hr = INetAsyncSendRequest();
            }
            else
            {
                hr = INetQueryInfo();
            }
            break;
        case INetState_DISPLAY_UI      :
            hr = INetDisplayUI();
            break;
        case INetState_AUTHENTICATE    :
            hr = INetAuthenticate();
            break;
        case INetState_READ            :
            hr = INetRead();
            break;
        case INetState_READ_DIRECT     :
            hr = INetReadDirect();
            break;
        case INetState_DATA_AVAILABLE  :
            hr = INetReportAvailableData();
            break;
        case INetState_INETSTATE_CHANGE:
            hr = INetStateChange();
            break;
        case INetState_DONE            :
            break;
        default:
            break;
        }
    }
    /*
    else
    {
        PProtAssert((FALSE && "Unknown state"));
    }
    */

    if ((hr != NOERROR) && (hr != E_PENDING))
    {
        ReportResultAndStop(hr);
    }

    PerfDbgLog(tagCINet, this, "-CINet::OnINetInternal");
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::TerminateRequest
//
//  Synopsis:   Close the server and request handle - wininet will make a
//              callback on each handle closed
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CINet::TerminateRequest()
{
    PerfDbgLog(tagCINet, this, "+CINet::TerminateRequest");
    CLock lck(_mxs);    // only one thread should be in here

    if ((_HandleStateRequest == HandleState_Initialized))
    {
        PProtAssert((_hRequest));
        _HandleStateRequest = HandleState_Closed;
        InternetCloseHandle(_hRequest);
        //_hRequest = 0;
    }
    else if ((_HandleStateRequest == HandleState_Pending))
    {
        _HandleStateRequest = HandleState_Aborted;
    }

    if (_HandleStateServer == HandleState_Initialized)
    {
        PerfDbgLog1(tagCINet, this, "=== CINet::TerminateRequest InternetCloseHandle (hServer:%lx)", _hServer);
        _HandleStateServer = HandleState_Closed;

        if (_hServer)
        {
            // the handle can be NULL
            // in case we got aborted during the
            // pending open request
            InternetCloseHandle(_hServer);
        }
    }
    else if ((_HandleStateServer == HandleState_Pending))
    {
        _HandleStateServer = HandleState_Aborted;
    }

    PerfDbgLog(tagCINet, this, "-CINet::TerminateRequest");
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::FindTagInHeader
//
//  Synopsis:
//
//  Arguments:  [lpszBuffer] --
//              [lpszTag] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR CINet::FindTagInHeader(LPCSTR lpszBuffer, LPCSTR lpszTag)
{
    PerfDbgLog(tagCINet, this, "+CINet::FindTagInHeader");
    LPCSTR p;
    int i, cbTagLen;

    cbTagLen = strlen(lpszTag);
    for (p = lpszBuffer; i = strlen(p); p += (i + 1))
    {
        if (!StrCmpNI(p, lpszTag, cbTagLen))
        {
            return (LPSTR)p;
        }
    }

    PerfDbgLog(tagCINet, this, "-CINet::FindTagInHeader");
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetDataAvailable
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetDataAvailable()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetDataAvailable");

    HRESULT hr = NOERROR;
    BOOL fRet = FALSE;

    PProtAssert((GetStatePending() == NOERROR));
    SetINetState(INetState_DATA_AVAILABLE);

    if (!_fFilenameReported)
    {
        char szFilename[MAX_PATH];

        HRESULT hr1 = GetUrlCacheFilename(szFilename, MAX_PATH);

        if (hr1 == NOERROR && szFilename[0] != '\0' )
        {
            ReportNotification(BINDSTATUS_CACHEFILENAMEAVAILABLE, (LPSTR) szFilename);
            _fFilenameReported = TRUE;
        }
    }

    // check if all data were read of the current buffer
    SetStatePending(E_PENDING);

    fRet = InternetQueryDataAvailable(_hRequest, &_cbReadReturn, 0, 0);

    if (fRet == FALSE)
    {
        dwLstError = GetLastError();
        if (dwLstError == ERROR_IO_PENDING)
        {
            hr = E_PENDING;
        }
        else
        {
            SetStatePending(NOERROR);
            hr = _hrError = INET_E_DATA_NOT_AVAILABLE;
        }
    }
    else
    {
        SetByteCountReadyToRead(_cbReadReturn);
        SetStatePending(NOERROR);

        if (_cbReadReturn == 0)
        {
            // done
            _fDone = 1;
        }

        hr = INetReportAvailableData();
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetDataAvailable (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetDataAvailable
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    2-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetDataAvailable( DWORD dwResult)
{
    PerfDbgLog1(tagCINet, this, "+CINet::OnINetDataAvailable (dwAvailable:%ld)", dwResult);
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));

    SetByteCountReadyToRead(dwResult);

    if (dwResult == 0)
    {
        // done
        _fDone = 1;
    }

    // set state to normal - no pending transaction
    SetStatePending(NOERROR);

    // notification for next request
    TransitState(INetState_DATA_AVAILABLE);


    PerfDbgLog1(tagCINet, this, "-CINet::OnINetDataAvailable (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetReportAvailableData
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-23-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetReportAvailableData()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetReportAvailableData");
    HRESULT hr = NOERROR;
    DWORD dwError;
    ULONG cbBytesAvailable = 0;

    //BUGBUG: we hit this assertion sometimes.
    //PProtAssert((GetStatePending() == NOERROR));

    _hrError = INET_E_OK;

    if ((GetStatePending() == E_PENDING))
    {
        // nothing to do - data for this notfication
        // already received
    }
    else if ((cbBytesAvailable = GetByteCountReadyToRead()) != 0)
    {
        if (   _fDone
            || (   _cbTotalBytesRead
                  && _cbDataSize
                  && (_cbTotalBytesRead == _cbDataSize)))
        {

            _hrError = INET_E_DONE;
        }
        else
        {
            if(_cbDataSize && (_cbDataSize < (cbBytesAvailable + _cbTotalBytesRead)))
            {
                _cbDataSize = cbBytesAvailable + _cbTotalBytesRead;
            }

            if (_bscf & BSCF_DATAFULLYAVAILABLE) 
            {
                _bscf |= BSCF_LASTDATANOTIFICATION;
                _bscf &= ~BSCF_FIRSTDATANOTIFICATION;
            }
            // BUG-WORK pCTrans migh be gone by now
            _cbSizeLastReportData = cbBytesAvailable + _cbTotalBytesRead;
            hr = _pEmbdFilter->ReportData(_bscf, cbBytesAvailable + _cbTotalBytesRead, _cbDataSize);

            if (_bscf & BSCF_FIRSTDATANOTIFICATION)
            {
                _bscf &= ~BSCF_FIRSTDATANOTIFICATION;
                _bscf |= BSCF_INTERMEDIATEDATANOTIFICATION;
            }
        }

    }
    else if (   _fDone
             || (   _cbTotalBytesRead
                 && _cbDataSize
                 && (_cbTotalBytesRead == _cbDataSize)))
    {
        if (_cbDataSize == 0)
        {
            _cbDataSize = _cbTotalBytesRead;
        }

        PerfDbgLog2(tagCINet, this, "=== CINet::INetReportAvailableData DONE! (cbTotalBytesRead:%ld, cbDataSize:%ld)", _cbTotalBytesRead, _cbDataSize);
        // now we should have all data
        PProtAssert(( (   ( _cbDataSize == 0)
                       || ((_cbDataSize != 0) && (_cbTotalBytesRead == _cbDataSize)))
                     && "Did not get all data!!"));


        _hrError = INET_E_DONE;
    }

    if (_hrError != INET_E_OK)
    {
        _bscf |= BSCF_LASTDATANOTIFICATION;
        if (_pCTrans )
        {
            _cbSizeLastReportData = _cbTotalBytesRead; 
            hr = _pEmbdFilter->ReportData(_bscf, _cbTotalBytesRead, _cbDataSize);

        }
        hr = NOERROR;
    }

    PerfDbgLog2(tagCINet, this, "-CINet::INetReportAvailableData (_hrError:%lx, hr:%lx)", _hrError, hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::ReadDataHere
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBytes] --
//              [pcbBytes] --
//
//  Returns:
//
//  History:    2-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::ReadDataHere(BYTE *pBuffer, DWORD cbBytes, DWORD *pcbBytes)
{
    PerfDbgLog(tagCINet, this, "+CINet::ReadDataHere");
    HRESULT hr = NOERROR;
    DWORD dwError;

    *pcbBytes = 0;
    ULONG cbReadReturn = 0;
    ULONG dwReturned = 0;
    ULONG dwReturnedTotal = 0;
    ULONG dwBytesLeft = cbBytes;

    //BUGBUG: turn this assertion on again
    //PProtAssert((GetStatePending() == NOERROR));

    if (_hrError == INET_E_DONE)
    {
        // means end of file
        hr = S_FALSE;
    }
    else if (GetStatePending() != NOERROR)
    {
        hr = E_PENDING;
    }
    else
    {
        _hrError = INET_E_OK;
        do
        {
            if ((cbReadReturn = GetByteCountReadyToRead()) == 0)
            {
                BOOL fRet;

                PerfDbgLog(tagCINet, this, "CINet::ReadDataHere -> InternetQueryDataAvailable");
                PProtAssert((GetStatePending() == NOERROR));
                SetStatePending(E_PENDING);
                fRet = InternetQueryDataAvailable(_hRequest, &_cbReadReturn,0 ,0);

                if (fRet == FALSE)
                {
                    dwLstError = GetLastError();
                    if (dwLstError == ERROR_IO_PENDING)
                    {
                        hr = E_PENDING;
                    }
                    else
                    {
                        SetStatePending(NOERROR);
                        hr = _hrError = INET_E_DATA_NOT_AVAILABLE;
                    }
                }
                else
                {
                    SetByteCountReadyToRead(_cbReadReturn);
                    SetStatePending(NOERROR);
                    if (_cbReadReturn == 0)
                    {
                        // download completed - no more data available
                        hr = _hrError = INET_E_DONE;
                    }
                }

                PerfDbgLog2(tagCINet, this, "CINet::ReadDataHere == InternetQueryDataAvailable (fRet:%d, _cbReadReturn:%ld)", fRet, _cbReadReturn);
            }

            // in case of noerror read the bits
            if ((hr == NOERROR) && (_hrError == INET_E_OK))
            {
                cbReadReturn = GetByteCountReadyToRead();
                PProtAssert((GetStatePending() == NOERROR));

                // get the read buffer from the trans data object
                PProtAssert(((pBuffer != NULL) && (cbBytes > 0)));
                PProtAssert((cbReadReturn > 0));

                dwBytesLeft = cbBytes - dwReturnedTotal;
                if (dwBytesLeft > cbReadReturn)
                {
                    dwBytesLeft = cbReadReturn;
                }

                PProtAssert(( dwBytesLeft <= (cbBytes - dwReturnedTotal) ));

                dwReturned = 0;
                PerfDbgLog1(tagCINet, this, "CINet::ReadDataHere -> InternetReadFile (dwBytesLeft:%ld)", dwBytesLeft);
                if (!InternetReadFile(_hRequest, pBuffer + dwReturnedTotal, dwBytesLeft, &dwReturned))
                {
                    dwError = GetLastError();
                    if (dwError != ERROR_IO_PENDING)
                    {
                        hr = _hrError = INET_E_DOWNLOAD_FAILURE;
                        DbgLog3(tagCINetErr, this, "CINet::ReadDataHere failed: (dwError:%lx, hr:%lx, hrError:%lx)",
                                                    dwError, hr, _hrError);

                    }
                    else
                    {
                        // Note: BIG ERROR - we need to shut down now
                        // wininet is using the client buffer and the client is not
                        // aware that the buffer is used during the pending time
                        //
                        DbgLog(tagCINetErr, this, "CINet::ReadDataHere - InternetReadFile returned E_PENDING!!!");
                        PProtAssert((FALSE &&  "CINet::ReadDataHere - InternetReadFile returned E_PENDING!!!"));

                        hr = _hrError = INET_E_DOWNLOAD_FAILURE;
                    }

                    PerfDbgLog1(tagCINet, this, "CINet::ReadDataHere == InternetReadFile (dwError:%lx)", dwError);

                }
                else
                {
                    PerfDbgLog3(tagCINet, this, "CINet::ReadDataHere == InternetReadFile ==> (cbBytes:%ld, dwReturned:%ld,_cbReadReturn:%ld)",
                        cbBytes, dwReturned,_cbReadReturn);

                    PProtAssert((  (cbBytes + dwReturnedTotal) >= dwReturned ));

                    if (dwReturned == 0)
                    {
                        hr = _hrError = INET_E_DONE;
                    }
                    else
                    {
                        hr = NOERROR;
                    }

                    dwReturnedTotal += dwReturned;
                    cbReadReturn -= dwReturned;
                    SetByteCountReadyToRead(-(LONG)dwReturned);
                    _cbTotalBytesRead += dwReturned;
                }
            }  // read case - bits available

            PerfDbgLog4(tagCINet, this, "CINet::ReadDataHere ooo InternetReadFile ==>(cbBytes:%ld, dwReturned:%ld,cbReadReturn:%ld,dwReturnedTotal:%ld)",
                cbBytes, dwReturned,cbReadReturn,dwReturnedTotal);

        } while ((hr == NOERROR) && (dwReturnedTotal < cbBytes));

        PProtAssert((dwReturnedTotal <= cbBytes));
        *pcbBytes = dwReturnedTotal;

        if (hr == INET_E_DONE)
        {
            hr = (dwReturnedTotal) ? S_OK : S_FALSE;
        }
    }

    // Note: stop the download in case of DONE or ERROR!
    if (_hrError != INET_E_OK)
    {
        ReportResultAndStop((hr == S_FALSE) ? NOERROR : hr,  _cbTotalBytesRead, _cbDataSize);
    }

    PerfDbgLog4(tagCINet, this, "-CINet::ReadDataHere (_hrError:%lx, [hr:%lx,cbBytesAsked:%ld,cbBytesReturned:%ld])",
        _hrError, hr, cbBytes, *pcbBytes);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::OperationOnAparmentThread
//
//  Synopsis:
//
//  Arguments:  [dwState] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CINet::OperationOnAparmentThread(DWORD dwState)
{
    PerfDbgLog(tagCINet, this, "+CINet::OperationOnAparmentThread");
    BOOL fRet = FALSE;
    switch (dwState)
    {
    case INetState_OPEN_REQUEST:
        break;
    case INetState_CONNECT_REQUEST:
        break;
    case INetState_PROTOPEN_REQUEST:
        break;
    case INetState_SEND_REQUEST:
        break;
    case INetState_READ:
    case INetState_READ_DIRECT:
        fRet = TRUE;
        break;
    default:
        fRet = TRUE;
        break;
    }
    //return fRet;

    PerfDbgLog(tagCINet, this, "-CINet::OperationOnAparmentThread");
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OperationOnAparmentThread
//
//  Synopsis:
//
//  Returns:
//
//  History:    10-27-98   DanpoZ (Danpo Zhang)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CINet::UTF8Enabled()
{
    BOOL bRet = FALSE;
    // do not enable utf8 on file or ftp protocol
    if( _dwIsA == DLD_PROTOCOL_FILE ||
        _dwIsA == DLD_PROTOCOL_FTP  )
    {
        goto exit;
    }

    // default to per-machine utf-8 setting
    bRet = g_bGlobalUTF8Enabled;

    // per-binding flag
    if( _BndInfo.dwOptions & BINDINFO_OPTIONS_ENABLE_UTF8)
    {
        bRet = TRUE;
    }

    if( _BndInfo.dwOptions & BINDINFO_OPTIONS_DISABLE_UTF8)
    {
        bRet = FALSE;
    }

    if( _BndInfo.dwOptions & BINDINFO_OPTIONS_USE_IE_ENCODING)
    {
        DWORD dwIE;
        DWORD dwOutLen = sizeof(DWORD);
        HRESULT hr = UrlMkGetSessionOption(
            URLMON_OPTION_URL_ENCODING,
            &dwIE,
            sizeof(DWORD),
            &dwOutLen,
            NULL );

        if( hr == NOERROR )
        {
            if( dwIE == URL_ENCODING_ENABLE_UTF8 )
            {
                bRet = TRUE;
            }
            else if( dwIE == URL_ENCODING_DISABLE_UTF8 )
            {
                bRet = FALSE;
            }
        }
    }

exit:
    return bRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryInfoOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::QueryInfoOnResponse()
{
    PerfDbgLog(tagCINet, this, "+CINet::QueryInfoOnResponse");
    HRESULT hr = NOERROR;

    DWORD dwFlags;
    DWORD cbLen = sizeof(dwFlags);

    // See if it is from the cache
    if (InternetQueryOption(_hRequest, INTERNET_OPTION_REQUEST_FLAGS, &dwFlags, &cbLen))
    {
        if (dwFlags & INTERNET_REQFLAG_FROM_CACHE)
        {
            _dwCacheFlags |= INTERNET_REQFLAG_FROM_CACHE;
            // set flag that data are from cache
            _bscf |= BSCF_DATAFULLYAVAILABLE;
        }
    }

    hr =  QueryStatusOnResponse();
    if (hr == NOERROR)
    {
        hr = QueryHeaderOnResponse();
    }

    if (_hrError != INET_E_OK)
    {
        SetINetState(INetState_DONE);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::QueryInfoOnResponse (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryStatusOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::QueryStatusOnResponse()
{
    PerfDbgLog(tagCINet, this, "+CINet::QueryStatusOnResponse");

    PProtAssert((FALSE));

    PerfDbgLog1(tagCINet, this, "-CINet::QueryStatusOnResponse hr:%lx", E_FAIL);
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryStatusOnResponseDefault
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::QueryStatusOnResponseDefault(DWORD dwStat)
{
    PerfDbgLog(tagCINet, this, "+CINet::QueryStatusOnResponseDefault");

    PProtAssert((FALSE));

    PerfDbgLog1(tagCINet, this, "-CINet::QueryStatusOnResponseDefault hr:%lx", E_FAIL);
    return E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::QueryHeaderOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::QueryHeaderOnResponse()
{
    PerfDbgLog(tagCINet, this, "+CINet::QueryHeaderOnResponse");

    PProtAssert((FALSE));

    PerfDbgLog1(tagCINet, this, "-CINet::QueryHeaderOnResponse hr:%lx", E_FAIL);
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::RedirectRequest
//
//  Synopsis:
//
//  Arguments:  [lpszBuffer] --
//              [pdwBuffSize] --
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::RedirectRequest(LPSTR lpszBuffer, DWORD *pdwBuffSize )
{
    PerfDbgLog(tagCINet, this, "+CINet::RedirectRequest");

    PProtAssert((FALSE));

    PerfDbgLog1(tagCINet, this, "-CINet::RedirectRequest(fRet:%ld)", FALSE);
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::AuthenticationRequest
//
//  Synopsis:
//
//  Arguments:  [lpszBuffer] --
//              [pdwBuffSize] --
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::AuthenticationRequest()
{
    PerfDbgLog(tagCINet, this, "+CINet::AuthenticationRequest");
    HRESULT hr = NOERROR;
    DWORD dwError;

    LPWSTR pwzUsername = NULL;
    LPWSTR pwzPassword = NULL;

    DWORD  dwBindF = GetBindFlags();


    PerfDbgLog2(tagCINet, this, "<1> _cbProxyAuthenticate:%d _cbAuthenticate:%d",
               _cbProxyAuthenticate, _cbAuthenticate);
    if (_fProxyAuth ?  (_cbProxyAuthenticate >= AUTHENTICATE_MAX):(_cbAuthenticate >= AUTHENTICATE_MAX))
    {
        // NOTE: set the error to noerror and
        // continue reading data and show the 401 contained
        _hrINet = hr = NOERROR;
    }
    else
    {
        if (_hwndAuth == NULL)
        {
            IAuthenticate *pPInfo;
            DbgLog(tagCINetErr, this, "+CINet::AuthenticationRequest: QS for IAuthenticate");
            hr = QueryService(IID_IAuthenticate, (void **)&pPInfo);
            if (hr == NOERROR)
            {
                PProtAssert((pPInfo));
                hr = pPInfo->Authenticate(&_hwndAuth, &pwzUsername, &pwzPassword);
                pPInfo->Release();
            }
        }

        if (hr == NOERROR)
        {
            _hrINet = hr = E_ACCESSDENIED;

            if (pwzUsername && pwzPassword)
            {
                // set the username and password
                // and retry sendrequest

                LPSTR pszUsername = DupW2A(pwzUsername);
                LPSTR pszPassword = DupW2A(pwzPassword);

                if (pszUsername)
                {
                    InternetSetOption(_hRequest, INTERNET_OPTION_USERNAME, pszUsername, strlen(pszUsername));
                    delete pszUsername;
                }
                if (pszPassword)
                {
                    InternetSetOption(_hRequest, INTERNET_OPTION_PASSWORD, pszPassword, strlen(pszPassword));
                    delete pszPassword;
                }

                // if we got username & pwd, only try once
                _fProxyAuth ?  _cbProxyAuthenticate = AUTHENTICATE_MAX :
                                _cbAuthenticate = AUTHENTICATE_MAX;
                _hrINet = hr = RPC_E_RETRY;
                PerfDbgLog2(tagCINet, this, "<2> _cbProxyAuthenticate:%d _cbAuthenticate:%d",
                       _cbProxyAuthenticate, _cbAuthenticate);
        }

            if (   (_hwndAuth || (_hwndAuth == (HWND)-1) )
                && (_pCAuthData == NULL) 
                && (hr != RPC_E_RETRY) )

            {
                PProtAssert((_pCAuthData == NULL));
                _pCAuthData = new CAuthData(this);
            }

            if (   (_hwndAuth || (_hwndAuth == (HWND)-1) )
                && _pCAuthData
                && (hr != RPC_E_RETRY) )
            {
                BOOL fRetry = FALSE;
                BOOL fDeleteAuthData = TRUE;
                DWORD dwFlags = (  FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS
                                 | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_SERIALIZE_DIALOGS);

                if ((dwBindF & BINDF_NO_UI) || (dwBindF & BINDF_SILENTOPERATION))
                {
                    dwFlags |= FLAGS_ERROR_UI_FLAGS_NO_UI;
                }

                if (_hwndAuth == (HWND)-1)
                {
                    _hwndAuth = 0;
                }

                do
                {
                    _fProxyAuth ? _cbProxyAuthenticate++ : _cbAuthenticate++;
                    PerfDbgLog2(tagCINet, this, "<3> _cbProxyAuthenticate:%d _cbAuthenticate:%d",
                               _cbProxyAuthenticate, _cbAuthenticate);

                    dwError = InternetErrorDlg(_hwndAuth,_hRequest,ERROR_SUCCESS,dwFlags,(LPVOID *)&_pCAuthData);

                    switch (dwError)
                    {
                    case ERROR_CANCELLED :
                        // wininet should never return cancelled here
                        PProtAssert((FALSE));
                    case ERROR_SUCCESS  :
                        // NOTE: succes and cancel means display the content according to ArthurBi
                        // continue reading data and show the 401 contained
                        _hrINet = hr = NOERROR;
                        break;

                    case ERROR_INTERNET_FORCE_RETRY :
                        _hrINet = hr = RPC_E_RETRY;
                        break;

                    case ERROR_INTERNET_DIALOG_PENDING :
                        // a dialog is up on another thread
                        // start wating on the callback
                        SetINetState(INetState_AUTHENTICATE);
                        SetStatePending(E_PENDING);
                        _fProxyAuth ? _cbProxyAuthenticate-- : _cbAuthenticate--;
                        PerfDbgLog2(tagCINet, this, "<4> _cbProxyAuthenticate:%d _cbAuthenticate:%d",
                                   _cbProxyAuthenticate, _cbAuthenticate);
                        fDeleteAuthData = FALSE;
                        _hrINet = hr = E_PENDING;
                        break;

                    case ERROR_INTERNET_RETRY_DIALOG:
                        _fProxyAuth ? _cbProxyAuthenticate-- : _cbAuthenticate--;
                        PerfDbgLog2(tagCINet, this, "<5> _cbProxyAuthenticate:%d _cbAuthenticate:%d",
                                   _cbProxyAuthenticate, _cbAuthenticate);
                        fRetry = TRUE;
                        break;

                    default:
                        _hrINet = hr = E_ACCESSDENIED;
                        break;
                    }

                } while (fRetry);

                if (fDeleteAuthData)
                {
                    delete _pCAuthData;
                    _pCAuthData = NULL;
                }

            }
        }
        else
        {
            _hrINet = hr = E_ACCESSDENIED;
        }

        if (hr == RPC_E_RETRY)
        {
            _hrINet = NOERROR;
            _fCompleted = FALSE;
            _fSendAgain = TRUE;
            if (IsA() == DLD_PROTOCOL_FTP || IsA() == DLD_PROTOCOL_GOPHER)
            {
                // Retry InternetOpenUrl using the updated auth info.
                _fDoSimpleRetry = TRUE;
                hr = INetAsyncOpenRequest();
            }
            else
            {  
                hr = INetAsyncSendRequest();
            }
        }
        else if (hr == E_PENDING)
        {
            // do nothing and wait for async completion
        }
        else if (   (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
                 && (hr != NOERROR))
        {
            // set the error to access denied
            _hrINet = hr = E_ACCESSDENIED;
        }
    }

    if (pwzUsername)
    {
        delete pwzUsername;
    }
    if (pwzPassword)
    {
        delete pwzPassword;
    }

    PerfDbgLog1(tagCINet, this, "-CINet::AuthenticationRequest(hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetUrlCacheFilename
//
//  Synopsis:
//
//  Arguments:  [szFilename] --
//              [dwSize] --
//
//  Returns:
//
//  History:    2-28-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::GetUrlCacheFilename(LPSTR szFilename, DWORD dwSize)
{
    PerfDbgLog(tagCINet, this, "+CINet::GetUrlCacheFilename");
    HRESULT hr = NOERROR;
    BOOL fRet = FALSE;
    DWORD dwSizeLocal = dwSize;
    DWORD dwError = 0;

    if (dwSize)
    {
        szFilename[0] = '\0';
    }
    
    if (   !(GetBindFlags() & BINDF_NOWRITECACHE)
        ||  (GetBindFlags() & BINDF_NEEDFILE))
    {
        fRet = InternetQueryOption(_hRequest, INTERNET_OPTION_DATAFILE_NAME, szFilename, &dwSizeLocal);
        
        if (!fRet && (GetBindFlags() & BINDF_NEEDFILE))
        {
            dwError = GetLastError();
            hr = INET_E_DATA_NOT_AVAILABLE;
            SetBindResult(dwError, hr);
            if (dwSize)
            {
                szFilename[0] = '\0';
            }
        }
    }

    PerfDbgLog3(tagCINet, this, "-CINet::GetUrlCacheFilename (hr:%lx,fRet%d; szFilename:%s)", hr, fRet, szFilename);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::LockFile
//
//  Synopsis:
//
//  Arguments:  [szFilename] --
//              [dwSize] --
//
//  Returns:
//
//  History:    8-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::LockFile(BOOL fRetrieve)
{
    PerfDbgLog(tagCINet, this, "+CINet::LockFile");
    HRESULT hr = NOERROR;
    BOOL fRet = FALSE;

    if (fRetrieve)
    {
        DWORD dwCacheEntryInfoBufferSize = MAX_URL_SIZE + MAX_PATH + sizeof(INTERNET_CACHE_ENTRY_INFO) + 2;
        INTERNET_CACHE_ENTRY_INFO *pCacheEntryInfo = (INTERNET_CACHE_ENTRY_INFO *)new CHAR [dwCacheEntryInfoBufferSize];
        DWORD dwError = 0;
        if (   (_fLocked == FALSE)
            && (pCacheEntryInfo != NULL)
            && (RetrieveUrlCacheEntryFileA( _pszFullURL, pCacheEntryInfo, &dwCacheEntryInfoBufferSize, 0)))
        {
            _fLocked = TRUE;
        }

        if (pCacheEntryInfo != NULL)
        {
            delete pCacheEntryInfo;
        }
    }
    else if ((_hLockHandle == NULL) && _hRequest)
    {
        if (InternetLockRequestFile(_hRequest, &_hLockHandle))
        {
            PProtAssert((_hLockHandle));
        }
    }

    PerfDbgLog2(tagCINet, this, "-CINet::LockFile (hr:%lx,fRet%d)", hr, fRet);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::UnlockFile
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::UnlockFile()
{
    PerfDbgLog(tagCINet, this, "IN CINet::UnlockFile");
    HRESULT hr = NOERROR;

    if (_fLocked)
    {
        UnlockUrlCacheEntryFile(_pszFullURL, 0);
        _fLocked = FALSE;
    }
    else if (_hLockHandle)
    {
        if (InternetUnlockRequestFile(_hLockHandle))
        {
            _hLockHandle = NULL;
        }
    }

    PerfDbgLog1(tagCINet, this, "-CINet::UnlockFile (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::HResultFromInternetError
//
//  Synopsis:   maps the dwStatus ERROR_INTERNET_XXX do an hresult
//
//  Arguments:  [dwStatus] --
//
//  Returns:
//
//  History:    3-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::HResultFromInternetError(DWORD dwStatus)
{
    PerfDbgLog(tagCINet, this, "+CINet::HResultFromInternetError");
    // dwResult is out of our know table
    HRESULT hr = INET_E_DOWNLOAD_FAILURE;
    ULONG   ulIndex = dwStatus - INTERNET_ERROR_BASE;

    PProtAssert((ulIndex > 0));
    BOOL fTable1 = (ulIndex > 0 && ulIndex < sizeof(INetError)/sizeof(InterErrorToHResult));
    DWORD dwTable2Size = sizeof(INetErrorExtended)/sizeof(InterErrorToHResult);
    BOOL fTable2 = FALSE;

    if (!fTable1)
    {
        fTable2 = (dwStatus <= INetErrorExtended[dwTable2Size].dwError);
    }
    if (fTable1)
    {
        // sequential table
        hr = INetError[ulIndex].hresult;
    }
    else if (fTable2)
    {
        // walk the table
        for (DWORD i = 0; i < dwTable2Size; i++)
        {
            if (INetErrorExtended[i].dwError == dwStatus)
            {
                hr = INetErrorExtended[i].hresult;
                break;
            }
        }
    }

    PerfDbgLog1(tagCINet, this, "-CINet::HResultFromInternetError (hr:%lx)", hr);
    return hr;
}
//+---------------------------------------------------------------------------
//
//  Function:   SetBindResult
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    5-10-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::SetBindResult(DWORD dwResult, HRESULT hrSuggested)
{
    PerfDbgLog2(tagCINet, this, "+CINet::SetBindResult(dwResult:%ld, hrSuggested:%lx)", dwResult, hrSuggested);
    HRESULT hr = NOERROR;

    PProtAssert((_pszResult == NULL));
    _pszResult = NULL;
    _dwResult = dwResult;

    // only find hresult in the mapping table
    // if no erro was suggested
    if (hrSuggested == NOERROR)
    {
        hr = _hrINet = HResultFromInternetError(_dwResult);
    }
    else
    {
         hr = _hrINet = hrSuggested;
    }

    PerfDbgLog3(tagCINet, this, "-CINet::SetBindResult (dwResult:%ld, _hrINet:%lx,  hr:%lx)", dwResult, _hrINet, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetBindResult
//
//  Synopsis:   returns the protocol specific error
//
//  Arguments:  [pclsidProtocol] --
//              [pdwResult] --
//              [DWORD] --
//              [pdwReserved] --
//
//  Returns:
//
//  History:    4-16-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::GetBindResult(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult, DWORD *pdwReserved)
{
    PerfDbgLog(tagCINet, this, "+CINet::GetBindResult");
    HRESULT hr = NOERROR;

    PProtAssert((pclsidProtocol && pdwResult && pszResult));

    *pclsidProtocol = _pclsidProtocol;
    *pdwResult = _dwResult;
    if (_pszResult)
    {
        // the client is supposted to free the string
        *pszResult = new WCHAR [strlen(_pszResult) + 1];
        if (*pszResult)
        {
            A2W(_pszResult, *pszResult, strlen(_pszResult));
        }
    }
    else
    {
        *pszResult = NULL;
    }

    PerfDbgLog2(tagCINet, this, "-CINet::GetBindResult (dwResult:%lx, szStr:%ws)", _dwResult, pszResult);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnRedirected
//
//  Synopsis:   Called on wininet worker thread when wininet does a redirect.
//              Sends notification to apartment thread.
//
//  Arguments:  [szNewUrl] --
//
//  Returns:
//
//  History:    4-17-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnRedirect(LPSTR szNewUrl)
{
    PerfDbgLog1(tagCINet, this, "+CINet::OnRedirect (szNewUrl:%s)",szNewUrl);
    HRESULT hr = NOERROR;

    PProtAssert((szNewUrl && "WinINet reports redirect with redirect URL"));

    if (szNewUrl)
    {
        _fRedirected = TRUE;
        ReportNotification(BINDSTATUS_REDIRECTING, szNewUrl);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnRedirect(hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   AppendToString
//  
//  Synopsis:   Fast append of src to dest, reallocing of dest if necessary.
//              
//
//  Arguments:  [IN/OUT] pszDest
//              [IN/OUT] pcbDest
//              [IN/OUT] pcbAlloced
//              [IN]     szSrc
//              [IN]     cbSrc
//
//  Returns:    TRUE/FALSE
//
//  History:    6-2-97   Adriaan Canter (AdriaanC)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL AppendToString(LPSTR* pszDest, LPDWORD pcbDest, 
                    LPDWORD pcbAlloced, LPSTR szSrc, DWORD cbSrc)
{
    DWORD cbNew = *pcbDest + cbSrc;

    if (cbNew > *pcbAlloced)
    {
        DWORD cbNewAlloc  = *pcbAlloced + (cbSrc < MAX_PATH ? MAX_PATH : cbSrc);
        LPSTR szNew = (LPSTR) new CHAR[cbNewAlloc];
        if (!szNew)
        {
            return FALSE;
        }
        memcpy(szNew, *pszDest, *pcbDest);
        delete [] *pszDest;
        *pszDest = szNew;        
        *pcbAlloced = cbNewAlloc;
    }

     memcpy(*pszDest + *pcbDest, szSrc, cbSrc);
    *pcbDest = cbNew;
    return TRUE;
}

#ifdef unix
#include <sys/utsname.h>
#endif /* unix */
//+---------------------------------------------------------------------------
//
//  Function:   GetUserAgentString
//
//  Synopsis:   Gets the user agent string from the registry. If entry is
//              the default string is returned.
//
//  Arguments:  (none)
//
//  Returns:    Allocated user agent string.
//
//  History:    5-13-96   JohannP (Johann Posch)   Created
//              6-02-97   AdriaanC (Adriaan Canter) Mods for mult reg entries.
//
//              6-25-97   AdriaanC (Adriaan Canter) Further mods described below.
//  
//              12-18-98  Adriaanc (Adriaan Canter) - Versioned base values for
//                        IE5 and added a versioned IE5 location for token values
//                        which will get read in addition to common (IE4) location.
//
//  Notes:      User Agent string madness: We now generate the User Agent string
//              from diverse entries in the registry. We first scan HKCU for base
//              keys, falling back to HKLM if not found, or finally, defaults.
//              For Pre and Post platform we now pickup all entries in both HKCU
//              and HKLM. Finally, for back compatibility we enumerate a list of
//              tokens (such as MSN 2.0, MSN 2.1, etc) from UA Tokens in the HKLM
//              registry under Internet Settings, and if any of these tokens are
//              found in the *old* HKCU location User Agent String we insert them
//              into the pre platform portion of the generated user agent string.
//              This was specifically done for MSN which has been fixing up the old
//              registry location and depends on these tokens being found in the 
//              User Agent string.
//
//----------------------------------------------------------------------------
LPCSTR GetUserAgentString()
{
    // Reg keys.
    #define INTERNET_SETTINGS_KEY_SZ  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"   
    #define USER_AGENT_KEY_SZ         "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\User Agent"
    #define USER_AGENT_KEY5_SZ        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\5.0\\User Agent"
    #define USER_AGENT_SZ             "User Agent"
    #define PRE_PLATFORM_KEY_SZ       "Pre Platform"
    #define POST_PLATFORM_KEY_SZ      "Post Platform"
    #define UA_TOKENS_KEY_SZ          "UA Tokens"
    
    // Base UA key strings.    
    #define UA_KEY_SZ NULL
    #define COMPATIBLE_KEY_SZ   "Compatible"
    #define VERSION_KEY_SZ      "Version"
    #define PLATFORM_KEY_SZ     "Platform"

    // Base UA value strings.
    #define UA_VAL_SZ           "Mozilla/4.0"
    #define COMPATIBLE_VAL_SZ   "compatible"
    #define VERSION_VAL_SZ      "MSIE 5.01"

    // Their lengths.
    #define UA_VAL_LEN           (sizeof(UA_VAL_SZ) - 1)
    #define COMPATIBLE_VAL_LEN   (sizeof(COMPATIBLE_VAL_SZ) - 1)
    #define VERSION_VAL_LEN      (sizeof(VERSION_VAL_SZ) - 1)


    // If we encounter a failure in constructing the string, send this.
    #define DEFAULT_UA_STRING "Mozilla/4.0 (compatible; MSIE 5.01; Win32)"

    // Used for backing up user agent string.
    #define IE4_UA_BACKUP_FLAG     "IE5_UA_Backup_Flag"
    #define BACKUP_USER_AGENT_SZ    "BackupUserAgent"

    #define COMPAT_MODE_TOKEN      "compat"
    #define NUM_UA_KEYS            4
    
    BOOL bSuccess = TRUE;
    INT i, nBaseKeys;
    DWORD dwIndex, dwType, cbBuf, cbUA, cbTotal;
    LPSTR szUA, szBuf, pszWinVer;
    OSVERSIONINFO osVersionInfo;    

    // Reg handles.
    HKEY hHKCU_ISKey;
    HKEY hHKCU_UAKey;
    HKEY hHKLM_UAKey;
    HKEY hHKCU_UA5Key;
    HKEY hHKLM_UA5Key;
    HKEY hPreKey; 
    HKEY hPostKey; 
    HKEY hTokensKey;

    // Set all regkeys to invalid handle.
    hHKCU_ISKey = hHKLM_UAKey = hHKCU_UAKey = hHKLM_UA5Key = hHKCU_UA5Key
        = hPreKey = hPostKey = hTokensKey = (HKEY) INVALID_HANDLE_VALUE;
    
    // The UA keys are iterated in loops below; Keep an array
    // of pointers to the HKLMUA, HKCUUA, HKLMUA5 and HKCUUA5 locations
    // to use as alias in the loop. NOTE!! - Do not change the ordering.
    HKEY *phUAKeyArray[NUM_UA_KEYS];
    phUAKeyArray[0] = &hHKLM_UAKey;
    phUAKeyArray[1] = &hHKCU_UAKey;
    phUAKeyArray[2] = &hHKLM_UA5Key;
    phUAKeyArray[3] = &hHKCU_UA5Key;
    
    // Platform strings.
    LPSTR szWin95 = "Windows 95";
    LPSTR szWin98 = "Windows 98";
    LPSTR szWinNT4 = "Windows NT";
    LPSTR szWinNT5 = "Windows NT 5.0";
    LPSTR szWin32 = "Win32";

#ifdef unix
    CHAR szUnixPlatformName[SYS_NMLN*4+3+1]; // 4 substrings,3 spaces, 1 NULL
#endif /* unix */

    // Arrays of base keys, values and lengths.
    LPSTR szBaseKeys[] =   {UA_KEY_SZ,  COMPATIBLE_KEY_SZ,  VERSION_KEY_SZ};
    LPSTR szBaseValues[] = {UA_VAL_SZ,  COMPATIBLE_VAL_SZ,  VERSION_VAL_SZ};
    DWORD cbBaseValues[] = {UA_VAL_LEN, COMPATIBLE_VAL_LEN, VERSION_VAL_LEN};
    
    nBaseKeys = sizeof(szBaseKeys) / sizeof(LPSTR);

    cbUA = 0;
    cbTotal = cbBuf = MAX_PATH;
    szBuf = szUA = 0;

    // User agent string already exists.
    if (g_pszUserAgentString != NULL)
    {
        szUA = g_pszUserAgentString;
        goto End;
    }

    // Max size for any one field from registry is MAX_PATH.
    szUA = new CHAR[MAX_PATH];
    szBuf = new CHAR[MAX_PATH];
    if (!szUA || !szBuf)
    {
        bSuccess = FALSE;
        goto End;
    }
    
    // Open all 4 User Agent reg keys (HKLMUA, HKCUUA, HKLMUA5, HKCUUA5).
    if (RegOpenKeyEx(HKEY_CURRENT_USER, USER_AGENT_KEY_SZ, 0, KEY_QUERY_VALUE, &hHKCU_UAKey) != ERROR_SUCCESS)
        hHKCU_UAKey = (HKEY)INVALID_HANDLE_VALUE;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, USER_AGENT_KEY_SZ, 0, KEY_QUERY_VALUE, &hHKLM_UAKey) != ERROR_SUCCESS)
        hHKLM_UAKey = (HKEY)INVALID_HANDLE_VALUE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, USER_AGENT_KEY5_SZ, 0, KEY_QUERY_VALUE, &hHKCU_UA5Key) != ERROR_SUCCESS)
        hHKCU_UA5Key = (HKEY)INVALID_HANDLE_VALUE;
        
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, USER_AGENT_KEY5_SZ, 0, KEY_QUERY_VALUE, &hHKLM_UA5Key) != ERROR_SUCCESS)
        hHKLM_UA5Key = (HKEY)INVALID_HANDLE_VALUE;

    // Get user agent, compatible and version strings from IE 5.0 location. 
    // IE6 and on must revise this location.
    for (i = 0; i < nBaseKeys; i++)
    {
        if ((hHKCU_UA5Key != INVALID_HANDLE_VALUE) && RegQueryValueEx(hHKCU_UA5Key, szBaseKeys[i], 
            NULL, &dwType, (LPBYTE) szBuf, &(cbBuf = MAX_PATH)) == ERROR_SUCCESS
            && cbBuf > 1)
        {
            // Got from HKCU registry.
            if (!(bSuccess = AppendToString(&szUA, &cbUA, 
                &cbTotal, szBuf, cbBuf - 1)))
                goto End;
        }
        else if ((hHKLM_UA5Key != INVALID_HANDLE_VALUE) && RegQueryValueEx(hHKLM_UA5Key, szBaseKeys[i], 
            NULL, &dwType, (LPBYTE) szBuf, &(cbBuf = MAX_PATH)) == ERROR_SUCCESS
            && cbBuf > 1)
        {
            // Got from HKLM registry.
            if (!(bSuccess = AppendToString(&szUA, &cbUA, 
                &cbTotal, szBuf, cbBuf - 1)))
                goto End;
        }
        else
        {
            // Got from defaults.
            if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, 
                szBaseValues[i], cbBaseValues[i])))
                goto End;
        }

        // Formating.    
        if (!(bSuccess = AppendToString(&szUA, &cbUA, 
            &cbTotal, (i == 0 ? " (" : "; "), 2)))
            goto End;
    }

    // Leave the four UA keys open; Proceed to open HKLM tokens key to scan
    // to scan and open Internet Settings HKCU key to read legacy UA string.

    // Tokens to scan for from the old registry location to include
    // in the user agent string: These are enumerated from UA Tokens,
    // scanned for in the old location and added to the pre platform.    
    if (hHKLM_UAKey != INVALID_HANDLE_VALUE)
        RegOpenKeyEx(hHKLM_UAKey, UA_TOKENS_KEY_SZ, 0, KEY_QUERY_VALUE, &hTokensKey);

    if (hTokensKey != INVALID_HANDLE_VALUE)
    {
        CHAR szOldUserAgentString[MAX_PATH];

        // Read in the old user agent string from HKCU
        RegOpenKeyEx(HKEY_CURRENT_USER, INTERNET_SETTINGS_KEY_SZ, 0, KEY_QUERY_VALUE, &hHKCU_ISKey);

        if ((hHKCU_ISKey != INVALID_HANDLE_VALUE) && RegQueryValueEx(hHKCU_ISKey, USER_AGENT_SZ,
            NULL, &dwType, (LPBYTE) szOldUserAgentString, &(cbBuf = MAX_PATH)) == ERROR_SUCCESS
            && cbBuf > 1)
        {            
            // Close the HKCU Internet Settings key.
            RegCloseKey(hHKCU_ISKey);
            hHKCU_ISKey = (HKEY) INVALID_HANDLE_VALUE;

            // Got old user agent string from HKCU registry. Enumerate the values in UA Tokens
            // and see if any exist in the old string and if so, add them to the current string.
            dwIndex = 0;
#ifndef unix
            while (RegEnumValue(hTokensKey, dwIndex++, szBuf, &(cbBuf = MAX_PATH - 4),  /* sizeof(' ' + "; ") */
                    NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                // eg; find a token enumerated from UA Tokens
                // Mozilla/4.0 (compatible; MSIE 4.0b2; MSN2.5; Windows NT)
                //                                     ^
                //                                     szBuf
                if (cbBuf)
                {
                    // Fix up token to include leading
                    // space and trailing semi-colon before strstr.
                    CHAR szToken[MAX_PATH];
                    szToken[0] = ' ';
                    memcpy(szToken+1, szBuf, cbBuf);
                    memcpy(szToken + 1 + cbBuf, "; ", sizeof("; "));

                    // Found a match - insert this token into user agent string.
                    if (strstr(szOldUserAgentString, szToken))
                    {
                        if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, szBuf, cbBuf)))
                            goto End;

                        if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, "; ", 2)))
                            goto End;
                    }
                }
            }
#endif /* !unix */
        }
    
        RegCloseKey(hTokensKey);
        hTokensKey = (HKEY) INVALID_HANDLE_VALUE;
    }
            

    // Pre platform strings - get from HKCUUA, HKLMUA,
    // HKLMUA5 and HKCUUA5 locations. These are additive;
    // order is not important.
    for (i = 0; i < NUM_UA_KEYS; i++)
    {
        if (*(phUAKeyArray[i]) == INVALID_HANDLE_VALUE)
            continue;
        
        RegOpenKeyEx(*(phUAKeyArray[i]),
            PRE_PLATFORM_KEY_SZ, 0, KEY_QUERY_VALUE, &hPreKey);

        if (hPreKey != INVALID_HANDLE_VALUE)
        {
            dwIndex = 0;
            while (RegEnumValue(hPreKey, dwIndex++, szBuf, &(cbBuf = MAX_PATH),
                                NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                if (cbBuf)
                {
                    // Got from registry and non null.
                    if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, szBuf, cbBuf)))
                        goto End;
                    if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, "; ", 2)))
                        goto End;
                }
                cbBuf = MAX_PATH;
            }
        
            // Close pre platform key; User agent keys still open.
            RegCloseKey(hPreKey);
            hPreKey = (HKEY) INVALID_HANDLE_VALUE;
        }
    }     


    // Platform string. This is read from the IE 5.0 location only. IE6 and later
    // must revise this. If no platform value read from registry, get from OS.
    if (hHKCU_UA5Key != INVALID_HANDLE_VALUE && RegQueryValueEx(hHKCU_UA5Key, PLATFORM_KEY_SZ, 
        NULL, &dwType, (LPBYTE) szBuf, &(cbBuf = MAX_PATH)) == ERROR_SUCCESS)
    {
        // Got from HKCU.
        if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, szBuf, cbBuf -1)))
            goto End;
    }
    else if (hHKLM_UA5Key != INVALID_HANDLE_VALUE && RegQueryValueEx(hHKLM_UA5Key, PLATFORM_KEY_SZ, 
             NULL, &dwType, (LPBYTE) szBuf, &(cbBuf = MAX_PATH)) == ERROR_SUCCESS)
    {
        // Got from HKLM
        if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, szBuf, cbBuf -1)))
            goto End;
    }    
    else
    {
        // Couldn't get User Agent value from registry.
        // Set the default value.
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&osVersionInfo))
        {
            // Platform ID is either Win95 or WinNT.
#ifndef unix
            pszWinVer = (osVersionInfo.dwPlatformId 
                         == VER_PLATFORM_WIN32_WINDOWS ? 
                            ((!osVersionInfo.dwMinorVersion) ? szWin95 : szWin98)
                            : ((osVersionInfo.dwMajorVersion==4) ? szWinNT4 : szWinNT5));
#else
            struct utsname uName;
            if(uname(&uName) > -1)
            {
                strcpy(szUnixPlatformName,uName.sysname);
                strcat(szUnixPlatformName," ");
                strcat(szUnixPlatformName,uName.release);
                strcat(szUnixPlatformName," ");
                strcat(szUnixPlatformName,uName.machine);
                strcat(szUnixPlatformName,"; X11");
                pszWinVer = &szUnixPlatformName[0];
            }
            else
                pszWinVer = "Generic Unix";
#endif /* unix */
        }
        else
        {
            // GetVersionEx failed! - set Platform to Win32.
            pszWinVer = szWin32;
        }
        if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, 
            pszWinVer, strlen(pszWinVer))))
            goto End;
    }
    
    // Post platform strings - get from HKCUUA, HKLMUA,
    // HKLMUA5 and HKCUUA5 locations. These are additive;
    // order is not important. Special case the IE4
    // compat token. IE6 and later must do this also.
    for (i = 0; i < NUM_UA_KEYS; i++)
    {
        if (*(phUAKeyArray[i]) == INVALID_HANDLE_VALUE)
            continue;
        
        RegOpenKeyEx(*(phUAKeyArray[i]), 
            POST_PLATFORM_KEY_SZ, 0, KEY_QUERY_VALUE, &hPostKey);
        
        if (hPostKey != INVALID_HANDLE_VALUE)
        {
            dwIndex = 0;
            while (RegEnumValue(hPostKey, dwIndex++, szBuf, &(cbBuf = MAX_PATH), 
                            NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                // We need to special case the IE4 compat mode token in the 
                // first two keys in phUAKeyArray[]; 
                if (cbBuf && ((i > 1) || strcmp(szBuf, COMPAT_MODE_TOKEN)))
                {
                    // Got from registry and non null.
                    if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, "; ", 2)))
                        goto End;
                    if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, szBuf, cbBuf)))
                        goto End;
                }
                cbBuf = MAX_PATH;
            }
     
            // Close post platform key; User agent keys still open.
            RegCloseKey(hPostKey);
            hPostKey = (HKEY) INVALID_HANDLE_VALUE;
        }
    }
    // Terminate with ")\0"
    if (!(bSuccess = AppendToString(&szUA, &cbUA, &cbTotal, ")", 2)))
        goto End;
 
    for (i = 0; i < NUM_UA_KEYS; i++)
    {
        if (*(phUAKeyArray[i]) != INVALID_HANDLE_VALUE)
        {
            RegCloseKey(*(phUAKeyArray[i]));
            *(phUAKeyArray[i]) = (HKEY) INVALID_HANDLE_VALUE;
        }
    }

    // Finally, write out the generated user agent string in the old location.
    if (bSuccess)
    {
        // Remember the computed user agent string for later.
        g_pszUserAgentString = szUA;
    }

End:

    // Cleanup.
    
    delete [] szBuf;

    if (!bSuccess)
    {
        delete [] szUA;
        return DEFAULT_UA_STRING;
    }

    return szUA;
}


//+---------------------------------------------------------------------------
//
//  Method:     ObtainUserAgentString 
//
//  Synopsis:
//
//  Returns:
//
//  History:   08-07-1997   DanpoZ (Danpo Zhang) Created  
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ObtainUserAgentString(DWORD dwOption, LPSTR pszUAOut, DWORD *cbSize )
{
    // since GetUserAgentString may change some global variable, 
    // this API needs to add a global mutex to protect them
    CLock lck(g_mxsSession); 

    HRESULT hr = NOERROR;
    if( pszUAOut && cbSize )
    {
        LPSTR   pcszUA = (LPSTR)GetUserAgentString();
        DWORD   cbLen = strlen(pcszUA);

        if( *cbSize <= cbLen ) 
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            strcpy(pszUAOut, pcszUA);
        } 
        *cbSize = cbLen + 1;
        
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::InternetAuthNotifyCallback
//
//  Synopsis:
//
//  Arguments:  [dwContext] --
//              [dwAction] --
//              [lpReserved] --
//
//  Returns:
//
//  History:    10-10-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CALLBACK CINet::InternetAuthNotifyCallback(DWORD_PTR dwContext, DWORD dwAction, LPVOID lpReserved)
{
    // If this is a request, then we know the cookie type
    CINet *pCINet = (CINet *) dwContext;
    PerfDbgLog2(tagCINet, pCINet, "+CINet::InternetAuthNotifyCallback Action:%ld, State:%ld",
        dwAction, pCINet->_INState);

    PProtAssert((lpReserved == NULL));

    DWORD dwRes = ERROR_SUCCESS;

    switch (dwAction)
    {
    case ERROR_SUCCESS  :
        // should never be returned here
        PProtAssert((FALSE));

    case ERROR_CANCELLED :
        // NOTE: succes and cancel means display the content according to ArthurBi
        // continue reading data and show the 401 contained
        pCINet->_hrINet = NOERROR;
        // Why do we inc the count here??
        pCINet->_fProxyAuth ? pCINet->_cbProxyAuthenticate++ : pCINet->_cbAuthenticate++;
        break;

    case ERROR_INTERNET_RETRY_DIALOG:
        pCINet->_hrINet = INET_E_AUTHENTICATION_REQUIRED;
        break;

    case ERROR_INTERNET_FORCE_RETRY :
        pCINet->_hrINet = RPC_E_RETRY;
        pCINet->_fProxyAuth ? pCINet->_cbProxyAuthenticate++ : pCINet->_cbAuthenticate++;
        break;

    default:
        pCINet->_hrINet = E_ACCESSDENIED;
        break;
    }

    pCINet->OnINetAuthenticate(dwAction);

    PerfDbgLog2(tagCINet, pCINet, "-CINet::InternetAuthNotifyCallback (pCINet->_hrINet:%lx, dwResult:%lx)", pCINet->_hrINet,dwRes);
    return dwRes;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetAuthenticate
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    10-10-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetAuthenticate(DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetAuthenticate");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);
    PProtAssert((_INState == INetState_AUTHENTICATE));

    if (dwResult)
    {
        TransitState(INetState_AUTHENTICATE, TRUE);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetAuthenticate (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetAuthenticate
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-10-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetAuthenticate()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetAuthenticate");
    PProtAssert((GetStatePending() == NOERROR));

    HRESULT hr = NOERROR;
    DWORD dwOperation;

    PProtAssert((GetStatePending() == NOERROR));


    if (_hrINet == INET_E_AUTHENTICATION_REQUIRED)
    {
        // bring up the internet errro dialog
        hr = AuthenticationRequest();

        if ((hr != NOERROR) && (hr != E_PENDING))
        {
            _hrError = INET_E_AUTHENTICATION_REQUIRED;
        }
        else
        {
            _hrError = INET_E_OK;
        }

    }
    
    if (hr == NOERROR)
    {
        if (_hrINet == RPC_E_RETRY)
        {
            // retry the send/request
            _hrINet = NOERROR;
            hr = INetAsyncSendRequest();
        }
        else if (_hrINet == NOERROR)
        {
            hr = QueryStatusOnResponseDefault(0);
            if( hr == NOERROR )
            {
                hr = QueryHeaderOnResponse();
                if (hr == NOERROR)
                {
                    // read more data from wininet
                    hr = INetRead();
                }
            }
        }
        else
        {
            // this will report the hresult return from the dialog
            hr = _hrINet;
        }
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetAuthenticate (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetResumeAsyncRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetResumeAsyncRequest(DWORD dwResultCode)
{
    PerfDbgLog(tagCINet, this, "+CINet::INetResumeAsyncRequest");

    HRESULT hr = NOERROR;
    BOOL fRestarted;
    BOOL fRet;
    PProtAssert((GetStatePending() == NOERROR));

    SetINetState(INetState_SEND_REQUEST);
    {
        SetStatePending(E_PENDING);

        fRet = ResumeSuspendedDownload(_hRequest,
                        dwResultCode
                        );

        if (fRet == FALSE)
        {
            dwLstError = GetLastError();
            if (dwLstError == ERROR_IO_PENDING)
            {
                // wait async for the handle
                hr = E_PENDING;
            }
            else
            {
                SetStatePending(NOERROR);
                hr = _hrError = INET_E_DOWNLOAD_FAILURE;
                SetBindResult(dwLstError,hr);
                PerfDbgLog3(tagCINet, this, "CINet::INetResumeAsyncRequest (fRet:%d, _hrError:%lx, LstError:%ld)", fRet, _hrError, dwLstError);
            }
        }
        else
        {
            SetStatePending(NOERROR);
        }
    }

    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop(hr);
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetResumeAsyncRequest (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetDisplayUI
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    10-10-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetDisplayUI()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetDisplayUI");
    PProtAssert((GetStatePending() == NOERROR));

    HRESULT hr = NOERROR;
    DWORD dwOperation;
    DWORD dwResultCode = ERROR_SUCCESS;

    PProtAssert((GetStatePending() == NOERROR));
    DWORD  dwBindF = GetBindFlags();


    if (_hrINet == INET_E_AUTHENTICATION_REQUIRED)
    {
        HWND hwnd = 0;
        hr = NOERROR;
        // Get a window handle. QueryService on IWindowForBindingUI
        // to get a window object first if necessary.
        if (_pWindow == NULL)
        {
            hr = QueryService(IID_IWindowForBindingUI, (void **) &_pWindow);
        }
        // If we don't already have a window handle, get one from the interface.
        if (!hwnd && _pWindow)
        {
            hr = _pWindow->GetWindow(IID_IHttpSecurity, &hwnd);
            PProtAssert((   (hr == S_FALSE) && (hwnd == NULL)
                         || (hr == S_OK) && (hwnd != NULL)));
        }

        if (hwnd && (hr == S_OK))
        {
            DWORD dwFlags = FLAGS_ERROR_UI_SERIALIZE_DIALOGS;
            if ((dwBindF & BINDF_NO_UI) || (dwBindF & BINDF_SILENTOPERATION))
            {
                dwFlags |= FLAGS_ERROR_UI_FLAGS_NO_UI;
            }

            dwResultCode =
                InternetErrorDlg( hwnd, _hRequest, _dwSendRequestResult, dwFlags, &_lpvExtraSendRequestResult);


            if ( dwResultCode == ERROR_CANCELLED || dwResultCode == ERROR_SUCCESS )               
            {
                // hack-hack alert, if _lpvExtraSendRequestResult non-null we change behavior
                if ( !(dwResultCode == ERROR_SUCCESS && _lpvExtraSendRequestResult != NULL) ) 
                {
                    dwResultCode = ERROR_INTERNET_OPERATION_CANCELLED;
                }
            }

            _hrINet = RPC_E_RETRY;
            hr = NOERROR;
        }

        if ((hr != NOERROR) && (hr != E_PENDING))
        {
            _hrError = INET_E_AUTHENTICATION_REQUIRED;
        }
        else
        {
            _hrError = INET_E_OK;
        }
    }
    
    if (hr == NOERROR)
    {
        if (_hrINet == RPC_E_RETRY)
        {
            // retry the send/request
            _hrINet = NOERROR;

            // hack-arama around to allow CD-ROM dialog to still work
            if ( _dwSendRequestResult == ERROR_INTERNET_INSERT_CDROM ) 
            {
                hr = INetAsyncSendRequest();
            }
            else
            {
                hr = INetResumeAsyncRequest(dwResultCode);
            }
        }
        else
        {
            // this will report the hresult return from the dialog
            hr = _hrINet;
        }
    }

    PerfDbgLog1(tagCINet, this, "-CINet::INetDisplayUI (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetSeek
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//              [ULARGE_INTEGER] --
//              [plibNewPosition] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetSeek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
    PerfDbgLog(tagCINet, this, "+CINet::INetSeek");
    HRESULT hr = E_FAIL;

    // each protocol has to overwrite this method if
    // seek is supported

    DWORD dwResult = InternetSetFilePointer(
                                      _hRequest
                                     ,dlibMove.LowPart
                                     ,0
                                     ,dwOrigin
                                     ,0
                                     );

    PerfDbgLog1(tagCINet, this, "-CINet::INetSeek (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::IsUpLoad
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CINet::IsUpLoad()
{
    BINDINFO *pBndInfo = GetBindInfo();
    return (   (pBndInfo->dwBindVerb != BINDVERB_GET)
            && (pBndInfo->stgmedData.tymed == TYMED_ISTREAM) 
            && !_fCompleted
            && !_fRedirected);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::CPrivUnknown::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINet::CPrivUnknown::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    PerfDbgLog(tagCINet, this, "+CINet::CPrivUnknown::QueryInterface");
    CINet *pCINet = GETPPARENT(this, CINet, _Unknown);

    *ppvObj = NULL;

    if ((riid == IID_IUnknown) || (riid == IID_IOInetProtocol) || (riid == IID_IOInetProtocolRoot) )
    {
        *ppvObj = (IOInetProtocol *) pCINet;
        pCINet->AddRef();
    }
    else if ( ( IsEqualIID(riid, IID_IWinInetInfo)|| 
                IsEqualIID(riid, IID_IWinInetHttpInfo) ) &&
              ( !IsEqualIID(CLSID_FileProtocol, pCINet->_pclsidProtocol) ) )
                    
    {
        *ppvObj = (void FAR *) (IWinInetHttpInfo *)pCINet;
        pCINet->AddRef();
    }
    else if (riid == IID_IOInetThreadSwitch)
    {
        *ppvObj = (IOInetThreadSwitch *)pCINet;
        pCINet->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagCINet, this, "-CINet::CPrivUnknown::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CINet::CPrivUnknown::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CINet::CPrivUnknown::AddRef(void)
{
    PerfDbgLog(tagCINet, this, "+CINet::CPrivUnknown::AddRef");

    LONG lRet = ++_CRefs;

    PerfDbgLog1(tagCINet, this, "-CINet::CPrivUnknown::AddRef (cRefs:%ld)", lRet);
    return lRet;
}
//+---------------------------------------------------------------------------
//
//  Function:   CINet::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CINet::CPrivUnknown::Release(void)
{
    PerfDbgLog(tagCINet, this, "+CINet::CPrivUnknown::Release");

    CINet *pCINet = GETPPARENT(this, CINet, _Unknown);

    LONG lRet = --_CRefs;

    if (lRet == 0)
    {
        delete pCINet;
    }

    PerfDbgLog1(tagCINet, this, "-CINet::CPrivUnknown::Release (cRefs:%ld)", lRet);
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINet::OnINetReadDirect
//
//  Synopsis:
//
//  Arguments:  [dwResult] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::OnINetReadDirect(DWORD dwResult)
{
    PerfDbgLog(tagCINet, this, "+CINet::OnINetReadDirect");
    HRESULT hr = NOERROR;

    PProtAssert((GetStatePending() == E_PENDING));
    // set state to normal - no pending transaction
    SetStatePending(NOERROR);

    if (OperationOnAparmentThread(INetState_SEND_REQUEST))
    {
        TransitState(INetState_READ_DIRECT);
    }
    else
    {
        hr = INetReadDirect();
    }

    PerfDbgLog1(tagCINet, this, "-CINet::OnINetReadDirect (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::INetReadDirect
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::INetReadDirect()
{
    PerfDbgLog(tagCINet, this, "+CINet::INetReadDirect");
    HRESULT hr = NOERROR;
    DWORD dwError;
    ULONG cbBytesAvailable = 1;
    ULONG cbBytesReport = 0;
    SetINetState(INetState_READ_DIRECT);

    //BUGBUG: we hit this assertion sometimes.
    //PProtAssert((GetStatePending() == NOERROR));

    _hrError = INET_E_OK;

    if (!_fFilenameReported)
    {
        char szFilename[MAX_PATH];

        HRESULT hr1 = GetUrlCacheFilename(szFilename, MAX_PATH);

        if (hr1 == NOERROR && szFilename[0] != '\0')
        {
            ReportNotification(BINDSTATUS_CACHEFILENAMEAVAILABLE, (LPSTR) szFilename);
            _fFilenameReported = TRUE;
        }
    }

    if ((GetStatePending() == E_PENDING))
    {
        // nothing to do - data for this notfication
        // already received
        DbgLog(tagCINetErr, this, "CINet::INetReadDirect E_PENIDNG");

    }
    else
    {
        if (   _fDone
            || (   _cbTotalBytesRead
                  && _cbDataSize
                  && (_cbTotalBytesRead == _cbDataSize)))
        {

            _hrError = INET_E_DONE;
            _pEmbdFilter->ReportData(_bscf, cbBytesAvailable, _cbDataSize);
            ReportResultAndStop(NOERROR, _cbTotalBytesRead, _cbDataSize);
            hr = NOERROR;
        }
        else
        {

            if (_bscf & BSCF_DATAFULLYAVAILABLE)
            {
                _bscf |= BSCF_LASTDATANOTIFICATION;
                _bscf &= ~BSCF_FIRSTDATANOTIFICATION;
                cbBytesReport = cbBytesAvailable + _cbTotalBytesRead;
                if (_pEmbdFilter )
                {
                    _pEmbdFilter->ReportData(_bscf, cbBytesReport, _cbDataSize);
                }
                ReportResultAndStop(NOERROR, cbBytesReport, _cbDataSize);
            }
            else
            {
                _bscf |= BSCF_AVAILABLEDATASIZEUNKNOWN;
                cbBytesReport = cbBytesAvailable + _cbTotalBytesRead;
                if (_pCTrans && _pEmbdFilter )
                {
                    _cbSizeLastReportData = cbBytesReport;
                    hr = _pEmbdFilter->ReportData(_bscf, cbBytesReport, _cbDataSize);
                }
            }

            if (_bscf & BSCF_FIRSTDATANOTIFICATION)
            {
                _bscf &= ~BSCF_FIRSTDATANOTIFICATION;
                _bscf |= BSCF_INTERMEDIATEDATANOTIFICATION;
            }
        }
    }

    PerfDbgLog2(tagCINet, this, "-CINet::INetReadDirect (_hrError:%lx, hr:%lx)", _hrError, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::ReadDirect
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBytes] --
//              [pcbBytes] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINet::ReadDirect(BYTE *pBuffer, DWORD cbBytes, DWORD *pcbBytes)
{
    PerfDbgLog(tagCINet, this, "+CINet::ReadDirect");
    HRESULT hr = NOERROR;
    DWORD dwError;

    *pcbBytes = 0;
    ULONG dwReturned = 0;

    //BUGBUG: turn this assertion on again
    //PProtAssert((GetStatePending() == NOERROR));

    if (_hrError == INET_E_DONE)
    {
        // means end of file
        hr = S_FALSE;
    }
    else if (GetStatePending() != NOERROR)
    {
        hr = E_PENDING;
    }
    else
    {
        _hrError = INET_E_OK;
        PProtAssert(((pBuffer != NULL) && (cbBytes > 0)));
        //PerfDbgLog1(tagCINet, this, "CINet::ReadDirect -> InternetReadFile (dwBytesLeft:%ld)", dwBytesLeft);

        LPINTERNET_BUFFERSA pIB = &_inetBufferSend;
        pIB->dwStructSize = sizeof (INTERNET_BUFFERSA);
        pIB->Next = 0;
        pIB->lpcszHeader = 0;
        pIB->dwHeadersLength = 0;
        pIB->dwHeadersTotal = 0;
        pIB->lpvBuffer = pBuffer;
        pIB->dwBufferLength = cbBytes;
        pIB->dwBufferTotal = 0;
        pIB->dwOffsetLow = 0;
        pIB->dwOffsetHigh = 0;


        dwReturned = 0;
        PProtAssert((GetStatePending() == NOERROR));
        SetStatePending(E_PENDING);

        if (!InternetReadFileExA(
                             _hRequest       //IN HINTERNET hFile,
                            ,pIB             // OUT LPINTERNET_BUFFERSA lpBuffersOut,
                            ,IRF_NO_WAIT     //    IN DWORD dwFlags,
                            ,0               //    IN DWORD dwContext
                            ))
        {
            //
            // async completion
            //
            dwError = GetLastError();
            if (dwError != ERROR_IO_PENDING)
            {
                hr = _hrError = INET_E_DOWNLOAD_FAILURE;
                DbgLog3(tagCINetErr, this, "CINet::ReadDirect failed: (dwError:%lx, hr:%lx, hrError:%lx)",
                                            dwError, hr, _hrError);
            }
            else
            {
                hr = E_PENDING;
            }
        }
        else
        {
            //
            // sync completion
            //
            SetStatePending(NOERROR);
            //
            dwReturned = pIB->dwBufferLength;
            _cbTotalBytesRead += dwReturned;
            *pcbBytes = dwReturned;

            PerfDbgLog3(tagCINet, this, "CINet::ReadDirect == InternetReadFileEx ==> (cbBytes:%ld, dwReturned:%ld,_cbTotalBytesRead:%ld)",
                cbBytes, dwReturned,_cbTotalBytesRead);


            if (dwReturned == 0)
            {
                // eof
                hr = _hrError = INET_E_DONE;
                //TransDebugOut((DEB_TRACE, "%p _IN CINetStream::INetSeek\n", this));
                PProtAssert(( (   (_cbDataSize && (_cbDataSize == _cbTotalBytesRead))
                               || (!_cbDataSize)) &&  "WinInet returned incorrent amount of data!!" ));
            }
            else
            {
                hr = NOERROR;
            }

        }  // read case - bits available

        *pcbBytes = dwReturned;

        if (hr == INET_E_DONE)
        {
            hr = (dwReturned) ? S_OK : S_FALSE;
        }
    }

    // Note: stop the download in case of DONE or ERROR!
    if (_hrError != INET_E_OK)
    {
        ReportResultAndStop((hr == S_FALSE) ? NOERROR : hr);
    }

    PerfDbgLog4(tagCINet, this, "-CINet::ReadDirect (_hrError:%lx, [hr:%lx,cbBytesAsked:%ld,cbBytesReturned:%ld])",
        _hrError, hr, cbBytes, *pcbBytes);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::GetUserAgentString
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBytes] --
//              [pcbBytes] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR CINet::GetUserAgentString()
{
    PerfDbgLog(tagCINet, this, "+CINet::GetUserAgentString");
    HRESULT hr = NOERROR;
    ULONG ulCount = 0;
    LPWSTR pwzStr = 0;
    LPSTR pszStr = 0;
    

    hr = _pOIBindInfo->GetBindString(BINDSTRING_USER_AGENT, (LPWSTR *)&pwzStr, 1, &ulCount);

    if ((hr == NOERROR) && ulCount)
    {
        PProtAssert((pwzStr));
        delete _pszUserAgentStr;
        pszStr = _pszUserAgentStr  = DupW2A(pwzStr); 
    }
    else
    {
        pszStr = (LPSTR)::GetUserAgentString();
    }

    
    PerfDbgLog1(tagCINet, this, "-CINet::GetUserAgentString (pszStr:%s)", pszStr);
    return pszStr;
}


BOOL GlobalUTF8Enabled()
{
    // read the IE5 B2 global reg key
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  fRet = FALSE;
    HKEY  hKeyClient;
    DWORD dwUTF8 = 0;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;

    dwErr = RegOpenKeyEx(
                HKEY_CURRENT_USER, 
                INTERNET_POLICIES_KEY,
                0,
                KEY_QUERY_VALUE,
                &hKeyClient
            );
    if( dwErr == ERROR_SUCCESS )
    {
        dwErr = RegQueryValueEx(
                hKeyClient, 
                "EnableUTF8",
                0,  
                &dwType, 
                (LPBYTE)&dwUTF8, 
                &dwSize
        );
        
        if( dwErr == ERROR_SUCCESS && dwUTF8 )
        {
            fRet = TRUE;
        }
        RegCloseKey(hKeyClient);
    }

    return fRet;
}
