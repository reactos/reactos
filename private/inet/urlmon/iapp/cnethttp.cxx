//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cnethttp.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <iapp.h>
#include "shlwapip.h"
#ifndef unix
#include "..\trans\transact.hxx"
#include "..\trans\oinet.hxx"
#else
#include "../trans/transact.hxx"
#include "../trans/oinet.hxx"
#endif /* unix */

PerfDbgTag(tagCINetHttp, "Urlmon", "Log CINetHttp", DEB_PROT);

extern LPSTR g_pszUAInfoString;
static CHAR  gszAcceptEncHeaders[] = "Accept-Encoding: gzip, deflate";

// http specifics
static char vszGet[]    = "GET";
static char vszPost[]   = "POST";
static char vszPut[]    = "PUT";
static char vszAttachment[] = "attachment";
static DWORD dwLstError;
DWORD GetRedirectSetting();

// list of content-type we should not apply content-encoding onto it.
static LPSTR  vszIgnoreContentEnc[] =
{
     "application/x-tar"
    ,"x-world/x-vrml"
    ,"application/zip"
    ,"application/x-gzip"
    ,"application/x-zip-compressed"
    ,"application/x-compress"
    ,"application/x-compressed"
    ,"application/x-spoon"
    , 0
};

BOOL IgnoreContentEncoding(LPSTR szContentType, LPSTR szEnc, LPSTR szAccept)
{
    BOOL bRet = FALSE;

    if( szEnc && szAccept && !StrStrI(szAccept, szEnc) )
    {
        //
        // some of the old web server will ignore the schemas indicated at
        // Accept-Endocing: header, we need to add another check here
        // to make sure the server returned content-encoding is the
        // one we supported, otherwise, we will not init the decoder
        //
        bRet = TRUE;
    }

    if( !bRet )
    {
        for( int i = 0; vszIgnoreContentEnc[i]; i++)
        {
            if(!StrCmpI(szContentType, vszIgnoreContentEnc[i]) )
            {
                bRet = TRUE;
                break;
            }
        }
    }

    return bRet;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetRedirectSetting
//
//  Synopsis:   Reads the registry UrlMon Setting of Redirect
//
//  Arguments:  (none)
//
//  Returns:    0 if redirect should be done by WinINet,
//              1 if should be done by UrlMon
//
//  History:    4-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD GetRedirectSetting()
{
    HKEY hUrlMonKey = NULL;
    DWORD dwType;
    static DWORD dwValue = 0xffffffff;

    if (dwValue == 0xffffffff)
    {
        DWORD dwValueLen = sizeof(DWORD);
        dwValue = 0;

        #define szUrlMonKey "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UrlMon Settings"
        #define szRedirect  "Redirect"

        if (RegOpenKeyEx(HKEY_CURRENT_USER, szUrlMonKey, 0, KEY_QUERY_VALUE, &hUrlMonKey) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(hUrlMonKey, szRedirect, NULL, &dwType, (LPBYTE)&dwValue, &dwValueLen) != ERROR_SUCCESS)
            {
                dwValue = 0;
            }
            RegCloseKey(hUrlMonKey);
        }
    }

    return dwValue;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::CINetHttp
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetHttp::CINetHttp(REFCLSID rclsid, IUnknown *pUnkOuter) : CINet(rclsid,pUnkOuter)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::CINetHttp");

    _pHttpNeg = NULL;
    _dwIsA = DLD_PROTOCOL_HTTP;
    _dwBufferSize = 0;
    _pBuffer = 0;
    _pszVerb = 0;
    _f2ndCacheKeySet = FALSE;


    PerfDbgLog(tagCINetHttp, this, "-CINetHttp::CINetHttp");
}

//----------------------------------------------------------------------------
//
//  Method:     CINetHttp::~CINetHttp
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetHttp::~CINetHttp()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::~CINetHttp");
    PProtAssert(( !_hRequest ));
    PProtAssert(( !_hServer ));

    delete [] _pBuffer;
    delete [] _pszHeader;
    delete [] _pszSendHeader;
    delete [] _pwzAddHeader;
    delete [] _pszVerb;

    PProtAssert((_pHttpNeg == NULL));
    PProtAssert((_pWindow == NULL));
    PProtAssert(( _pHttSecurity == NULL));

    PerfDbgLog(tagCINetHttp, this, "-CINetHttp::~CINetHttp");
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::INetAsyncOpenRequest
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
HRESULT CINetHttp::INetAsyncOpenRequest()
{
    PerfDbgLog1(tagCINetHttp, this, "+CINetHttp::INetAsyncOpenRequest (_szObject:%s)", GetObjectName());
    HRESULT hr = NOERROR;
    DWORD dwBindF = 0;
    const ULONG culSize = 256;
    ULONG ulSize = culSize;
    LPCSTR rgszAcceptStr[culSize] = { 0 };
    LPWSTR rgwzStr[culSize];

    SetINetState(INetState_PROTOPEN_REQUEST);
    PProtAssert((g_hSession != NULL));
    PProtAssert((GetStatePending() == NOERROR));

    if (_pOIBindInfo)
    {
        LPWSTR *pwzStr;
        IEnumString *pEnumString = NULL;
        ULONG ulCount = culSize;

        hr = _pOIBindInfo->GetBindString(BINDSTRING_ACCEPT_MIMES, (LPWSTR *)rgwzStr, ulSize, &ulCount);

        if (hr == NOERROR)
        {
            ULONG c = 0;

            for (c = 0; c < ulCount; c++)
            {
                rgszAcceptStr[c] = (LPCSTR) DupW2A(rgwzStr[c]);
                delete rgwzStr[c];
                rgwzStr[c] = 0;
            }
            rgszAcceptStr[c] = 0;
        }
        else
        if( hr == INET_E_USE_DEFAULT_SETTING )
        {
            rgszAcceptStr[0] = (LPCSTR) DupW2A(L"*/*");
            rgszAcceptStr[1] = NULL;
            hr = NOERROR;
        }

    }
    if (hr != NOERROR)
    {
        hr = INET_E_NO_VALID_MEDIA;
        _hrError = INET_E_NO_VALID_MEDIA;
    }
    else if (!_hServer)
    {
        // the download was probably aborted
        if (_hrError == NOERROR)
        {
            SetBindResult(ERROR_INVALID_HANDLE, hr);
            hr = _hrError = INET_E_CANNOT_CONNECT;
        }
        else
        {
            hr = _hrError;
        }
    }
    else
    {
        PProtAssert((_hServer));

        //PProtAssert((ppszAcceptStr && *ppszAcceptStr));

        #if DBG==1
        {
            LPSTR *pszMime = (LPSTR *) &rgszAcceptStr;
            while (*pszMime)
            {
                PerfDbgLog1(tagCINetHttp, this, "=== CTransData::GetAcceptStr (szMime:%s)", *pszMime);
                pszMime++;
            }
        }
        #endif


        dwBindF = GetBindFlags();

        if (dwBindF & BINDF_IGNORESECURITYPROBLEM)
        {
            _dwOpenFlags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
            _dwOpenFlags |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
            _dwOpenFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS;
            _dwOpenFlags |= INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP;
        }

        if (GetRedirectSetting() != 0)
        {
            DbgLog(tagCINetHttp, this, "=== CINet::INetAsyncOpenRequest redirect done by UrlMon!");
            _dwOpenFlags |= INTERNET_FLAG_NO_AUTO_REDIRECT;
        }
        else
        {
            DbgLog(tagCINetHttp, this, "=== CINet::INetAsyncOpenRequest redirect done by WinINet!");
        }

        //
        // we always request keep-alive
        //
        _dwOpenFlags |= INTERNET_FLAG_KEEP_CONNECTION;

        //
        // Notify wininet if this is a multipart upload so it doesn't
        // add a terminating 0x0d 0x0a to the first send
        //
        if (IsUpLoad())
        {
            _dwOpenFlags |= INTERNET_FLAG_NO_AUTO_REDIRECT;
            //BUGBUG: is the flag below needed?
            //_dwOpenFlags |= INTERNET_FLAG_MULTIPART;
        }



        PrivAddRef(TRUE);
        SetStatePending(E_PENDING);

        _HandleStateRequest = HandleState_Pending;
        HINTERNET hRequestTmp = HttpOpenRequest(
                        _hServer,                   // hHttpSession
                        GetVerb(),                  // lpszVerb
                        GetObjectName(),            // lpszObjectName
                        NULL, //HTTP_VERSION,       // lpszVersion
                        NULL,                       // lpszReferer
                        rgszAcceptStr,              // lplpszAcceptTypes
                        _dwOpenFlags,               // flag
                        (DWORD_PTR) this            // context
                        );
        if ( hRequestTmp == 0)
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
                hr = _hrError = INET_E_RESOURCE_NOT_FOUND;
                SetBindResult(dwLstError,hr);
            }
        }
        else
        {
            _hRequest = hRequestTmp; 
            SetStatePending(NOERROR);
            _HandleStateRequest = HandleState_Initialized;
            hr = INetAsyncSendRequest();
        }

        {
            LPSTR  *pszMime = (LPSTR *) &rgszAcceptStr;
            while (*pszMime)
            {
                LPSTR pszDel = *pszMime;
                delete pszDel;
                pszMime++;
            }
        }
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::INetAsyncOpenRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::INetAsyncSendRequest
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
HRESULT CINetHttp::INetAsyncSendRequest()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::INetAsyncSendRequest");

    HRESULT hr = NOERROR;
    BOOL fRestarted;
    BOOL fRet;
    PProtAssert((GetStatePending() == NOERROR));

    SetINetState(INetState_SEND_REQUEST);

    LPVOID pBuffer = 0;
    DWORD  dwBufferSize = 0;
    LPSTR szVerb = GetVerb();

    //
    // BUGBUG: move this into GetAdditionalHeader
    //
    if (_fRedirected == TRUE || (_cbProxyAuthenticate + _cbAuthenticate))
    {
        if (_pszSendHeader)
        {
            delete _pszSendHeader;
            _pszSendHeader = NULL;
        }

    }

    if (_fRedirected == FALSE && !(_cbProxyAuthenticate + _cbAuthenticate))
    {
        GetAdditionalHeader();
    }

    if (_fRedirected == FALSE || _fP2PRedirected )
    {
        // Note: the buffer returned here will be freed
        // by the destructor
        GetDataToSend(&pBuffer, &dwBufferSize);
        _fP2PRedirected = FALSE;
    }

    // Call HttpNeg only the first time in case of authentication
    // i.e. both Auth counts == 0 ?
    if (!(_cbAuthenticate || _cbProxyAuthenticate))     {
        //BUGBUG: does BeginingTrans need to be called for
        // redirect and authentication resends?
        hr = HttpNegBeginningTransaction();
        // Note: the header is appended to the AddHeader
    }

    if (hr == E_ABORT)
    {
        _hrError = hr;
        SetBindResult(ERROR_CANCELLED,hr);
    }
    else
    {
        PerfDbgLog1(tagCINetHttp, this, "+CINetHttp::INetAsyncSendRequest HttpSendRequest (_pszSendHeader:%s)", XDBG(_pszSendHeader,""));

        if (GetBindInfo()->dwBindVerb == BINDVERB_POST && !_f2ndCacheKeySet)
        {
            // WININET request: send SECONDARY_CACHE_KEY only once
            ULONG ulCount = 0;
            LPWSTR pwzPostCookieStr = 0;
            HRESULT hr1 = _pOIBindInfo->GetBindString(BINDSTRING_POST_COOKIE, (LPWSTR *)&pwzPostCookieStr, 1, &ulCount);
            if ((hr1 == NOERROR) && pwzPostCookieStr)
            {
                // BUGBUG: trident return s_ok and no string
                PProtAssert((pwzPostCookieStr));
                LPSTR pszStr = DupW2A(pwzPostCookieStr);
                if (pszStr)
                {
                    _f2ndCacheKeySet = TRUE;
                    InternetSetOption(_hRequest, INTERNET_OPTION_SECONDARY_CACHE_KEY, pszStr, strlen(pszStr));
                    delete pszStr;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    _hrError = INET_E_DOWNLOAD_FAILURE;
                    SetBindResult(hr,hr);
                }
                delete pwzPostCookieStr;
            }

        }

        /************ enable this after wininet sniff checked in *******
        // set option for data sniff
        DWORD dwDataSniff = 1;
        InternetSetOption(_hRequest, INTERNET_OPTION_DATASNIFF, &dwDataSniff, sizeof(DWORD));
        ****************************************************************/

        InternetSetOption(_hRequest, INTERNET_OPTION_REQUEST_PRIORITY, &_nPriority, sizeof(LONG));

        InternetSetOption(_hRequest, INTERNET_OPTION_CODEPAGE, &_BndInfo.dwCodePage, sizeof(DWORD));

        SetStatePending(E_PENDING);

        if (hr == NOERROR)
        {
            if (IsUpLoad())
            {
                // post verb
                // DWORD dwSendFlags = HSR_ASYNC | HSR_CHUNKED | HSR_INITIATE;
                DWORD dwSendFlags = HSR_CHUNKED | HSR_INITIATE;
                DWORD dwSendContext = 0;

                if (!_pStm)
                {
                    BINDINFO *pBI = GetBindInfo();
                    if (pBI && pBI->stgmedData.tymed == TYMED_ISTREAM)
                            {
                        _pStm = pBI->stgmedData.pstm;
                    }
                }
                if (_pStm)
                {
                    hr = GetNextSendBuffer(&_inetBufferSend,_pStm, TRUE);
                }

                fRet = HttpSendRequestExA(
                          _hRequest               // IN HINTERNET hRequest,
                         ,&_inetBufferSend         // IN LPINTERNET_BUFFERSA lpBuffersIn OPTIONAL,
                         , NULL                   // OUT lpBuffersOut not used
                         ,dwSendFlags             // IN DWORD dwFlags,
                         ,dwSendContext           // IN DWORD dwContext
                            );
            }
            else
            {
                DWORD dwError;

                // Allow ERROR_INTERNET_INSERT_CDROM to be returned from HttpSendRequest
#ifdef MSNJIT
                DWORD dwErrorMask = INTERNET_ERROR_MASK_INSERT_CDROM | INTERNET_ERROR_MASK_COMBINED_SEC_CERT | INTERNET_ERROR_MASK_NEED_MSN_SSPI_PKG;
#else                
                DWORD dwErrorMask = INTERNET_ERROR_MASK_INSERT_CDROM | INTERNET_ERROR_MASK_COMBINED_SEC_CERT;
#endif                
                dwErrorMask = dwErrorMask | INTERNET_ERROR_MASK_LOGIN_FAILURE_DISPLAY_ENTITY_BODY;

                InternetSetOption(_hRequest, INTERNET_OPTION_ERROR_MASK, &dwErrorMask, sizeof(DWORD));

                fRet = HttpSendRequest(_hRequest,
                       _pszSendHeader,                     // additional headers
                       (_pszSendHeader) ? (ULONG)-1L : 0L, // size of additional headers data
                       pBuffer,                            // Optional data (POST or put)
                       dwBufferSize);                      // optional data length

                PerfDbgLog(tagCINetHttp, this, "-CINetHttp::INetAsyncSendRequest HttpSendRequest");

            }   // end else

            if (fRet == FALSE)
            {
                dwLstError = GetLastError();
                if (dwLstError == ERROR_IO_PENDING)
                {
                    // wait async for the handle
                    hr = E_PENDING;
                }
                else if (dwLstError == ERROR_INTERNET_INSERT_CDROM)
                {
                    _hrINet = INET_E_AUTHENTICATION_REQUIRED;
                    _dwSendRequestResult = ERROR_INTERNET_INSERT_CDROM;
                    _lpvExtraSendRequestResult = NULL;
                    TransitState(INetState_DISPLAY_UI, TRUE);
                    hr = E_PENDING;
                    fRet = TRUE;
                }
                else
                {
                    SetStatePending(NOERROR);
                    hr = _hrError = INET_E_DOWNLOAD_FAILURE;
                    SetBindResult(dwLstError,hr);
                    PerfDbgLog3(tagCINetHttp, this, "CINetHttp::INetAsyncSendRequest (fRet:%d, _hrError:%lx, LstError:%ld)", fRet, _hrError, dwLstError);
                }
            }
            else
            {
                SetStatePending(NOERROR);

                // in case of redirect, we need to reset all the
                // _dwSendRequestResult from previous callback
                _dwSendRequestResult = 0;
                _lpvExtraSendRequestResult = NULL;
                hr = INetQueryInfo();
            }
        }
    }


    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop(hr);
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::INetAsyncSendRequest (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::QueryStatusOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::QueryStatusOnResponse()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::QueryStatusOnResponse");
    HRESULT hr = NOERROR;

    DWORD dwStatus;
    char szBuffer[max(2 * MAX_URL_SIZE, 400)];
    DWORD cbBufferLen = sizeof(szBuffer);
    DWORD cbLen = cbBufferLen;

    if (_dwSendRequestResult)
    {
        // handle the sendrequest result
        // zone crossing
        switch (_dwSendRequestResult)
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
        case ERROR_INTERNET_SEC_CERT_ERRORS           :
        case ERROR_INTERNET_SEC_CERT_REV_FAILED       :
        case ERROR_INTERNET_SEC_CERT_REVOKED          :
        {
            hr = HttpSecurity(_dwSendRequestResult);

            if ((hr != NOERROR) && (hr != E_PENDING))
            {
                _hrError = INET_E_AUTHENTICATION_REQUIRED;
            }
            else
            {
                _hrError = INET_E_OK;
            }

        }
        break;

        case ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY :
        {
            _hrError = INET_E_OK;
            hr = NOERROR; 
        }
        break;

#ifdef MSNJIT        
        case ERROR_INTERNET_NEED_MSN_SSPI_PKG     :
        {
            const GUID MSN_AUTH_GUID = 
            { 0x6fab99d0, 0xbab8, 0x11d1, {0x99, 0x4a, 0x00, 0xc0, 0x4f, 0x98, 0xbb, 0xc9} };

            HWND hWnd = NULL;
            DWORD dwJITFlags = 0;
            
            uCLSSPEC classpec;
            classpec.tyspec=TYSPEC_TYPELIB;
            classpec.tagged_union.typelibID = (GUID)MSN_AUTH_GUID;

            QUERYCONTEXT qc;
            memset(&qc, 0, sizeof(qc));

            // fill in the minimum version number of the component you need
            //qc.dwVersionHi = 
            //qc.dwVersionLo = 
            hr = FaultInIEFeature(hWnd, &classpec, &qc, dwJITFlags);

            if (hr == S_OK)
            {
                hr = INET_E_AUTHENTICATION_REQUIRED;
            }
            else 
            {
                hr = E_ABORT;
            }
        }

        break;
#endif
        default:
        break;
        }

    }
    else if (HttpQueryInfo(_hRequest, HTTP_QUERY_STATUS_CODE, szBuffer,&cbLen, NULL))
    {
        dwStatus = atoi(szBuffer);
        _fProxyAuth = FALSE;
        switch (dwStatus)
        {
        case HTTP_STATUS_DENIED:
        {
            _hrINet = INET_E_AUTHENTICATION_REQUIRED;
            TransitState(INetState_AUTHENTICATE, TRUE);
            hr = E_PENDING;
        }
        break;
        case HTTP_STATUS_PROXY_AUTH_REQ :
        {
            _hrINet = INET_E_AUTHENTICATION_REQUIRED;
            TransitState(INetState_AUTHENTICATE, TRUE);
            _fProxyAuth = TRUE;
            hr = E_PENDING;
        }
        break;
        case HTTP_STATUS_MOVED:
        case HTTP_STATUS_REDIRECT:
        case HTTP_STATUS_REDIRECT_METHOD:
        case HTTP_STATUS_REDIRECT_KEEP_VERB:
        {
            cbLen = cbBufferLen;
            hr = RedirectRequest(szBuffer, &cbLen);
            if ((hr != S_FALSE) &&  (hr != NOERROR) && (hr != E_PENDING))
            {
                _hrError = INET_E_INVALID_URL;
            }
            else
            {
                _hrError = INET_E_OK;
                hr = S_FALSE;
                SetINetState(INetState_DONE);
            }
        }
        break;
        case HTTP_STATUS_NO_CONTENT:
        {
	        BINDINFO *pBndInfo = GetBindInfo();

    	    if (pBndInfo && pBndInfo->dwBindVerb != BINDVERB_CUSTOM)
        	{
            hr = _hrError = E_ABORT;
            SetBindResult(ERROR_CANCELLED, hr);
            }
            else
            {
            hr = QueryStatusOnResponseDefault(dwStatus);
            }
        }
        break;
        default:
        {
            hr = QueryStatusOnResponseDefault(dwStatus);
        }
        break;
        }
    }

    if (_hrError != INET_E_OK)
    {
        SetINetState(INetState_DONE);
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::QueryStatusOnResponse (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::QueryStatusOnResponseDefault
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::QueryStatusOnResponseDefault(DWORD dwStat)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::QueryStatusOnResponseDefault");
    HRESULT hr = NOERROR;

    DWORD dwStatus = 0;
    char szBuffer[max(2 * MAX_URL_SIZE, 400)];
    DWORD cbBufferLen = sizeof(szBuffer);
    DWORD cbLen = cbBufferLen;

    if( !dwStat )
    {
        if (HttpQueryInfo(_hRequest, HTTP_QUERY_STATUS_CODE, szBuffer,&cbLen, NULL))
        {
            dwStatus = atoi(szBuffer);
        }
    }
    else
    {
        dwStatus = dwStat;
    }

    if( dwStatus )
    {
        #if DBG==1
        if ( !((dwStatus >= HTTP_STATUS_OK) && (dwStatus <= HTTP_STATUS_GATEWAY_TIMEOUT)) )
        {
            DbgLog1(DEB_PROT|DEB_TRACE, this, "CINetHttp::QueryStatusOnResponse (dwStatus:%lx)", dwStatus);
        }
        PProtAssert((   (dwStatus >= HTTP_STATUS_BEGIN) && (dwStatus <= HTTP_STATUS_END)
                         && L"WinINet returned an invalid status code: please contact a WININET developer" ));
        #endif //DBG==1

        // check if we got redirected from a file to a directory
        {
            cbLen = cbBufferLen;
            InternetQueryOption(_hRequest, INTERNET_OPTION_URL, szBuffer, &cbLen);
            if (cbLen)
            {
                BOOL fRedirected;
                fRedirected = strcmp(szBuffer, _pszFullURL);

                if (fRedirected)
                {
                    hr = RedirectRequest(szBuffer, &cbLen);

                    if ((hr != NOERROR) && (hr != E_PENDING))
                    {
                        if (hr != INET_E_DOWNLOAD_FAILURE)
                        {
                            _hrError = INET_E_INVALID_URL;
                        }
                    // else set nothing
                    }
                    else
                    {
                        _hrError = INET_E_OK;
                    }
                }
            }
        }

        cbLen = cbBufferLen;
        if (HttpQueryInfo(_hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, szBuffer,&cbLen, NULL))
        {
            if (IsStatusOk(dwStatus))
            {
                hr = HttpNegOnHeadersAvailable(dwStatus,szBuffer);
            }
            else
            {
                hr = ErrorHandlingRequest(dwStatus, szBuffer);
                if ((hr != NOERROR) && (hr != E_PENDING))
                {
                    _hrError = hr;
                }
                else
                {
                    _hrError = INET_E_OK;
                }
            }

            if (hr == E_ABORT)
            {
                SetBindResult(ERROR_CANCELLED,hr);
            }
        }
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::QueryStatusOnResponseDefault (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::QueryHeaderOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::QueryHeaderOnResponse()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::QueryHeaderOnResponse");
    HRESULT hr = NOERROR;
    DWORD dwStatus;
    char szBuffer[max(2 * MAX_URL_SIZE, 400)];
    DWORD cbBufferLen = sizeof(szBuffer);
    DWORD dwFlags;
    char szMIMEType[SZMIMESIZE_MAX] = "";
    char szENCType[SZMIMESIZE_MAX] = "";

    // Get file length
    if(HttpQueryInfo(_hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuffer,&cbBufferLen, NULL))
    {
        _cbDataSize = atoi(szBuffer);
    }

    // Get Content-Disposition
    szBuffer[0] = '\0';
    cbBufferLen = sizeof(szBuffer);

    if(HttpQueryInfo(
            _hRequest,
            HTTP_QUERY_CONTENT_DISPOSITION,
            szBuffer,
            &cbBufferLen,
            NULL) )
    {
        // Search for :Attachment, if found, report it
        if( StrStrI(szBuffer, vszAttachment) )
        {
            ReportNotification(BINDSTATUS_CONTENTDISPOSITIONATTACH, NULL);
        }
    }

    // Get Accept-Ranges 
    szBuffer[0] = '\0';
    cbBufferLen = sizeof(szBuffer);
    if(HttpQueryInfo(
            _hRequest,
            HTTP_QUERY_ACCEPT_RANGES,
            szBuffer,
            &cbBufferLen,
            NULL) )
    {
        ReportNotification(BINDSTATUS_ACCEPTRANGES, NULL);
    }

    // mimetype
    cbBufferLen = sizeof(szMIMEType);
    szMIMEType[0] = 0;
    HttpQueryInfo(_hRequest, HTTP_QUERY_CONTENT_TYPE, szMIMEType, &cbBufferLen, NULL);
    if (cbBufferLen && (szMIMEType[0] != 0))
    {
        //BUG-WORK
        //_pCTransData->SetMimeType(szMIMEType);
        //_pCTrans->ReportProgress();
        DbgLog1(DEB_PROT|DEB_TRACE, this,
                "CINetHttp::QueryHeaderOnResponse MIME TYPE(szMime:%s)!",
                szMIMEType);

        // work around image display problem, turn off report mime type
        // for direct binding
        if( _grfBindF & BINDF_FROMURLMON)
        {
            ReportNotification(BINDSTATUS_MIMETYPEAVAILABLE,szMIMEType);
        }
        else
        {
            ReportNotification(BINDSTATUS_RAWMIMETYPE, szMIMEType);
        }

        /*** enable this block after wininet data sniff checked in ****
        if( _cbDataSize )
        {

            // datasniff enabled ?
            DWORD dwDataSniff = 0;
            DWORD dwSize = sizeof(DWORD);

            if( InternetQueryOption(
                    _hRequest,
                    INTERNET_OPTION_DATASNIFF,
                    &dwDataSniff,
                    &dwSize ) )

            {
                char szVCType[SZMIMESIZE_MAX] = "";
                cbBufferLen = SZMIMESIZE_MAX;

                InternetQueryOption(
                    _hRequest,
                    INTERNET_OPTION_VERIFIED_CONTENT_TYPE,
                    szVCType,
                    &cbBufferLen );

            }
        }
        ****************************************************************/
    }
    else
    {
        DbgLog1(DEB_PROT|DEB_TRACE, this,
                "CINetHttp::QueryHeaderOnResponse NO MIME TYPE (szUrl:%s)!",
                GetBaseURL());
        //BUGBUG: need data sniffing later on
        //_pCTransData->SetMimeType("text/html");
        // work around image display problem, turn off report mime type
        // for direct binding
        if( _grfBindF & BINDF_FROMURLMON)
        {
            ReportNotification(BINDSTATUS_MIMETYPEAVAILABLE,"text/html");
        }
    }

    // content encoding
    HttpQueryInfo(_hRequest, HTTP_QUERY_CONTENT_ENCODING, szENCType, &cbBufferLen, NULL);
    if (cbBufferLen && (szENCType[0] != 0))
    {
        DbgLog1(DEB_PROT|DEB_TRACE, this,
                "CINetHttp::QueryHeaderOnResponse ENCODING TYPE(szEnc :%s)!",
                szENCType);
        //
        // existing http servers may mishandle the content-encoding
        // header,  we have to taken care of the following cases:
        //
        // 1. we do not send Accept-Encoding header(http1.1 disabled),
        //    however, server sends back Content-Encoding: foo
        //    (_pszHeaders contains the accept-encoding info, so if
        //     this is null, we should not invoke the decoder )
        //
        // 2. we send Accept-Encoding: A, server sends back
        //    Content-Encoding: B, this is a protocol violation
        //    IgnoreContentEncoding() takes care of that, it compares
        //    the _pszHeader and szENCType, and we should not invoke
        //    the decoder if they are mis-matched
        //
        // 3. server sends out content-encoding, but what they really
        //    mean is that let the application (e.g. gunzip.exe) to
        //    handle the compressed file, we can add the check for
        //    content-type, for a list of content-type we do not
        //    understand (e.g. application/x-compress, x-world/x-vrml..)
        //    do not invoke the decoder
        //
        if( _pszHeader &&
            !IgnoreContentEncoding(szMIMEType, szENCType, _pszHeader) )
        {
            ReportNotification(BINDSTATUS_ENCODING, szENCType);

            // Load The decompression handler now...
            COInetSession       *pCOInetSession = NULL;
            IOInetProtocol      *pProtHandler = NULL;
            IOInetProtocolSink  *pProtSnkHandler = NULL;
            IOInetBindInfo      *pBindInfo = NULL;
            LPWSTR              pwzStr = DupA2W(szENCType);
            CLSID               clsid;

            hr = GetCOInetSession(0,&pCOInetSession,0);
            if( hr == NOERROR )
            {
                hr = pCOInetSession->CreateHandler(
                    pwzStr, 0, 0, &pProtHandler, &clsid);

                if( hr == NOERROR )
                {
                    hr = pProtHandler->QueryInterface(
                        IID_IOInetProtocolSink, (void **) &pProtSnkHandler);


                    //hr = QueryInterface(
                    //    IID_IOInetBindInfo, (void **) &pBindInfo);
                }

                if( hr == NOERROR )
                {
                    HRESULT hr2 = NOERROR;
                    hr2 = _pEmbdFilter->SwitchSink(pProtSnkHandler);
                    if( hr2 == NOERROR )
                    {
                        hr = _pEmbdFilter->StackFilter(
                            pwzStr, pProtHandler, pProtSnkHandler, _pOIBindInfo );
                    }
                }

                if( pBindInfo )
                {
                    pBindInfo->Release();
                }
            }

            if( szMIMEType[0] != '\0' )
            {
                ReportNotification(BINDSTATUS_MIMETYPEAVAILABLE,szMIMEType);
            }

            // urlmon will create a decompressed cache-file, so we should not
            // report the compressed file name to the client.
            _fFilenameReported = TRUE;
        }
    }

#ifdef TEST_STACK_FILTER_ONE
    //test for stackable filter..
    //need to include mft.hxx for sample implementation of the filter
    {
        IOInetProtocol* pFilter = (IOInetProtocol*) new CMft;
        IOInetProtocolSink* pFilterSink = NULL;
        pFilter->QueryInterface(
            IID_IOInetProtocolSink, (void**)&pFilterSink);

        // connect the last filter sink with pFilter's Sink
        HRESULT hr2 = NOERROR;
        hr2 = _pEmbdFilter->SwitchSink(pFilterSink);

        if( hr2 == NOERROR )
        {
            hr = _pEmbdFilter->StackFilter(NULL, pFilter, NULL, NULL);
        }

        // this object gets created here, pFilter gets AddRef'd during
        // the StackFilter(), we should release the additional Ref Count
        // here
        // this does not apply to the first filter stacked
        if( _pEmbdFilter->FilterStacked() > 1 )
        {
            pFilter->Release();
        }
    }
#endif

#ifdef TEST_STACK_FILTER_TWO
    {
        // another one...
        IOInetProtocol* pFilter_2 = (IOInetProtocol*) new CMft;
        IOInetProtocolSink* pFilterSink_2 = NULL;
        pFilter_2->QueryInterface(
            IID_IOInetProtocolSink, (void**)&pFilterSink_2);

        // connect the last filter sink with pFilter's Sink
        HRESULT hr3 = NOERROR;
        hr3 = _pEmbdFilter->SwitchSink(pFilterSink_2);

        if( hr3 == NOERROR )
        {
            hr = _pEmbdFilter->StackFilter(NULL, pFilter_2, NULL, NULL);
        }

        // this object gets created here, pFilter gets AddRef'd during
        // the StackFilter(), we should release the additional Ref Count
        // here
        // this does not apply to the first filter stacked
        if( _pEmbdFilter->FilterStacked() > 1 )
        {
            pFilter_2->Release();
        }
    }
#endif

    if (_hrError != INET_E_OK)
    {
        SetINetState(INetState_DONE);
        hr = S_FALSE;
    }

    PerfDbgLog2(tagCINetHttp, this, "-CINetHttp::QueryHeaderOnResponse (hr:%lx, _cbDataSize:%ld)", hr, _cbDataSize);
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::RedirectRequest
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
HRESULT CINetHttp::RedirectRequest(LPSTR lpszBuffer, DWORD *pdwBuffSize )
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::RedirectRequest");
    HRESULT hr = INET_E_DOWNLOAD_FAILURE;

    char *pszHeader;
    DWORD cbBufferLen;
    // we assume when we get here that we have recieved a redirection
    // now we are going to see where we need to do the next send

    cbBufferLen = *pdwBuffSize;

    if (cbBufferLen <= strlen(vszLocationTag))
    {
        goto End;
    }
    strcpy(lpszBuffer, vszLocationTag);


    if (HttpQueryInfo(_hRequest, HTTP_QUERY_RAW_HEADERS, lpszBuffer,&cbBufferLen, NULL))
    {
        LPSTR  pszRedirect = 0;
        pszHeader = FindTagInHeader(lpszBuffer, vszLocationTag);
        if (!pszHeader)
        {
            goto End;
        }

        //
        // _pszPartURL get allocated here!
        //
        if( _pszPartURL )
        {
            delete [] _pszPartURL;
            _pszPartURL = NULL;
        }

        DWORD dwPartUrlLen = strlen( (pszHeader + strlen(vszLocationTag) ) );
        if( dwPartUrlLen > MAX_URL_SIZE)
        {
            hr = INET_E_DOWNLOAD_FAILURE;
            goto End;
        }

        _pszPartURL = new char[dwPartUrlLen + 1];
        if( !_pszPartURL )
        {
            hr = E_OUTOFMEMORY;
            goto End;
        }

        strcpy(_pszPartURL, pszHeader + strlen(vszLocationTag));

        DbgLog1(DEB_PROT|DEB_TRACE, this, "=== CINetHttp::RedirectRequest (Location:%s)", _pszPartURL);

        _fRedirected = TRUE;
        hr = S_FALSE;

        if (!ParseUrl())
        {
            pszRedirect = _pszPartURL;
        }
        else
        {
            pszRedirect = _pszFullURL;
        }

        PProtAssert((pszRedirect));
        ReportResultAndStop(INET_E_REDIRECTING, 0, 0, DupA2W(pszRedirect));
    }

End:
    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::RedirectRequest(hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::ErrorHandlingRequest
//
//  Synopsis:
//
//  Arguments:  [dwstatus] --
//              [szBuffer] --
//
//  Returns:
//
//  History:    2-28-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::ErrorHandlingRequest(DWORD dwStatus, LPSTR szBuffer)
{
    PerfDbgLog1(tagCINetHttp, this, "+CINetHttp::ErrorHandlingRequest (dwStatus:%ld)", dwStatus);
    HRESULT hr = NOERROR;

    PProtAssert((szBuffer));
    hr = HttpNegOnError(dwStatus,szBuffer);

    if (hr == E_RETRY)
    {
        _hrINet = NOERROR;
        hr = INetAsyncSendRequest();
    }
    else if (hr == E_ABORT)
    {
        _hrINet = E_ABORT;
    }
    else if (hr == S_FALSE)
    {
        // the error was not handled - stop download
        _hrINet = hr = HResultFromHttpStatus(dwStatus);
    }
    else
    {
        _hrINet = hr;
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::ErrorHandlingRequest(hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::GetVerb
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR CINetHttp::GetVerb()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::GetVerb");
    LPSTR pszRes = vszGet;

    if (_fRedirected == TRUE)
    {
        // for HTTP 1.1, we have to check if this is an POST->POST redirect
        INTERNET_VERSION_INFO   httpVersion;
        DWORD                   dwBufferSize = sizeof(INTERNET_VERSION_INFO);
        if(    InternetQueryOption( _hRequest, INTERNET_OPTION_HTTP_VERSION, &httpVersion, &dwBufferSize )
            && httpVersion.dwMajorVersion >= 1
            && httpVersion.dwMinorVersion >= 1 )
        {
            CHAR    szVerb[16];
            DWORD   dwIndex;
            DWORD   dwLength = sizeof(szVerb);
            if(    HttpQueryInfo(_hRequest, HTTP_QUERY_REQUEST_METHOD, szVerb, &dwLength, &dwIndex)
                && !lstrcmp(szVerb, vszPost) )
            {
                // HACK HACK HACK !!
                // Double check the status code to see if this is a real POST
                // there is a HttpQueryInfo() bug which will send verb=POST
                // on a POST->GET Redirect
                //

                DWORD dwStatus = 0;
                if (   HttpQueryInfo(
                           _hRequest,
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                           &dwStatus ,&dwLength, NULL)
                    && dwStatus == HTTP_STATUS_REDIRECT_KEEP_VERB )
                {
                    _fP2PRedirected = TRUE;
                    pszRes = vszPost;
                }
            }
        }
    }
    else
    {
        BINDINFO *pBndInfo = GetBindInfo();

        if (pBndInfo)
        {
            switch (pBndInfo->dwBindVerb)
            {
            case BINDVERB_GET      :
                pszRes = vszGet;
                break;
            case BINDVERB_POST     :
                pszRes = vszPost;
                break;
            case BINDVERB_PUT      :
                pszRes = vszPut;
                break;
            case BINDVERB_CUSTOM   :
                {
                    //BUGBUG: custom verb support
                    if (!_pszVerb && pBndInfo->szCustomVerb)
                    {
                        pszRes = _pszVerb = DupW2A(pBndInfo->szCustomVerb);
                    }
                    else if(_pszVerb)
                    {
                        pszRes = _pszVerb;
                    }
                }
                break;
            }
        }
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::GetVerb (szRes:%s)", pszRes);
    return pszRes;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::GetAdditionalHeader
//
//  Synopsis:
//
//  Arguments:  [ppszRes] --
//              [pdwSize] --
//
//  Returns:
//
//  History:    2-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::GetAdditionalHeader()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::GetAdditionalHeader");

    DWORD dwSizeData = 0;
    DWORD dwSizeHeader = 0;
    LPSTR szLocal;

    szLocal = g_pszUAInfoString;

    dwSizeHeader += strlen(szLocal);

    ULONG   dwSizeEncHeader = 0;

    PProtAssert((_pOIBindInfo));
    // only send Accept-Encoding header with HTTP 1.1 or higher
    INTERNET_VERSION_INFO   httpVersion;
    DWORD                   dwBufferSize = sizeof(INTERNET_VERSION_INFO);
    if(     _hRequest
         && _pOIBindInfo
         && InternetQueryOption( _hRequest, INTERNET_OPTION_HTTP_VERSION, &httpVersion, &dwBufferSize )
         && httpVersion.dwMajorVersion >= 1
         && httpVersion.dwMinorVersion >= 1 )
    {
        dwSizeEncHeader = 1;
        dwSizeHeader += strlen(gszAcceptEncHeaders);
    }

    // delete the old header and allocate a new buffer
    if (_pszHeader)
    {
        delete _pszHeader;
        _pszHeader = 0;
    }

    if (dwSizeHeader || dwSizeEncHeader)
    {
        _pszHeader = new CHAR [dwSizeHeader + 1];
    }

    if (_pszHeader)
    {
        if (szLocal && szLocal[0] != 0)
        {
            strcat(_pszHeader, szLocal);
        }

        if( dwSizeEncHeader)
        {
            strcat(_pszHeader, gszAcceptEncHeaders);
        }
    }

    PerfDbgLog2(tagCINetHttp, this, "-CINetHttp::GetAdditionalHeader (pStr:>%s<, hr:%lx)", XDBG(_pszHeader,""), NOERROR);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::GetDataToSend
//
//  Synopsis:
//
//  Arguments:  [ppBuffer] --
//              [pdwSize] --
//
//  Returns:
//
//  History:    2-05-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::GetDataToSend(LPVOID *ppBuffer, DWORD *pdwSize)
{
    HRESULT hr = INET_E_DOWNLOAD_FAILURE;
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::GetDataToSend");

    *ppBuffer = NULL;
    *pdwSize  = 0;
    BINDINFO *pBndInfo = GetBindInfo();

    if (pBndInfo)
    {
        switch (pBndInfo->dwBindVerb)
        {
        default:
        case BINDVERB_CUSTOM   :
        case BINDVERB_POST     :
        case BINDVERB_PUT      :
        {
            if (pBndInfo->stgmedData.tymed == TYMED_HGLOBAL)
            {
                *ppBuffer = pBndInfo->stgmedData.hGlobal;
                *pdwSize = pBndInfo->cbstgmedData;
                hr = NOERROR;
            }
        }
        break;
        case BINDVERB_GET     :
        // nothing should be uploaded
        break;
        }
    }

    PerfDbgLog3(tagCINetHttp, this, "-CINetHttp::GetDataToSend (pBuffer:%lx, dwSize:%ld, hr:%lx)", *ppBuffer, *pdwSize, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::HttpNegBeginningTransaction
//
//  Synopsis:
//
//  Arguments:  [szURL] --
//              [DWORD] --
//              [dwReserved] --
//              [pszAdditionalHeaders] --
//
//  Returns:
//
//  History:    2-08-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::HttpNegBeginningTransaction()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::HttpNegBeginningTransaction");
    HRESULT hr = NOERROR;
    LPWSTR pwzAddHeaders = NULL;

    if (_pHttpNeg == NULL)
    {
        hr = QueryService(IID_IHttpNegotiate, (void **) &_pHttpNeg);
    }
    if (_pHttpNeg && (hr == NOERROR))
    {
        LPWSTR pwzUrl = GetUrl();
        LPWSTR pwzHeaders = NULL;
        DWORD dwlen = 0;
        if (_pszHeader)
        {
            dwlen = strlen(_pszHeader);
            pwzHeaders = new WCHAR [dwlen +1];
            if (pwzHeaders == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto End;

            }
            A2W(_pszHeader, pwzHeaders,dwlen + 1);

        }

        PProtAssert((pwzUrl));
        hr = _pHttpNeg->BeginningTransaction(pwzUrl, pwzHeaders, NULL, &pwzAddHeaders);

        if (SUCCEEDED(hr) )
        {
            if (pwzAddHeaders)
            {
                // add the additional length
                dwlen += wcslen(pwzAddHeaders) * sizeof(WCHAR);
            }

            if (dwlen)
            {

                if (_pszSendHeader)
                {
                    if (strlen(_pszSendHeader) < (dwlen + 1))
                    {
                        // delete the old header
                        delete _pszSendHeader;
                        // allocate a new one
                        _pszSendHeader = new CHAR [dwlen + 1];
                    }
                }
                else
                {
                    _pszSendHeader = new CHAR [dwlen + 1];
                }

                if (_pszSendHeader)
                {
                    if ( pwzAddHeaders )
                    {
                        W2A(pwzAddHeaders, _pszSendHeader, dwlen + 1);

                        // append the original header
                        if (_pszHeader)
                        {
                            strcat(_pszSendHeader,_pszHeader);
                        }
                    }
                    else
                    {
                        // no additional header
                        strcpy(_pszSendHeader, _pszHeader);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                // delete the wchar header
                if (pwzHeaders)
                {
                    delete pwzHeaders;
                }
            }
        }


    }
    else
    {
        PProtAssert((_pHttpNeg == NULL));
    }

End:

    // delete this buffer
    if (pwzAddHeaders)
    {
        delete pwzAddHeaders;
    }

    PerfDbgLog(tagCINetHttp, this, "-CINetHttp::HttpNegBeginningTransaction");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::HttpNegOnHeadersAvailable
//
//  Synopsis:
//
//  Arguments:  [dwResponseCode] --
//              [szHeaders] --
//
//  Returns:
//
//  History:    2-08-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::HttpNegOnHeadersAvailable(DWORD dwResponseCode, LPSTR szResponseHeader)
{
    PerfDbgLog2(tagCINetHttp, this, "+CINetHttp::HttpNegOnHeadersAvailable (dwResponseCode:%lx) (szResponseHeader:%s)", dwResponseCode, XDBG(szResponseHeader,""));
    HRESULT hr = NOERROR;

    PProtAssert((szResponseHeader != NULL));

    if (_pHttpNeg)
    {

        LPWSTR pwzResponseHeader;
        DWORD dwlen = strlen(szResponseHeader);

        pwzResponseHeader = new WCHAR [dwlen +1];
        if (pwzResponseHeader == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            PProtAssert((pwzResponseHeader));
            A2W(szResponseHeader, pwzResponseHeader, dwlen + 1);
            if( _pHttpNeg )
            {
                hr = _pHttpNeg->OnResponse(dwResponseCode, pwzResponseHeader,NULL ,NULL);
            }

            // the only valid return code is NOERROR
            PProtAssert((hr == NOERROR && "HttpNegotiate::OnHeaders returned ivalid hresult"));
            PProtAssert((pwzResponseHeader));

            delete pwzResponseHeader;
        }
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::HttpNegOnHeadersAvailable (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::HttpNegOnError
//
//  Synopsis:
//
//  Arguments:  [dwResponseCode] --
//              [szResponseHeaders] --
//              [pszAdditionalRequestHeaders] --
//
//  Returns:
//
//  History:    2-08-96   JohannP (Johann Posch)   Created
//
//  Notes:      return S_FALSE as default - will stop download
//              and map to INET_E hresult
//----------------------------------------------------------------------------
HRESULT CINetHttp::HttpNegOnError(DWORD dwResponseCode, LPSTR szResponseHeader)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::HttpNegOnError");
    HRESULT hr = S_FALSE;

    PProtAssert((szResponseHeader != NULL));

    if (_pHttpNeg)
    {
        LPWSTR pwzNewSendHeader = 0;
        LPWSTR pwzSendHeader = 0;
        LPWSTR pwzResponseHeader = 0;
        DWORD dwLenSendHeader = 0;
        DWORD dwLenResponseHeader = 0;

        if (_pszSendHeader)
        {
            dwLenSendHeader = strlen(_pszSendHeader);
            pwzSendHeader = new WCHAR [dwLenSendHeader + 1];
        }

        if (pwzSendHeader)
        {
            A2W(_pszSendHeader, pwzSendHeader, dwLenSendHeader + 1);
        }

        if (szResponseHeader)
        {
            dwLenResponseHeader = strlen(szResponseHeader);
            pwzResponseHeader = new WCHAR [dwLenResponseHeader + 1];
        }

        if (pwzResponseHeader)
        {
            A2W(szResponseHeader, pwzResponseHeader, dwLenResponseHeader + 1);
        }

        if( _pHttpNeg )
        {
            hr = _pHttpNeg->OnResponse(dwResponseCode, pwzResponseHeader,pwzSendHeader ,&pwzNewSendHeader);
        }

        if (pwzSendHeader)
        {
            delete pwzSendHeader;
            pwzSendHeader = 0;
        }

        if (pwzResponseHeader)
        {
            delete pwzResponseHeader;
            pwzResponseHeader = 0;
        }

        if ((hr == NOERROR) && (pwzNewSendHeader != NULL))
        {
            LPSTR pszNewSendHeader = 0;
            DWORD dwLen = wcslen(pwzNewSendHeader);
            DWORD dwLen1 = 0;
            if (_pszSendHeader)
            {
                dwLen1 = strlen(_pszSendHeader);
            }
            PProtAssert((dwLen + dwLen1));

            pszNewSendHeader = new CHAR [dwLen + dwLen1 + 1];

            if (pszNewSendHeader)
            {
                strcpy(pszNewSendHeader, _pszSendHeader);
                W2A(pwzNewSendHeader,pszNewSendHeader + dwLen1, dwLen + 1);

                // retry the call
                hr = E_RETRY;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
        else
        {
            // we should not have headers
            PProtAssert((pwzNewSendHeader == NULL));
            if (pwzNewSendHeader)
            {
                delete pwzNewSendHeader;
            }
        }
    }
    else
    {
        // the error should be mapped to an INET_E hresult
        hr = S_FALSE;
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::HttpNegOnError (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::SecurityProblem
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
HRESULT CINetHttp::HttpSecurity(DWORD dwProblem)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::HttpSecurity");
    HRESULT hr = NOERROR;
    HWND hwnd;

    hr = HttpSecurityProblem(&hwnd, dwProblem);

    if (hr == NOERROR)
    {
        if (hwnd)
        {
            DWORD  dwBindF = GetBindFlags();
            DWORD dwFlags = (FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA );

            if ((dwBindF & BINDF_NO_UI) || (dwBindF & BINDF_SILENTOPERATION))
            {
                dwFlags |= FLAGS_ERROR_UI_FLAGS_NO_UI;
            }


            DWORD dwError;

            if (SUCCEEDED(ZonesSecurityCheck(hwnd, dwProblem, &dwError)))
            {
                // dwError will be set by ZonesSecurityCheck.
            }
            else
            {
                dwError = InternetErrorDlg(hwnd, _hRequest, dwProblem, dwFlags,NULL);
            }

            switch (dwError)
            {
            case ERROR_CANCELLED :
                _hrINet = hr = E_ABORT;
                break;

            case ERROR_SUCCESS  :
                _hrINet = hr = E_RETRY;
                break;

            default:
                _hrINet = hr = E_ABORT;
                break;
            }
        }
        else
        {
            hr = INET_E_SECURITY_PROBLEM;
        }
    }
    else if (hr == E_ABORT)
    {
        _hrINet = E_ABORT;
    }

    if (hr == E_RETRY)
    {
        _hrINet = NOERROR;
        hr = INetAsyncSendRequest();
    }
    else if (hr == E_ABORT)
    {
        _hrINet = E_ABORT;
    }
    else if (hr != NOERROR )
    {
        // set the error to access denied
        _hrINet = hr = INET_E_SECURITY_PROBLEM;
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::HttpSecurity(hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINet::ZonesSecurityCheck
//
//  Synopsis:
//
//  Arguments:  [hwnd] --
//              [dwProblem] --
//              [pdwError] --
//
//  Returns:   SUCCESS if it was able to decide the action, INET_E_DEFAULT_ACTION otherwise.
//
//  History:    8-14-97   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT CINetHttp::ZonesSecurityCheck(HWND hWnd, DWORD dwProblem, DWORD *pdwError)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::HttpSecurity");
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if (pdwError == NULL)
    {
        TransAssert(FALSE);
        return E_INVALIDARG;
    }

    // Right now the only error we check for is the redirect confirmation error.
    if (dwProblem == ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION ||
        dwProblem == ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR)
    {
        IInternetSecurityManager *pSecMgr = NULL;

        if (SUCCEEDED(CoCreateInstance(CLSID_InternetSecurityManager, NULL,
                        CLSCTX_INPROC_SERVER, IID_IInternetSecurityManager, (void **)&pSecMgr)))
        {
            char szUrl[MAX_URL_SIZE];
            WCHAR wzUrl[MAX_URL_SIZE];
            DWORD cbLen = MAX_URL_SIZE;
            DWORD dwPolicy;

            TransAssert(pSecMgr != NULL);

            InternetQueryOption(_hRequest, INTERNET_OPTION_URL, szUrl, &cbLen);

            // First check if the redirect is to the same server. If that is the 
            // case we don't need to warn because we already did the first time around.

            if (dwProblem == ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION)
            {
                URL_COMPONENTS uc = { 0 };

                uc.dwStructSize = sizeof(uc);
                uc.dwHostNameLength = 1;     // So we get back the host name. 

                if ( InternetCrackUrl(szUrl, 0, 0, &uc) && 
                      (StrCmpNI(uc.lpszHostName, GetServerName(), uc.dwHostNameLength) == 0)
                   )
                {
                    *pdwError = ERROR_SUCCESS;
                    hr = S_OK;
                }
            }
            
            if (hr == INET_E_DEFAULT_ACTION)             
            {
                // Convert to widechar so we can call the security manager. 
                MultiByteToWideChar(CP_ACP, 0, szUrl, -1, wzUrl, MAX_URL_SIZE);

                PARSEDURL pu;
                pu.cbSize = sizeof(pu);
                if (SUCCEEDED(ParseURLA(szUrl, &pu)) && pu.nScheme == URL_SCHEME_HTTPS)
                {
                    // The forms submit zones policies are only supposed to apply to
                    // unencrypted posts. We will allow these to be posted silently.
                    *pdwError = ERROR_SUCCESS;
                    hr = S_OK ;
                }
                else if (SUCCEEDED(pSecMgr->ProcessUrlAction(wzUrl, URLACTION_HTML_SUBMIT_FORMS_TO,
                                (BYTE *)&dwPolicy, sizeof(dwPolicy), NULL, 0, PUAF_NOUI, 0)))
                {
                    DWORD dwPermissions = GetUrlPolicyPermissions(dwPolicy);
                    // If it is allow or deny don't call InternetErrorDlg, unless it is a encrypted to
                    // unencrypted redir in which case we still need to warn the user..
                    if (dwPermissions == URLPOLICY_ALLOW && dwProblem != ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR)
                    {
                        *pdwError = ERROR_SUCCESS;
                        hr = S_OK;
                    }
                    else if (dwPermissions == URLPOLICY_DISALLOW)
                    {
                        *pdwError = ERROR_CANCELLED;
                        hr = S_OK;
                    }
                }
            }

            pSecMgr->Release();
        }
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::ZonesSecurityCheck(hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::HttpSecurityProblem
//
//  Synopsis:   QI's for HttpSecurity or IWindow
//
//  Arguments:  [phwnd] -- window handle for security dialog
//
//  Returns:    S_OK if dialog should be displayed
//
//  History:    2-08-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::HttpSecurityProblem(HWND* phwnd, DWORD dwProblem)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::HttpSecurityProblem");
    HRESULT hr = NOERROR;
    *phwnd = 0;

    if (_pHttSecurity == NULL)
    {
        hr = QueryService(IID_IHttpSecurity, (void **) &_pHttSecurity);
    }

    if ((hr == NOERROR) && _pHttSecurity)
    {
         hr = _pHttSecurity->OnSecurityProblem(dwProblem);
         if (hr == S_OK)
         {
             // client wants to continue
         }
         else if (hr == S_FALSE)
         {
             // client does not care
             hr = _pHttSecurity->GetWindow(IID_IHttpSecurity, phwnd);
             UrlMkAssert((   ((hr == S_FALSE) && (*phwnd == NULL))
                          || ((hr == NOERROR) && (*phwnd != NULL)) ));
         }
         else if (hr != E_ABORT)
         {
             UrlMkAssert((FALSE && "Invalid result on IHttSecurity->OnSecurityProblem"));
         }
    }
    else
    {
        if (_pWindow == NULL)
        {
            hr = QueryService(IID_IWindowForBindingUI, (void **) &_pWindow);
        }

        if ((hr == NOERROR) && _pWindow)
        {
             hr = _pWindow->GetWindow(IID_IHttpSecurity, phwnd);
             UrlMkAssert((   (hr == S_FALSE) && (*phwnd == NULL)
                          || (hr == S_OK) && (*phwnd != NULL)));
        }
    }

    PerfDbgLog2(tagCINetHttp, this, "-CINetHttp::HttpSecurityProblem (hr:%lx, hwnd:%lx)", hr, *phwnd);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::HResultFromInternetError
//
//  Synopsis:
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
HRESULT CINetHttp::HResultFromHttpStatus(DWORD dwStatus)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::HResultFromHttpStatus");
    HRESULT hr = NOERROR;
    switch(dwStatus)
    {
    case  HTTP_STATUS_OK                :
    case  HTTP_STATUS_NOT_MODIFIED      :
    case  HTTP_STATUS_RETRY_WITH        :
        hr = NOERROR;
    break;

    case  HTTP_STATUS_NOT_FOUND :
        hr = INET_E_OBJECT_NOT_FOUND;
    break;

    case  HTTP_STATUS_NONE_ACCEPTABLE   :
        // comes back if server can not handle mime type
        hr = INET_E_NO_VALID_MEDIA;
    break;

    case  HTTP_STATUS_SERVICE_UNAVAIL   :
        hr = INET_E_INVALID_REQUEST;
        break;

    case  HTTP_STATUS_GATEWAY_TIMEOUT  :
    case  HTTP_STATUS_REQUEST_TIMEOUT  :
        hr = INET_E_CONNECTION_TIMEOUT;
        break;

    case  HTTP_STATUS_CREATED           :
    case  HTTP_STATUS_ACCEPTED          :
    case  HTTP_STATUS_PARTIAL           :
    case  HTTP_STATUS_NO_CONTENT        :
    case  HTTP_STATUS_AMBIGUOUS         :
    case  HTTP_STATUS_MOVED             :
    case  HTTP_STATUS_REDIRECT          :
    case  HTTP_STATUS_REDIRECT_METHOD   :
    case  HTTP_STATUS_REDIRECT_KEEP_VERB:
    case  HTTP_STATUS_BAD_REQUEST       :
    case  HTTP_STATUS_DENIED            :
    case  HTTP_STATUS_PAYMENT_REQ       :
    case  HTTP_STATUS_FORBIDDEN         :
    case  HTTP_STATUS_BAD_METHOD        :
    case  HTTP_STATUS_PROXY_AUTH_REQ    :
    case  HTTP_STATUS_CONFLICT          :
    case  HTTP_STATUS_GONE              :
    case  HTTP_STATUS_LENGTH_REQUIRED   :
    case  HTTP_STATUS_SERVER_ERROR      :
    case  HTTP_STATUS_NOT_SUPPORTED     :
    case  HTTP_STATUS_BAD_GATEWAY       :
    default:
        //PProtAssert((FALSE && "Mapping Ineternet error to generic hresult!"));
        hr = INET_E_DOWNLOAD_FAILURE;
        DbgLog2(DEB_PROT|DEB_TRACE, this, "=== Mapping Internet error to generic hresult!(dwStatus:%ld, hr:%lx)", dwStatus, hr);
        break;
    }

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::HResultFromHttpStatus (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::Terminate
//
//  Synopsis:   Close the server and request handle - wininet will make a
//              callback on each handle closed
//
//  Arguments:  [dwOptions] --
//
//  Returns:
//
//  History:    07-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINetHttp::Terminate(DWORD dwOptions)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttp::Terminate");
    HRESULT hr = NOERROR;
    //PProtAssert(( IsApartmentThread() ));

    if (_pHttpNeg)
    {
        PerfDbgLog1(tagCINetHttp, this, "=== CINetHttp::Terminate Release on _pHttpNeg (%lx)", _pHttpNeg);
        _pHttpNeg->Release();
        _pHttpNeg = NULL;
    }
    if (_pWindow)
    {
        PerfDbgLog1(tagCINetHttp, this, "+CINetHttp::Terminate Release on _pWindow (%lx)", _pWindow);
        _pWindow->Release();
        _pWindow = NULL;
    }

    if (_pHttSecurity)
    {
        PerfDbgLog1(tagCINetHttp, this, "+CINetHttp::Terminate Release on _pHttSecurity (%lx)", _pHttSecurity);
        _pHttSecurity->Release();
        _pHttSecurity = NULL;
    }

    hr = CINet::Terminate(dwOptions);

    PerfDbgLog1(tagCINetHttp, this, "-CINetHttp::Terminate (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetHttpS::CINetHttpS
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetHttpS::CINetHttpS(REFCLSID rclsid, IUnknown *pUnkOuter) : CINetHttp(rclsid,pUnkOuter)
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttpS::CINetHttpS");

    _dwIsA = DLD_PROTOCOL_HTTPS;
    _dwConnectFlags = INTERNET_FLAG_SECURE;
    _dwOpenFlags = INTERNET_FLAG_SECURE;

    PerfDbgLog(tagCINetHttp, this, "-CINetHttpS::CINetHttpS");
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttpS::~CINetHttpS
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetHttpS::~CINetHttpS()
{
    PerfDbgLog(tagCINetHttp, this, "+CINetHttpS::~CINetHttpS");

    PerfDbgLog(tagCINetHttp, this, "-CINetHttpS::~CINetHttpS");
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::INetWrite
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
HRESULT CINetHttp::INetWrite()
{
    TransDebugOut((DEB_PROT, "%p OUT CINetHttp::INetWrite\n", this));
    HRESULT hr = NOERROR;

    BOOL fRet;
    DWORD dwSendFlags = HSR_CHUNKED | HSR_INITIATE;
    DWORD dwSendContext = 0;

    // If our caller gave us a TYMED_ISTREAM, then we need to pick it up
    // now.
    if (!_pStm)
    {
        BINDINFO *pBI = GetBindInfo();
        if (pBI && pBI->stgmedData.tymed == TYMED_ISTREAM)
        {
            _pStm = pBI->stgmedData.pstm;
        }
    }

    TransAssert((_pStm));
    if( _fSendAgain && _pStm )
    {
        LARGE_INTEGER        li;
        li.LowPart = 0;
        li.HighPart = 0;
        hr = _pStm->Seek(li, STREAM_SEEK_SET, NULL);
        if( SUCCEEDED(hr) )
        {
            _fSendEnd = FALSE;
        }
        else
        {
            _fCompleted = TRUE;
        }

        _fSendAgain = FALSE;
    }

    // loop until pending
    if (_fSendEnd)
    {
        _hrError = INET_E_DONE;
    }
    else do
    {
        _dwBytesSent = 0;

        SetStatePending(E_PENDING);

        hr = GetNextSendBuffer(&_inetBufferSend,_pStm);
        if (FAILED(hr))
        {
            break;
        }

        if (hr == S_FALSE)
        {
            // end of stream
            _fCompleted = TRUE;
        }

        if (!_fCompleted)
        {
            if (_inetBufferSend.dwBufferLength)
            {
                fRet = InternetWriteFile(
                            _hRequest           //IN HINTERNET hFile,
                            ,_inetBufferSend.lpvBuffer         //IN LPCVOID lpBuffer,
                            ,_inetBufferSend.dwBufferLength    //IN DWORD dwNumberOfBytesToWrite,
                            ,&_dwBytesSent       //OUT LPDWORD lpdwNumberOfBytesWritten
                            );
            }
            else
            {
                fRet = TRUE;
            }
        }
        else
        {
            fRet = HttpEndRequestA(
                         _hRequest              //IN HINTERNET hRequest,
                        ,NULL
                        ,dwSendFlags            //IN DWORD dwFlags,
                        ,dwSendContext          //IN DWORD dwContext

                        );

            _fSendEnd = TRUE;
        }

        //PerfDbgLog(tagCINetHttp, this, "-CINetHttp::INetAsyncSendRequest HttpSendRequest");

        if (fRet == FALSE)
        {
            DWORD dwLstError = GetLastError();
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
                //PerfDbgLog3(tagCINetHttp, this, "CINetHttp::INetAsyncSendRequest (fRet:%d, _hrError:%lx, LstError:%ld)", fRet, _hrError, dwLstError);
            }
        }
        else
        {
            SetStatePending(NOERROR);
        }

    } while ((fRet == TRUE) && (_fCompleted == FALSE));

    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop((_hrError == INET_E_DONE) ? NOERROR : _hrError);
    }

    TransDebugOut((DEB_PROT, "%p OUT CINetHttp::INetWrite (hr:%lx)\n", this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetHttp::GetNextSendBuffer
//
//  Synopsis:
//
//  Arguments:  [pIB] --
//              [pStm] --
//              [fFirst] --
//
//  Returns:
//
//  History:    4-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetHttp::GetNextSendBuffer(INTERNET_BUFFERS *pIB, IStream *pStm, BOOL fFirst)
{
    TransDebugOut((DEB_PROT, "%p OUT CINetHttp::GetNextSendBuffer\n", this));
    HRESULT hr = NOERROR;

    TransAssert(pIB);

    do
    {
        BINDINFO *pBndInfo = GetBindInfo();
        DWORD dwBufferFilled = 0;

        if (!pStm)
        {
            hr = E_FAIL;
            break;
        }

        if (!fFirst)
        {
            TransAssert((_pBuffer));
            hr = pStm->Read(_pBuffer, _dwBufferSize, &dwBufferFilled);

            if (FAILED(hr))
            {
                break;
            }
            else if (!dwBufferFilled)
            {
            	hr = S_FALSE;
            }
        }
        else
        {
            LARGE_INTEGER        li;
            li.LowPart = 0;
            li.HighPart = 0;

            // We do not need to addref this here
            pStm->Seek(li, STREAM_SEEK_SET, NULL);

            if( !_pBuffer )
            {
                _pBuffer = new char [SENDBUFFER_MAX];

                if (!_pBuffer)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                _dwBufferSize = SENDBUFFER_MAX;
            }
        }

        pIB->dwStructSize = sizeof (INTERNET_BUFFERSA);
        pIB->Next = 0;
        pIB->lpcszHeader = (fFirst) ? _pszSendHeader : 0;
        pIB->dwHeadersLength = (fFirst) ? ((_pszSendHeader) ? (ULONG)-1L : 0L) : 0;
        pIB->dwHeadersTotal = (fFirst) ? ((_pszSendHeader) ? (ULONG)-1L : 0L) : 0;
        pIB->lpvBuffer = (fFirst) ? 0 : _pBuffer;
        pIB->dwBufferLength = (fFirst) ? 0 : dwBufferFilled;
        pIB->dwBufferTotal = (fFirst) ? pBndInfo->cbstgmedData : 0; // :_dwBufferSize;
        pIB->dwOffsetLow = 0;
        pIB->dwOffsetHigh = 0;

        break;
    } while (TRUE);

    TransDebugOut((DEB_PROT, "%p OUT CINetHttp::GetNextSendBuffer (hr:%lx)\n", this, hr));
    return hr;
}


