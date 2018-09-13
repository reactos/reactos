// 
// Pei-Hwa Lin (peiwhal), Feb 3, 1997
//

#include "private.h"
#include "downld.h"
#include "urlmon.h"

#undef TF_THISMODULE
#define TF_THISMODULE   TF_POSTAGENT

// Advanced inetcpl setting to disable channel logging
const TCHAR c_szNoChannelLogging[] = TEXT("NoChannelLogging");

const char c_szHeaders[] = "Content-Type: application/x-www-form-urlencoded\r\n";
#define c_ccHearders  (ARRAYSIZE(c_szHeaders) - 1)

#define ENCODE_LOG
#ifdef ENCODE_LOG
const char c_szEncodeHeader[] = "Content-Transfer-Encoding: x-";
#define c_ccEncodeHeader (ARRAYSIZE(c_szEncodeHeader) - 1)
#endif  //ENCODE_LOG

// *** W3C extended log format *** //
// text strings definitions
const char c_szUrlFields[] = "#Fields: s-URI";
#define c_ccUrlFields  (ARRAYSIZE(c_szUrlFields) - 1)

const char c_szLogFields[] = "#Fields: x-context x-cache x-date x-time x-duration x-meta";
#define c_ccLogFields  (ARRAYSIZE(c_szLogFields) - 1);
// *** W3C extended log format *** //

const TCHAR c_szPostRetryReg[] = TEXT("PostRetry");
const TCHAR c_szUserAgent[] = TEXT("User Agent");
extern TCHAR szInternetSettings[];

//-----------------------------------------------------------------------------
//
// CTrackingStg implementation
//
//-----------------------------------------------------------------------------

// this will empty log contents without removing the entry
//
HRESULT	CTrackingStg::EmptyCacheEntry(GROUPID enumId)
{
    HANDLE      hEnum, hFile;
    LPINTERNET_CACHE_ENTRY_INFOA lpCE = NULL;
    DWORD       cbSize;
    HRESULT     hr = S_OK;

    ASSERT(enumId);

    lpCE = (LPINTERNET_CACHE_ENTRY_INFOA)MemAlloc(LPTR, MY_MAX_CACHE_ENTRY_INFO);
    if (!lpCE)
        return E_OUTOFMEMORY;

    cbSize = MY_MAX_CACHE_ENTRY_INFO;
    hEnum = FindFirstUrlCacheEntryExA(_lpPfx,
                                     0,              //dwFlags
                                     URLCACHE_FIND_DEFAULT_FILTER,  //dwFilters,
                                     0,             //enumId, IE50:wininet change, not support group in extensible cache
                                     lpCE,
                                     &cbSize,
                                     NULL, NULL, NULL);

    if (hEnum)
    {
        FILETIME    ftModified;

        ftModified.dwHighDateTime = (DWORD)(enumId >> 32);
        ftModified.dwLowDateTime = (DWORD)(0x00000000ffffffff & enumId);
        for(;;)
        {
            if (lpCE->LastModifiedTime.dwLowDateTime == ftModified.dwLowDateTime &&
                lpCE->LastModifiedTime.dwHighDateTime == ftModified.dwHighDateTime)
            {
                hFile = OpenItemLogFile(lpCE->lpszLocalFileName);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hFile);
                    DeleteFileA(lpCE->lpszLocalFileName);

                    DeleteUrlCacheEntryA(lpCE->lpszSourceUrlName);
                    
                    TCHAR   szUrl[INTERNET_MAX_URL_LENGTH];
                    SHAnsiToTChar(lpCE->lpszSourceUrlName, szUrl, ARRAYSIZE(szUrl));
                    CreateLogCacheEntry(szUrl, lpCE->ExpireTime,
                                         ftModified, lpCE->CacheEntryType);
                }

            }
            
            // reuse lpCE
            if (!FindNextUrlCacheEntryExA(hEnum, lpCE, &cbSize, NULL, NULL, NULL))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }
        }

        FindCloseUrlCache(hEnum);
    }

    SAFELOCALFREE(lpCE);
    return hr;
}

// this will delete url cache entries 
// and delete url cache group
HRESULT	CTrackingStg::RemoveCacheEntry(GROUPID enumId)
{
    HANDLE      hEnum;
    LPINTERNET_CACHE_ENTRY_INFOA lpCE = NULL;
    DWORD       cbSize;
    PROPVARIANT vProp = {0};
    HRESULT     hr = S_OK;

    ASSERT(enumId);

    lpCE = (LPINTERNET_CACHE_ENTRY_INFOA)MemAlloc(LPTR, MY_MAX_CACHE_ENTRY_INFO);
    if (!lpCE)
        return E_OUTOFMEMORY;

    cbSize = MY_MAX_CACHE_ENTRY_INFO;
    hEnum = FindFirstUrlCacheEntryExA(_lpPfx,
                                     0,              //dwFlags
                                     URLCACHE_FIND_DEFAULT_FILTER,   //dwFilters,
                                     0,              //enumId, IE50: wininet change
                                     lpCE,
                                     &cbSize,
                                     NULL, NULL, NULL);

    if (hEnum)
    {
        FILETIME    ftModified;

        ftModified.dwHighDateTime = (DWORD)(enumId >> 32);
        ftModified.dwLowDateTime = (DWORD)(0x00000000ffffffff & enumId);
        for(;;)
        {
            if (lpCE->LastModifiedTime.dwLowDateTime == ftModified.dwLowDateTime &&
                lpCE->LastModifiedTime.dwHighDateTime == ftModified.dwHighDateTime)
            {
                DeleteUrlCacheEntryA(lpCE->lpszSourceUrlName);
                vProp.vt = VT_UI4;
                vProp.ulVal = 0;            // clear tracking flag

                TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
                SHAnsiToTChar(lpCE->lpszSourceUrlName+lstrlenA(_lpPfx), szUrl, ARRAYSIZE(szUrl));
                hr = IntSiteHelper(szUrl,
                                   &c_rgPropRead[PROP_TRACKING], &vProp, 1, TRUE);
                PropVariantClear( &vProp );        
            }
            
            // reuse lpCE
            if (!FindNextUrlCacheEntryExA(hEnum, lpCE, &cbSize, NULL, NULL, NULL))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }
        }

        FindCloseUrlCache(hEnum);
    }

    SAFELOCALFREE(lpCE);
    return hr;
}

HANDLE CTrackingStg::OpenItemLogFile(LPCSTR lpFile)
{
   HANDLE   hFile = NULL;

    hFile = CreateFileA(lpFile, 
                GENERIC_READ | GENERIC_WRITE,
                0,                              //no sharing
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    return hFile;
}

DWORD CTrackingStg::ReadLogFile(LPCSTR lpFile, LPSTR* lplpbuf)
{
    HANDLE  hFile = NULL;
    DWORD   cbFile = 0;
    DWORD   cbRead;

    hFile = OpenItemLogFile(lpFile);
    if (hFile == INVALID_HANDLE_VALUE) 
        return 0;

    cbFile = GetFileSize(hFile, NULL);
    if (cbFile == 0xFFFFFFFF)
    {
        CloseHandle(hFile);
        return 0;
    }
        
    *lplpbuf = (LPSTR)MemAlloc(LPTR, (cbFile + 2) * sizeof(CHAR));
    cbRead = 0;
    if (!ReadFile(hFile, *lplpbuf, cbFile, &cbRead, NULL))
    {
        cbRead = 0;
    }    
        
    ASSERT((cbRead == cbFile));
    CloseHandle(hFile);
    return cbRead;
}


void CTrackingStg::AppendLogEntries(LPINTERNET_CACHE_ENTRY_INFOA lpce)
{
    LPSTR  lpbuf = NULL;
    DWORD   cbsize, cbWritten;
    
    cbsize = ReadLogFile(lpce->lpszLocalFileName, &lpbuf);
    cbWritten = 0;
    if (lpbuf && (cbsize > c_ccEmptyLog))   //change this threshold if modify
    {                                       //CreatePrefixedCacheEntry in trkcache.cpp
        AppendLogUrlField(lpce);            
        WriteFile(_hLogFile, lpbuf, cbsize, &cbWritten, NULL);
        ASSERT((cbsize == cbWritten));
    }

    SAFELOCALFREE(lpbuf);
    
    return;

}

void CTrackingStg::AppendLogUrlField(LPINTERNET_CACHE_ENTRY_INFOA lpce)
{
    DWORD dwWritten = 0;
    LPSTR lpbuf = NULL;
    DWORD cb;

    // lpce->lpszSourcesUrlName contains prefixed string
    DWORD cchAlloc = lstrlenA(lpce->lpszSourceUrlName) - lstrlenA(_lpPfx) + c_ccUrlFields 
                        + c_ccLogFields;
    cb = (cchAlloc + 7) * sizeof(CHAR);
    lpbuf = (LPSTR)MemAlloc(LPTR, cb);
    if (lpbuf)
    {
        wnsprintfA(lpbuf, cb, "%s\r\n%s\r\n%s\r\n", c_szUrlFields, 
                 lpce->lpszSourceUrlName+lstrlenA(_lpPfx), c_szLogFields);
        WriteFile(_hLogFile, lpbuf, lstrlenA(lpbuf), &dwWritten, NULL);
    }
    
    SAFELOCALFREE(lpbuf);
    return;
}

HRESULT CTrackingStg::Enumeration(LONGLONG enumId)
{
    HRESULT hr = E_FAIL;
    HANDLE  hEnum;
    LPINTERNET_CACHE_ENTRY_INFOA lpCE = NULL;

    lpCE = (LPINTERNET_CACHE_ENTRY_INFOA)MemAlloc(LPTR, MY_MAX_CACHE_ENTRY_INFO);
    if (!lpCE)
        return E_OUTOFMEMORY;

    DWORD   cbSize = MY_MAX_CACHE_ENTRY_INFO;
    hEnum = FindFirstUrlCacheEntryExA(_lpPfx,
                                    0,              //dwFlags
                                    URLCACHE_FIND_DEFAULT_FILTER,              //dwFilters,
                                    0,              //enumId, IE50: wininet change, not support group in extensible cache
                                    lpCE,
                                    &cbSize,
                                    NULL, NULL, NULL);

    if (hEnum)
    {
        FILETIME    ftModified;
    
        ftModified.dwHighDateTime = (DWORD)(enumId >> 32);
        ftModified.dwLowDateTime = (DWORD)(0x00000000ffffffff & enumId);
        for(;;)
        {
            if (!StrCmpNIA(lpCE->lpszSourceUrlName, _lpPfx, lstrlenA(_lpPfx))
                && lpCE->LastModifiedTime.dwLowDateTime == ftModified.dwLowDateTime 
                && lpCE->LastModifiedTime.dwHighDateTime == ftModified.dwHighDateTime)
            {    
                AppendLogEntries(lpCE);
            }
            
            // reuse lpCE
            cbSize = MY_MAX_CACHE_ENTRY_INFO;
            if (!FindNextUrlCacheEntryExA(hEnum, lpCE, &cbSize, NULL, NULL, NULL))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }
        }

        FindCloseUrlCache(hEnum);
        hr = S_OK;
    }

    SAFELOCALFREE(lpCE);
    return hr;
}

HRESULT CTrackingStg::RetrieveLogData(ISubscriptionItem* pCDFItem)
{
    HRESULT hr = E_FAIL;

    // heck: wininet does not support multiple groups to same URL
    // have to enumerate new and old groups to cover
    GROUPID newId;

    ReadLONGLONG(pCDFItem, c_szTrackingCookie, &newId);
    if (newId)
    {
        hr = Enumeration(newId);
    }
    // heck: end

    hr = Enumeration(_groupId);

    CloseLogFile();
    return hr;
}

HRESULT CTrackingStg::RetrieveAllLogStream(ISubscriptionItem* pCDFItem, LPCSTR lpFile)
{
    HRESULT hr = E_FAIL;
    LPTSTR  lpPfx = ReadTrackingPrefix();

#ifdef UNICODE
    if (lpPfx)
    {
        int len = lstrlenW(lpPfx) + 1;
        
        _lpPfx = (LPSTR)MemAlloc(LPTR, len * sizeof(CHAR));

        if (_lpPfx)
        {
            SHUnicodeToAnsi(lpPfx, _lpPfx, len);
        }

        MemFree(lpPfx);
    }
#else
    _lpPfx = lpPfx;
#endif

    _hLogFile = CreateFileA(lpFile, 
                GENERIC_WRITE,
                0,                              //no sharing
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

    if (_hLogFile == INVALID_HANDLE_VALUE)      //turn-around is read into a buffer.
        return hr;

    hr = RetrieveLogData(pCDFItem);

    return S_OK;    
}

//  
//  Validate the Posting by
//  1. attempt to post this log does not exceed limits set in registry
//  2. the log itself is valid
//
BOOL CTrackingStg::IsExpired(ISubscriptionItem* pItem)
{
    BOOL    bret;
    DWORD   regRetry = 3;   // Intelligent default so we don't need the registry value set.
        
    ReadRegValue(HKEY_LOCAL_MACHINE, MY_WEBCHECK_POST_REG,
                     c_szPostRetryReg, (void *)&regRetry, sizeof(regRetry));
                     
    bret = (_dwRetry < regRetry) ? FALSE : TRUE;

    if (bret) return bret;

    // read the purgetime from subscription property bag
    DATE        dt = 0.0;

    if (SUCCEEDED(ReadDATE(pItem, c_szPostPurgeTime, &dt)) && dt != 0)
    {
        SYSTEMTIME  st, expiredst;
        FILETIME    ft, expiredft;

        VariantTimeToSystemTime(dt, &expiredst);
        SystemTimeToFileTime(&expiredst, &expiredft);

        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &ft);

        bret = (CompareFileTime(&ft, &expiredft) > 0) ? TRUE : FALSE;
    }

    return bret;
}


CTrackingStg::CTrackingStg
(
)
{
    _pwszURL = NULL;
    _groupId = 0;
    _dwRetry = 0;
    _lpPfx = NULL;
}

CTrackingStg::~CTrackingStg()
{
    //
    // Release/delete any resources
    //
    DBG("CTrackingStg d'tor");

    SAFEFREEOLESTR(_pwszURL);
    SAFELOCALFREE(_lpPfx);
    
    if (_hLogFile)
        CloseHandle(_hLogFile);

}


//------------------------------------------------------------
//
// Private member functions
//
//------------------------------------------------------------

#if 0
void CALLBACK CWCPostAgent::PostCallback
(
    HINTERNET hInternet,
    DWORD     dwContext,
    DWORD     dwInternetStatus,
    LPVOID    lpvStatusInfo,
    DWORD     dwInfoLen
)
{
    DWORD dwLen;

    TraceMsg(TF_THISMODULE, "PostAgent Callback: of %d (looking for 31)", dwContext);
    if ( (dwInternetStatus == INTERNET_STATUS_REQUEST_SENT) && 
         (dwLen == (DWORD)lpvStatusInfo) )
    {
        OnUploadComplete();
    }
    else
        UpdateStatus();
}

#endif
//------------------------------------------------------------
//
//  CWCPostAgent implementation
//
//------------------------------------------------------------

//------------------------------------------------------------
// virtual functions overriding CDeliveryAgent
//------------------------------------------------------------

ISubscriptionMgr2 *
CWCPostAgent::GetSubscriptionMgr()
{
    ISubscriptionMgr2* pSubsMgr=NULL;

    CoCreateInstance(CLSID_SubscriptionMgr, NULL, CLSCTX_INPROC_SERVER,
                    IID_ISubscriptionMgr2, (void**)&pSubsMgr);

    return pSubsMgr;
}

//
// Re-stamp a set of tracking URLs from old group to new group
// without losing logging data
//
void
CWCPostAgent :: MergeGroupOldToNew()
{
    GROUPID newId;
    HANDLE  hEnum;
    LPINTERNET_CACHE_ENTRY_INFO lpCE = NULL;
    DWORD   cbSize;
    LPTSTR  lpPfx = NULL;

    newId = 0;
    ReadLONGLONG(_pCDFItem, c_szTrackingCookie, &newId);

    if (!newId && !_pUploadStream->_groupId)
        return;
    
    ASSERT(newId);

    lpCE = (LPINTERNET_CACHE_ENTRY_INFO)MemAlloc(LPTR, MY_MAX_CACHE_ENTRY_INFO);
    if (!lpCE)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return;
    }

    lpPfx = ReadTrackingPrefix();
    if (!lpPfx)
        return;

    cbSize = MY_MAX_CACHE_ENTRY_INFO;
    hEnum = FindFirstUrlCacheEntryEx(lpPfx,
                                    0,              //dwFlags
                                    URLCACHE_FIND_DEFAULT_FILTER,              //dwFilters,
                                    0,              //_pUploadStream->_groupId, IE50 changed
                                    lpCE,
                                    &cbSize,
                                    NULL, NULL, NULL);

    if (hEnum)
    {
        FILETIME    ftModified, ftOld;

        ftModified.dwHighDateTime = (DWORD)(newId >> 32);
        ftModified.dwLowDateTime = (DWORD)(0x00000000ffffffff & newId);
        ftOld.dwHighDateTime = (DWORD)(_pUploadStream->_groupId >> 32);
        ftOld.dwLowDateTime = (DWORD)(0x00000000ffffffff & _pUploadStream->_groupId);

        for(;;)
        {
            if (!StrCmpNI(lpCE->lpszSourceUrlName, lpPfx, lstrlen(lpPfx))
                && lpCE->LastModifiedTime.dwLowDateTime == ftOld.dwLowDateTime 
                && lpCE->LastModifiedTime.dwHighDateTime == ftOld.dwHighDateTime)
            {
                // commit changes to url cache
                lpCE->LastModifiedTime.dwHighDateTime = ftModified.dwHighDateTime;
                lpCE->LastModifiedTime.dwLowDateTime = ftModified.dwLowDateTime;
                
                SetUrlCacheEntryInfo(lpCE->lpszSourceUrlName, lpCE, CACHE_ENTRY_MODTIME_FC);
            }

            // reuse lpCE
            cbSize = MY_MAX_CACHE_ENTRY_INFO;
            if (!FindNextUrlCacheEntryEx(hEnum, lpCE, &cbSize, NULL, NULL, NULL))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }
        }

        FindCloseUrlCache(hEnum);
    }

    SAFELOCALFREE(lpCE);
    return;
}

HRESULT CWCPostAgent::InitRequest
(
    LPCSTR lpszVerb
)
{
    char    hostName[INTERNET_MAX_HOST_NAME_LENGTH+1];
    char    userName[INTERNET_MAX_USER_NAME_LENGTH+1];
    char    password[INTERNET_MAX_PASSWORD_LENGTH+1];
    char    urlPath[INTERNET_MAX_PATH_LENGTH+1];
    URL_COMPONENTSA     uc;
    BOOL    bRet;
    LPSTR   pszPostURL;
    int     iLen;
    TCHAR   pszUA[128];
    HRESULT hr = E_FAIL;

    if ( !_pUploadStream->_pwszURL )
        return S_OK;             //do nothing if post url doesn't exist

    iLen = lstrlenW(_pUploadStream->_pwszURL) + 1;
    pszPostURL = new CHAR[iLen];
    if (!pszPostURL)
        return E_OUTOFMEMORY;

    SHUnicodeToAnsi(_pUploadStream->_pwszURL, pszPostURL, iLen);

    memset(&uc, 0, sizeof(URL_COMPONENTS));
    uc.dwStructSize = sizeof(URL_COMPONENTS);
    uc.lpszHostName = hostName;
    uc.dwHostNameLength = sizeof(hostName);
    uc.nPort = INTERNET_INVALID_PORT_NUMBER;
    uc.lpszUserName = userName;
    uc.dwUserNameLength = sizeof(userName);
    uc.lpszPassword = password;
    uc.dwPasswordLength = sizeof(password);
    uc.lpszUrlPath = urlPath;
    uc.dwUrlPathLength = sizeof(urlPath);
    
    bRet = InternetCrackUrlA(pszPostURL, lstrlenA(pszPostURL), 
                            ICU_DECODE, &uc);
    if (!bRet)
    {
        DBG("InternetCrackUrl failed");
        goto _exitInit;
    }

    // read User Agent string
    if (!ReadRegValue(HKEY_CURRENT_USER, szInternetSettings, c_szUserAgent, pszUA, sizeof(pszUA)))
        StrCpy(pszUA, TEXT("PostAgent"));

    _hSession = InternetOpen(pszUA,                  // used in User-Agent: header 
                            INTERNET_OPEN_TYPE_PRECONFIG,  //INTERNET_OPEN_TYPE_DIRECT, 
                            NULL,
                            NULL, 
                            //INTERNET_FLAG_ASYNC);  
                            0);         //synchronous operation

    if ( !_hSession )
    {
        DBG("!_hSession");
        goto _exitInit;
    }

    _hHttpSession = InternetConnectA(_hSession, 
                                    uc.lpszHostName,    //"peihwalap", 
                                    uc.nPort,           //INTERNET_INVALID_PORT_NUMBER,
                                    uc.lpszUserName,    //NULL, 
                                    uc.lpszPassword,    //NULL, 
                                    INTERNET_SERVICE_HTTP, 
                                    INTERNET_FLAG_KEEP_CONNECTION, 
                                    0); 
                                    //(DWORD)this); //dwContext.

//    InternetSetStatusCallback(m_hSession, CWCPostAgent::PostCallback);
                                
    if ( !_hHttpSession )
    {
        DBG( "!_hHttpSession");
        CloseRequest();
        goto _exitInit;
    }                                    
    

    // ignore security problem

    _hHttpRequest = HttpOpenRequestA(_hHttpSession, lpszVerb, 
                                    uc.lpszUrlPath,
                                    HTTP_VERSIONA, 
                                    NULL,                             //lpszReferer
                                    NULL,                             //lpszAcceptTyypes
                                    //INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                                    //INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                                    //INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
                                    //INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
                                    INTERNET_FLAG_NO_COOKIES, 
                                    0);
                                    //(DWORD)this);	//dwContext
                            

    if ( !_hHttpRequest )
    {
        DBG_WARN("Post Agent: !_hHttpRequest");
        CloseRequest();
        goto _exitInit;
    }
    else
        hr = S_OK;


_exitInit:

    delete [] pszPostURL;
    return hr;
    
}                                                                
    
HRESULT CWCPostAgent::SendRequest
(
    LPCSTR     lpszHeaders,
    LPDWORD    lpdwHeadersLength,
    LPCSTR     lpszOption,
    LPDWORD    lpdwOptionLength
)
{
    BOOL bRet=FALSE;

    HttpAddRequestHeadersA(_hHttpRequest, 
                           (LPCSTR)c_szHeaders, 
                           (DWORD)-1L, 
                           HTTP_ADDREQ_FLAG_ADD);

    if (lpszHeaders && *lpszHeaders)        // don't bother if it's empty
        bRet = HttpAddRequestHeadersA(_hHttpRequest, 
                          (LPCSTR)lpszHeaders, 
                          *lpdwHeadersLength, 
                          HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
/*
    bRet = HttpSendRequest(_hHttpRequest, 
                          (LPCTSTR)lpszHeaders,           //HEADER_ENCTYPE, 
                          *lpdwHeadersLength,              //sizeof(HEADER_ENCTYPE), 
                          (LPVOID)lpszOption, 
                          *lpdwOptionLength);
*/
    bRet = HttpSendRequest(_hHttpRequest, 
                          NULL,                            //HEADER_ENCTYPE, 
                          0,                               //sizeof(HEADER_ENCTYPE), 
                          (LPVOID)lpszOption, 
                          *lpdwOptionLength);

    if ( !bRet )
    {
        DWORD dwLastError = GetLastError();

        TraceMsg(TF_ERROR, "HttpSendRequest failed: Error = %lx", dwLastError);
        DBG_WARN("Post Agent: HttpSendRequest failed");
        return E_FAIL;
    }

    DWORD dwBuffLen;
    TCHAR buff[1024];

    dwBuffLen = sizeof(buff);

    bRet = HttpQueryInfo(_hHttpRequest,
                        //HTTP_QUERY_CONTENT_TYPE,  //HTTP_QUERY_REQUEST_METHOD,
                        HTTP_QUERY_STATUS_CODE,   //HTTP_QUERY_RAW_HEADERS,
                        buff,
                        &dwBuffLen,
                        NULL);
 
    //verify request response here
    //
    int iretcode = StrToInt(buff);
    if (iretcode == 200)   // || iretcode == 100)   //HTTP_STATUS_OK
        return S_OK;             //100: too many semaphors
                                 //501: required not supported
                                 //502: bad gateway
    return E_FAIL;
    
}                                                                

HRESULT CWCPostAgent::CloseRequest
(
    void
)
{
    InternetCloseHandle(_hSession);
    _hSession = _hHttpSession = _hHttpRequest = 0;
    return S_OK;
}

// Called if upload failed
//   just increase retry number
HRESULT CWCPostAgent::OnPostFailed()
{
    WriteDWORD(_pCDFItem, c_szPostingRetry, _pUploadStream->_dwRetry+1);

    MergeGroupOldToNew();

    return S_OK;
}

// Called if upload succeeded
//   1) remove PostUrl from item
//   2) delete tracking cache entry  (Doh!)
HRESULT CWCPostAgent::OnPostSuccess()
{
    GROUPID newId = 0;

    if (!_pCDFItem)
        return E_INVALIDARG;

    ReadLONGLONG(_pCDFItem, c_szTrackingCookie, &newId);
    _pUploadStream->RemoveCacheEntry(_pUploadStream->_groupId);

    if (newId == _pUploadStream->_groupId)
    {
        BSTR  bstrURL = NULL;
        ReadBSTR(_pCDFItem, c_szTrackingPostURL, &bstrURL);
        if (!StrCmpIW(bstrURL, _pUploadStream->_pwszURL))
        {
            WriteOLESTR(_pCDFItem, c_szTrackingPostURL, bstrURL);
        }

        SAFEFREEBSTR(bstrURL);

    }
    else
        _pUploadStream->EmptyCacheEntry(newId);

    return S_OK;
}

HRESULT
CWCPostAgent::FindCDFStartItem()
{
    IServiceProvider    *pSP;
    
    if (_pCDFItem)
        return S_OK;
    
    if (SUCCEEDED(m_pAgentEvents->QueryInterface(IID_IServiceProvider, (void **)&pSP)) && pSP)
    {
        pSP->QueryService(CLSID_ChannelAgent, IID_ISubscriptionItem, (void **)&_pCDFItem);
        pSP->Release();
    }

    if (NULL == _pCDFItem)
    {
        TraceMsg(TF_THISMODULE, "PostAgent : FindCDFStartItem failed");
    }

    return (_pCDFItem) ? S_OK : E_NOINTERFACE;
}

HRESULT CWCPostAgent::DoFileUpload()
{
    HRESULT hr;
    DWORD   dwLen, dwHdr;

    SAFELOCALFREE(_pszPostStream);
    dwLen = _pUploadStream->ReadLogFile(_lpLogFile, &_pszPostStream);
    if (dwLen == 0)
    {
        // no log to post, should clean up cache entries for this group.
        OnPostSuccess();
        return S_OK;
    }
  
    if (FAILED(hr = InitRequest("POST")))
        return hr;

#ifdef ENCODE_LOG
    CHAR   lpEncodeHdr[c_ccEncodeHeader+MAX_PATH];

    lpEncodeHdr[0] = '\0';
    if (SUCCEEDED(ReadBSTR(m_pSubscriptionItem, c_szPostHeader, &_pwszEncoding)))
    {
        if (_pwszEncoding && *_pwszEncoding)
        {
            IEncodingFilterFactory* pEflt = NULL;
            IDataFilter*    pDF = NULL;
                 
            CoCreateInstance(CLSID_StdEncodingFilterFac, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IEncodingFilterFactory, (void**)&pEflt);

            if (pEflt)
            {
                pEflt->GetDefaultFilter(_pwszEncoding, L"text", &pDF);
                if (pDF)
                {
                    LONG lInUsed = 0;
                    LONG lOutUsed = 0;
                    LPSTR  pInBuf = _pszPostStream;
                    LPSTR  pOutBuf = NULL;
                    HRESULT hrCode = NOERROR;

                    pOutBuf = (LPSTR)MemAlloc(LPTR, dwLen);
                    if (pOutBuf == NULL)
                        goto do_upload;

                    hrCode = pDF->DoEncode(
                                            0,
                                            dwLen,
                                            (BYTE *)pInBuf, 
                                            dwLen,
                                            (BYTE *)pOutBuf, 
                                            dwLen,
                                            &lInUsed,
                                            &lOutUsed,
                                            0);

                    if (SUCCEEDED(hrCode))
                    {
                        // add encoding header information, e.g.
                        // "Content-Transfer-Encoding: x-gzip\r\n"
                        wnsprintfA(lpEncodeHdr, ARRAYSIZE(lpEncodeHdr), "%s%S\r\n", 
                                   c_szEncodeHeader, _pwszEncoding);
                        
                        SAFELOCALFREE(_pszPostStream);
                        _pszPostStream = (LPSTR)MemAlloc(LPTR, lOutUsed+2);
                        ASSERT(_pszPostStream);
                        memcpy(_pszPostStream, pOutBuf, lOutUsed);  //do I need to append CR?
                        dwLen = (DWORD)lOutUsed;
                    }
                    pDF->Release();
                    SAFELOCALFREE(pOutBuf);
                }
                pEflt->Release();
            }
        }   // if (_pwszEncoding && *_pwszEncoding)

    }   //ReadBSTR


do_upload:
    dwHdr = lstrlenA(lpEncodeHdr);
    hr = SendRequest(lpEncodeHdr, &dwHdr, _pszPostStream, &dwLen);
#else
    dwHdr = lstrlenA("");
    hr = SendRequest(NULL, &dwHdr, _pszPostStream, &dwLen);
#endif  //ENCODE_LOG

    CloseRequest();

    if (FAILED(hr))
    {
        TraceMsg(TF_THISMODULE, "PostAgent failed to SendRequest");
        OnPostFailed();
    }
    else
    {
        TraceMsg(TF_THISMODULE, "PostAgent Posted data");
        OnPostSuccess();
    }


    return hr;
}

BOOL CWCPostAgent::GetTmpLogFileName()
{
    char   szPath[MAX_PATH];
    
    if (GetTempPathA(MAX_PATH, szPath) > 0)
    {
        _lpLogFile = (LPSTR)MemAlloc(LPTR, MAX_PATH+1);
        if (GetTempFileNameA(szPath, "trk", 0, _lpLogFile))
        {
            return TRUE;
        }
    }

    return FALSE;
}
    
HRESULT CWCPostAgent::DoPost()
{
    HRESULT hr = E_FAIL;
    
    _pUploadStream->_groupId = 0L;
    if (FAILED(ReadLONGLONG(m_pSubscriptionItem, c_szTrackingCookie, &_pUploadStream->_groupId)))
        return hr;

    if (FAILED(ReadDWORD(m_pSubscriptionItem, c_szPostingRetry, &_pUploadStream->_dwRetry)))
        _pUploadStream->_dwRetry = 0;

    if (_pUploadStream->IsExpired(m_pSubscriptionItem))
    {
        // post is expired, clean up log cache entries
        OnPostSuccess();
        return hr;
    }

    if (!GetTmpLogFileName())
    {
        if (NULL != _lpLogFile)
        {
            MemFree(_lpLogFile);
        }
        _lpLogFile = (LPSTR)MemAlloc(LPTR, (MAX_PATH + 1) * sizeof(CHAR));
        if (_lpLogFile)
        {
            lstrcpyA(_lpLogFile, "trkad.tmp");
        }
    }
        
    if (FAILED(hr = _pUploadStream->RetrieveAllLogStream(_pCDFItem, _lpLogFile)))
        return hr;

    hr = DoFileUpload();

    if (_lpLogFile)
        DeleteFileA(_lpLogFile);
    
    return hr;
}

// OnInetOnline
HRESULT CWCPostAgent::StartDownload()
{
    HRESULT hr;
    ASSERT(_pUploadStream && _pUploadStream->_pwszURL);

    hr = FindCDFStartItem();

    if (SUCCEEDED(hr))
    {
        BSTR bstrChannelURL = NULL;
        hr = ReadBSTR(m_pSubscriptionItem, c_szPropURL, &bstrChannelURL);
        ASSERT(SUCCEEDED(hr) && *bstrChannelURL);
        if (SUCCEEDED(hr) && !SHRestricted2W(REST_NoChannelLogging, bstrChannelURL, 0)
            && !ReadRegDWORD(HKEY_CURRENT_USER, c_szRegKey, c_szNoChannelLogging))
        {
            hr = DoPost();
        }
        else
        {
            OnPostSuccess();                // log is off, clean up log cache entries
        }
        SAFEFREEBSTR(bstrChannelURL);
    }
    SAFERELEASE(_pCDFItem);

    SetEndStatus(S_OK);

    CleanUp();

    return S_OK;
}

// OnAgentStart
HRESULT CWCPostAgent::StartOperation()
{
    if (_pUploadStream)
    {
        DBG_WARN("Agent busy, returning failure");
        SetEndStatus(E_FAIL);
        SendUpdateNone();
        return E_FAIL;
    }

    _pUploadStream = NULL;
    _pUploadStream = new CTrackingStg();
    if (!_pUploadStream)
    {
        DBG("failed to allocate CTrackStg");
        SetEndStatus(E_OUTOFMEMORY);
        SendUpdateNone();
        return E_OUTOFMEMORY;
    }

    SAFEFREEOLESTR(_pUploadStream->_pwszURL);
    if (FAILED(ReadOLESTR(m_pSubscriptionItem, c_szTrackingPostURL, &_pUploadStream->_pwszURL)) ||
        !CUrlDownload::IsValidURL(_pUploadStream->_pwszURL))
    {
        DBG_WARN("Couldn't get valid URL from start item (aborting)");
        SetEndStatus(E_INVALIDARG);
        SendUpdateNone();
        return E_INVALIDARG;
    }

    // After calling this, we'll reenter either in "StartDownload" or in 
    // "AbortUpdate" with m_scEndStatus = E_ACCESSDENIED
    return CDeliveryAgent::StartOperation();
}

//------------------------------------------------------------
//
// override CDeliveryAgent virtual functions
//
//------------------------------------------------------------

// OnInetOffline
// Forcibly abort current operation
HRESULT CWCPostAgent::AgentAbort(DWORD dwFlags)
{
    DBG("CWCPostAgent::AbortUpdate");

    if (_pUploadStream)
    {
        if (SUCCEEDED(FindCDFStartItem()))
        {
            OnPostFailed();
        }
    }

    return CDeliveryAgent::AgentAbort(dwFlags);
}

void CWCPostAgent::CleanUp()
{
    SAFEDELETE(_pUploadStream);

    if (_lpLogFile)
        DeleteFileA(_lpLogFile);

    SAFEFREEBSTR(_pwszEncoding);

    SAFELOCALFREE(_lpLogFile);
    SAFELOCALFREE(_pszPostStream);
    
    CDeliveryAgent::CleanUp();
}

//------------------------------------------------------------
//
// CWCPostAgent Constructor/D'Constr
//
//------------------------------------------------------------

CWCPostAgent::CWCPostAgent()
{
    DBG("Creating CWCPostAgent object");

    //
    // Maintain global count of objects in webcheck.dll
    //
    DllAddRef();

    //
    // Initialize object member variables
    //
    _pUploadStream = NULL;
    _pwszEncoding = NULL;
    _pszPostStream = NULL;
    _lpLogFile = NULL;
    _hSession = _hHttpSession = _hHttpRequest = 0;
}

CWCPostAgent::~CWCPostAgent()
{
    SAFERELEASE(_pCDFItem);

    //
    // Maintain global count of objects
    //
    DllRelease();

    //
    // Release/delete any resources
    //
    DBG("Destroyed CWCPostAgent object");
}

