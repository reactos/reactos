// 
// Pei-Hwa Lin (peiwhal), Feb 3, 1997
//
//  Notes:
//   Compile switch : NO_FILE_WHEN_CREATE can turn on when wininet cache
//                    would create url cache entry if attached file is empty.
//                    LOG_CACHE_PATH will create log cache at the same level as
//                    content cache.
#include "private.h"

#undef TF_THISMODULE
#define TF_THISMODULE    TF_TRACKCACHE


#define MY_CACHE_FILE_ENTRY_SIZE     2048

const TCHAR c_szExt[] = TEXT("log");

//used for creating tracking container (registry)
const TCHAR c_szLogContainer[] = TEXT("Log");        // if you modify this, modify iedev\inc\inetreg.h REGSTR_PATH_TRACKING too.
const TCHAR c_szLogPrefix[] = TEXT("Log:");

const char  c_szLogContainerA[] = "Log";

// helper function
inline BOOL IsNumber(WCHAR x) { return (x >= L'0' && x <= L'9'); }

/*=============================================================================
 FILEFROMPATH returns the filename of given filename which may include path.
=============================================================================*/
LPTSTR FileFromPath( LPCTSTR lpsz )
{
   LPTSTR lpch;

   /* Strip path/drive specification from name if there is one */
   lpch = CharPrev( lpsz, lpsz + lstrlen(lpsz) );

   // special case for "http://server/domain/channel.cdf/"
   if (*lpch == '/') lpch = CharPrev( lpsz, lpch);

   while (lpch > lpsz)
   {
      if (*lpch == '/') {
         lpch = CharNext(lpch);
         break;
      }
      lpch = CharPrev( lpsz, lpch);
   }
   return(lpch);

} /* end FileFromPath */

// CDF updates, create new group and deal with previous posting information
void
CUrlTrackingCache :: Init(LPCWSTR pwszURL)
{
    DWORD   dwRetry;
    BSTR    bstrEncoding = NULL;

    _groupId = 0;
    _pwszPostUrl = NULL;
    _pszChannelUrlSite = NULL;
    _pszPostUrlSite = NULL;
    _pwszEncodingMethod = NULL;
    
    _groupId = CreateUrlCacheGroup(CACHEGROUP_FLAG_GIDONLY, NULL);

    WriteLONGLONG(_pCDFStartItem, c_szTrackingCookie, _groupId);

    // #54653: remove previous tracking information, if any
    if (SUCCEEDED(ReadBSTR(_pCDFStartItem, c_szPostHeader, &bstrEncoding)))
    {
        WriteEMPTY(_pCDFStartItem, c_szPostHeader);
        SAFEFREEBSTR(bstrEncoding);
    }

    if (SUCCEEDED(ReadDWORD(_pCDFStartItem, c_szPostingRetry, &dwRetry)))
        WriteEMPTY(_pCDFStartItem, c_szPostingRetry);

    DoBaseURL(pwszURL);
    return;
}

void
CUrlTrackingCache :: DoBaseURL(LPCWSTR pwszURL)
{
    DWORD  cbLen;    
    DWORD  useSecurity = 1;

    ASSERT(!_pszChannelUrlSite)

    cbLen = (lstrlenW(pwszURL)+1) * sizeof(WCHAR);
    _pszChannelUrlSite = (LPTSTR)MemAlloc( LPTR, cbLen);


#ifdef DEBUG

    HKEY hkey;

    // provide security switch for debugging
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        MY_WEBCHECK_POST_REG,
                                        0,
                                        KEY_READ,
                                        &hkey))
    {
        DWORD cbsize = sizeof(DWORD);
        
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, 
                                        TEXT("PostSecurity"), 
                                        NULL, 
                                        NULL, 
                                        (LPBYTE)&useSecurity, 
                                        &cbsize))
        {
            if ((useSecurity == 0) && (NULL != _pszChannelUrlSite))
                StrCpy(_pszChannelUrlSite, TEXT("http://"));
        }
    }

#endif
    
    if ((useSecurity == 1) && (NULL != _pszChannelUrlSite))
    {
        MyOleStrToStrN(_pszChannelUrlSite, cbLen, pwszURL);
        *(FileFromPath( _pszChannelUrlSite )) = 0;
    }
    return;
}

// only track URLs come from the same server of Channel CDF or LogTarget URL
//
BOOL
CUrlTrackingCache :: IsValidURL(LPCTSTR lpszURL)
{
    BOOL    bret;

    if (!_pszChannelUrlSite || !_pszPostUrlSite)
        return FALSE;
       
    if (!StrCmpNI(lpszURL, _pszChannelUrlSite, lstrlen(_pszChannelUrlSite)))
        bret = TRUE;
    else if (!StrCmpNI(lpszURL, _pszPostUrlSite, lstrlen(_pszPostUrlSite)))
        bret = TRUE;
    else
        bret = FALSE;

    return bret;
}

#define LOG_CACHE_PATH
#ifdef LOG_CACHE_PATH
            
LPSTR PathPreviousBackslashA(LPSTR psz)
{
    LPSTR lpch = CharPrevA(psz, psz + lstrlenA(psz));
    for (; *lpch && *lpch != '\\'; lpch=CharPrevA(psz,lpch));
        
    return lpch;
}

//------------------------------------------------------------------------------
// GetCacheLocation
//
// Purpose:     Return the location of the logging cache
//    *****     GetUrlCacheConfigInfoW is yet implemented in wininet
//------------------------------------------------------------------------------
//
HRESULT GetCacheLocation
(
    LPTSTR  pszCacheLocation,
    DWORD   dwSize          // no. of chars in pszCacheLocation
)
{
    HRESULT hr = S_OK;
    DWORD dwLastErr;
    LPINTERNET_CACHE_CONFIG_INFOA lpCCI = NULL;
    DWORD dwCCISize = sizeof(INTERNET_CACHE_CONFIG_INFOA);
    BOOL fOnceErrored = FALSE;

    while (TRUE)
    {
        if ((lpCCI = (LPINTERNET_CACHE_CONFIG_INFOA)MemAlloc(LPTR,
                                                        dwCCISize)) == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        if (!GetUrlCacheConfigInfoA(lpCCI, &dwCCISize,
                                            CACHE_CONFIG_CONTENT_PATHS_FC))
        {
            if ((dwLastErr = GetLastError()) != ERROR_INSUFFICIENT_BUFFER  ||
                fOnceErrored)
            {
                hr = HRESULT_FROM_WIN32(dwLastErr);
                goto cleanup;
            }

            //
            // We have insufficient buffer size; reallocate a buffer with the
            //      new dwCCISize set by GetUrlCacheConfigInfo
            // Set fOnceErrored to TRUE so that we don't loop indefinitely
            //
            fOnceErrored = TRUE;
        }
        else
        {
            // 
            LPSTR pszPath = lpCCI->CachePaths[0].CachePath;
            INT iLen;

            PathRemoveBackslashA(pszPath);
            *(PathPreviousBackslashA(pszPath)) = 0;
            iLen = lstrlenA(pszPath) + sizeof(CHAR);        // + 1 is for the null char

            if ((((DWORD) iLen + ARRAYSIZE(c_szLogContainer) + 1) * sizeof(TCHAR)) < dwSize)
            {
                TCHAR szPathT[MAX_PATH];

                SHAnsiToTChar(pszPath, szPathT, ARRAYSIZE(szPathT));
                wnsprintf(pszCacheLocation, dwSize, TEXT("%s\\%s"), szPathT, c_szLogContainer);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }

            break;
        }

        SAFELOCALFREE(lpCCI);
        lpCCI = NULL;
    }

cleanup:
    if (lpCCI != NULL)
    {
        LocalFree(lpCCI);
    }

    return hr;
}
#endif
//-----------------------------------------------------------------------------
//
// ReadTrackingPrefix
//
// will create tracking container if current profile doesn't have one
// **** FindFirstUrlCacheContainerW is yet implemented in wininet
// **** FindNextUrlCacheContainerW is yet implemented either
//-----------------------------------------------------------------------------
LPTSTR
ReadTrackingPrefix(void)
{
    LPTSTR  lpPfx = NULL;

    DWORD   cbPfx = 0;
    struct {
        INTERNET_CACHE_CONTAINER_INFOA cInfo;
        CHAR  szBuffer[MAX_PATH+INTERNET_MAX_URL_LENGTH+1];
    } ContainerInfo;
    DWORD   dwModified, dwContainer;
    HANDLE  hEnum;
  
    dwContainer = sizeof(ContainerInfo);
    hEnum = FindFirstUrlCacheContainerA(&dwModified,
                                       &ContainerInfo.cInfo,
                                       &dwContainer,
                                       0);

    if (hEnum)
    {

        for (;;)
        {
            if (!StrCmpIA(ContainerInfo.cInfo.lpszName, c_szLogContainerA))
            {
                ASSERT(ContainerInfo.cInfo.lpszCachePrefix[0]);

                CHAR    szPfx[MAX_PATH];
                DWORD   cch = ARRAYSIZE(ContainerInfo.cInfo.lpszCachePrefix)+sizeof(CHAR);
                StrCpyNA(szPfx, ContainerInfo.cInfo.lpszCachePrefix, cch);

                cch *= sizeof(TCHAR);
                lpPfx = (LPTSTR)MemAlloc(LPTR, cch);
                if (!lpPfx)
                    SetLastError(ERROR_OUTOFMEMORY);

                SHAnsiToTChar(szPfx, lpPfx, cch);
                break;
            }

            dwContainer = sizeof(ContainerInfo);
            if (!FindNextUrlCacheContainerA(hEnum, &ContainerInfo.cInfo, &dwContainer))
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }

        }

        FindCloseUrlCache(hEnum);
    }

    if (!lpPfx)
    {
        LPTSTR pszCachePath = NULL;
#ifdef LOG_CACHE_PATH
        TCHAR szCachePath[MAX_PATH];

        pszCachePath = (SUCCEEDED(GetCacheLocation(szCachePath, MAX_PATH))) ?
                                  szCachePath : NULL;
#endif

        if (CreateUrlCacheContainer(c_szLogContainer, 
                  c_szLogPrefix, 
                  pszCachePath, // wininet bug:if NULL, will create under ..\History\Log //
                  8192,       // dwCacheLimit,
                  INTERNET_CACHE_CONTAINER_NOSUBDIRS,          // dwContainerType,
                  0,          // dwOptions,
                  NULL,       // pvBuffer,
                  0           // cbBuffer
                    ))
        {
            return ReadTrackingPrefix();
        }
        // unable to create Log container, stop.
    }
     
    return lpPfx;
}

//-----------------------------------------------------------------------------
//
// ConvertToPrefixedUrl
//
// caller must release lplpPrefixedUrl
//-----------------------------------------------------------------------------
BOOL
CUrlTrackingCache :: ConvertToPrefixedUrl
(
    IN LPCTSTR lpUrl, 
    IN LPTSTR* lplpPrefixedUrl
)
{
    BOOL    bret = FALSE;

    ASSERT(lpUrl);
    if (!_lpPfx)
        _lpPfx = ReadTrackingPrefix();

    if (_lpPfx)
    {
        int len = lstrlen(lpUrl) + lstrlen(_lpPfx) + 1;
        
        *lplpPrefixedUrl = NULL;
        
        *lplpPrefixedUrl = (LPTSTR)MemAlloc(LPTR, len * sizeof(TCHAR));
        if (*lplpPrefixedUrl)
        {
            wnsprintf(*lplpPrefixedUrl, len, TEXT("%s%s"), _lpPfx, lpUrl);
            bret = TRUE;
        }
        else
            bret = FALSE;
    }

    return bret;
}

//-----------------------------------------------------------------------------
//
// RetrieveUrlCacheEntry
//
// caller must release lpCE
//-----------------------------------------------------------------------------
LPINTERNET_CACHE_ENTRY_INFO 
CUrlTrackingCache :: RetrieveUrlCacheEntry
(
    IN  LPCTSTR     lpUrl
)
{
    LPINTERNET_CACHE_ENTRY_INFO   lpCE = NULL;
    DWORD          cbSize;
    BOOL           bret = FALSE;

    lpCE = (LPINTERNET_CACHE_ENTRY_INFO)MemAlloc(LPTR, MY_MAX_CACHE_ENTRY_INFO);
    if (lpCE)
    {
        cbSize = MY_MAX_CACHE_ENTRY_INFO;
        while ((bret = GetUrlCacheEntryInfo(lpUrl, lpCE, &cbSize)) != TRUE)
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                SAFELOCALFREE(lpCE);

                lpCE = (LPINTERNET_CACHE_ENTRY_INFO)MemAlloc(LPTR, cbSize);
                if (!lpCE)
                   break;
            }
            else
                break;
        }
    }

    if (!bret && lpCE)
    {
        SAFELOCALFREE(lpCE);
        SetLastError(ERROR_FILE_NOT_FOUND);
    }

    return lpCE;
}

#define FILETIME_SEC   100000000
#define SECS_PER_DAY   (60 * 60 * 24)

DWORD WCTOI(LPCWSTR pwstr)
{
    DWORD   dw;
    int     len = lstrlenW(pwstr);

    dw = 0;
    for (int i = 0; i<len; i++)
    {
        if (!IsNumber(pwstr[i]))
            break;

        dw = dw * 10 + (pwstr[i] - L'0');
    }

    if (dw == 0) dw = 24;
    return dw;
}

//-----------------------------------------------------------------------------
//
// CreatePrefixedCacheEntry
//
// Create cache entry in Tracking cache bucket
//-----------------------------------------------------------------------------
#ifdef NO_FILE_WHEN_CREATE
HRESULT CreateLogCacheEntry
(
    LPCTSTR  lpPfxUrl, 
    FILETIME ftExpire, 
    FILETIME ftModified,
    DWORD    CacheEntryType
)
{
    TCHAR   lpFile[MAX_PATH];
    BOOL    bret;
    HRESULT hr = E_FAIL;

    if (CreateUrlCacheEntry(lpPfxUrl, MY_CACHE_FILE_ENTRY_SIZE, c_szExt, lpFile, 0))
    {
        lpFile[0] = '\0';
        bret = CommitUrlCacheEntry(lpPfxUrl,
                                   lpFile, 
                                   ftExpire, 
                                   ftModified, 
                                   CacheEntryType,
                                   NULL,
                                   0,
                                   NULL,
                                   0);

        hr = bret ? S_OK : E_FAIL;
    }

    return hr;
}
#else
HRESULT CreateLogCacheEntry
(
    LPCTSTR  lpPfxUrl, 
    FILETIME ftExpire, 
    FILETIME ftModified,
    DWORD    CacheEntryType
)
{
    TCHAR   lpFile[MAX_PATH];
    HRESULT hr = E_FAIL;
    DWORD      cbSize;

    if (CreateUrlCacheEntry(lpPfxUrl, MY_CACHE_FILE_ENTRY_SIZE, c_szExt, lpFile, 0))
    {
        HANDLE hFile = CreateFile(lpFile,
                                    GENERIC_READ|GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            return hr;
              
        // note: wininet will not create the entry if file size equals to zero
//            WriteFile(hFile, c_szLogFields, g_ccLogFields, &cbSize, NULL);

        WriteFile(hFile, c_szEmptyLog, c_ccEmptyLog, &cbSize, NULL);
        CloseHandle(hFile);

        return (CommitUrlCacheEntry(lpPfxUrl, 
                                   lpFile, 
                                   ftExpire, 
                                   ftModified, 
                                   CacheEntryType,
                                   NULL,
                                   0,
                                   NULL,
                                   0)) ? S_OK : E_FAIL;
    }

    return hr;
}
#endif

HRESULT
CUrlTrackingCache :: CreatePrefixedCacheEntry
(
    IN LPCTSTR lpPfxUrl
)
{
    HRESULT     hr = E_FAIL;
    LPINTERNET_CACHE_ENTRY_INFO    lpCE = NULL;
    FILETIME    ftModified;

    // IE50: due to change to wininet cache group enumeration, now we save our filter
    // information _groupId along with each cache entry itself.  the wininet url cache
    // no longer maintain this for us
    ftModified.dwHighDateTime = (DWORD)(_groupId >> 32);
    ftModified.dwLowDateTime = (DWORD)(0x00000000ffffffff & _groupId);

    lpCE = RetrieveUrlCacheEntry(lpPfxUrl);
    if (lpCE ) 
    {
        // exist in Tracking bucket, set tracking flag
        // IE50: save _groupId info in LastModifiedTime
        lpCE->CacheEntryType |= _ConnectionScope;
        lpCE->LastModifiedTime.dwHighDateTime = ftModified.dwHighDateTime;
        lpCE->LastModifiedTime.dwLowDateTime = ftModified.dwLowDateTime;

        ASSERT(SetUrlCacheEntryInfo(lpCE->lpszSourceUrlName, lpCE, 
                             CACHE_ENTRY_ATTRIBUTE_FC | CACHE_ENTRY_MODTIME_FC) == TRUE);
        hr = S_OK;
    }
    else
    //FILE_NOT_FOUND, create it.
    {
        LONGLONG llExpireHorizon;     
        SYSTEMTIME  st;
        FILETIME ftMod, ftExpire;

        llExpireHorizon = (LONGLONG)(SECS_PER_DAY * _dwPurgeTime / 24);

        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &ftMod);

        llExpireHorizon *= FILETIME_SEC;
        ftExpire.dwLowDateTime = ftMod.dwLowDateTime + (DWORD)(llExpireHorizon % 0xFFFFFFFF);
        ftExpire.dwHighDateTime = ftMod.dwHighDateTime + (DWORD)(llExpireHorizon / 0xFFFFFFFF);

        hr = CreateLogCacheEntry(lpPfxUrl, ftExpire, ftModified, _ConnectionScope);

    }
        
    SAFELOCALFREE(lpCE);

    return hr;
}

HRESULT
CUrlTrackingCache :: AddToTrackingCacheEntry
(
    IN LPCWSTR  pwszUrl
)
{
    HRESULT     hr = E_OUTOFMEMORY;
    TCHAR       szCanonicalUrl[MAX_URL];
    DWORD       dwSize = MAX_URL;
    LPTSTR      lpUrl = NULL;
    LPTSTR      lpPfxUrl = NULL;
    DWORD       cbSize;
    PROPVARIANT vProp = {0};

    if (pwszUrl == NULL)
        return E_INVALIDARG;

    cbSize = lstrlenW(pwszUrl) + 1;
    lpUrl = (LPTSTR)MemAlloc(LPTR, cbSize * sizeof(TCHAR));
    if (!lpUrl)
        return hr;

    SHUnicodeToTChar(pwszUrl, lpUrl, cbSize);
    if (!IsValidURL(lpUrl))
    {
        MemFree(lpUrl);
        return E_INVALIDARG;
    }        

    // canonicalize URL
    InternetCanonicalizeUrl(lpUrl, szCanonicalUrl, &dwSize, ICU_DECODE);
    SAFELOCALFREE(lpUrl);
    ConvertToPrefixedUrl(szCanonicalUrl, &lpPfxUrl);
    if (!lpPfxUrl)
    {
        return hr;
    }

    hr = CreatePrefixedCacheEntry(lpPfxUrl);
    if (SUCCEEDED(hr))
    {
        // exist in Tracking bucket, set tracking flag.
        vProp.vt = VT_UI4;
        vProp.ulVal = _ConnectionScope;
        hr = IntSiteHelper(szCanonicalUrl, &c_rgPropRead[PROP_TRACKING], &vProp, 1, TRUE);
        PropVariantClear( &vProp );        
    }

    SAFELOCALFREE(lpPfxUrl);
    return hr;
}

//-----------------------------------------------------------------------------
//
// SchedulePostAgent
//
//  This routine will schedule post agent to upload tracking data
//-----------------------------------------------------------------------------
HRESULT
CUrlTrackingCache :: SchedulePostAgent(void)
{
    return E_NOTIMPL;
}


//-----------------------------------------------------------------------------
//
// OnProcessDone
//
//-----------------------------------------------------------------------------

// called by CDF agent
// fill information in item that Post agent would need to work
HRESULT
CUrlTrackingCache :: OnProcessDone
(
)
{
    return E_NOTIMPL;
}


//-----------------------------------------------------------------------------
//
// Process log related tags
//
//-----------------------------------------------------------------------------
//
// <LOGTARGET href="http://foo.htm" SCOPE="ALL"/>
//  <HTTP-EQUIV name="Encoding-type" value="gzip" />
//  <PurgeTime HOUR="12" />
// </Logtarget>
//
HRESULT
CUrlTrackingCache :: ProcessTrackingInLog
(
    IXMLElement     *pTracking
)
{

    HRESULT hr;
    LPWSTR  pwszScope = NULL;
    
    if (_pwszPostUrl)
        return S_OK;        // there are more than 1 logtarget, take whatever first was read

    hr = ReadAttribute(pTracking, L"HREF", &_pwszPostUrl);       // must exist to enalbe logging
    if (FAILED(hr))
        return hr;

    // fill it in item for post agent
    WriteOLESTR(_pCDFStartItem, c_szTrackingPostURL, _pwszPostUrl);

    // #41460: add 2nd domain allowing tracking to
    DWORD   cbLen = (lstrlenW(_pwszPostUrl)+1) * sizeof(WCHAR);
    _pszPostUrlSite = (LPTSTR)MemAlloc( LPTR, cbLen);
    MyOleStrToStrN(_pszPostUrlSite, cbLen, _pwszPostUrl);
    *(FileFromPath( _pszPostUrlSite )) = 0;


    _ConnectionScope = TRACK_ONLINE_CACHE_ENTRY | TRACK_OFFLINE_CACHE_ENTRY;
    hr = ReadAttribute(pTracking, L"SCOPE", &pwszScope);
    if (SUCCEEDED(hr))
    {     
        if (!StrCmpIW(pwszScope, L"OFFLINE"))
            _ConnectionScope = TRACK_OFFLINE_CACHE_ENTRY;            
        else if (!StrCmpIW(pwszScope, L"ONLINE"))                    
            _ConnectionScope = TRACK_ONLINE_CACHE_ENTRY;

        SAFELOCALFREE(pwszScope);
    }

    RunChildElement(pTracking);

    // #42687: save purgetime to item and used later by post agent
    if (_pwszPurgeTime)     // if not specify, default is 24 hours
    {
        _dwPurgeTime = WCTOI(_pwszPurgeTime);
    }

    DATE        dt = 0.0;
    SYSTEMTIME  st;

    GetLocalTime(&st);
    SystemTimeToVariantTime(&st, &dt);
    dt += ((DATE)_dwPurgeTime/24);
#ifdef DEBUG
    VariantTimeToSystemTime(dt, &st);
#endif
    WriteDATE(_pCDFStartItem, c_szPostPurgeTime, &dt);

    return S_OK;    
}

//-----------------------------------------------------------------------------
//
// ProcessTrackingItems
//  <Item href="http://foo">
//    <Log value="document:view"/>
//  </Item>
// or <Item>
//    <A href="http://foo" />
//  </Item>
//  This routine will setup tracking cache entries for all URLs which are
//  specified in CDF file to track.  All URLs entries belong to same channel 
//  are created in same cache group
//-----------------------------------------------------------------------------
HRESULT
CUrlTrackingCache :: ProcessTrackingInItem
(
    IXMLElement     *pItem,                 //point to <Item> tag
    LPCWSTR         pwszUrl,                //absolute URL for item
    BOOL            fForceLog               //global log flag
)
{
    HRESULT hr = S_OK;

    _bTrackIt = fForceLog;

    if (!_bTrackIt)
        hr = RunChildElement(pItem);

    if (SUCCEEDED(hr) && _bTrackIt)
        hr = AddToTrackingCacheEntry(pwszUrl);
    
    return (_bTrackIt) ? S_OK : E_FAIL;        // #42604: global logging, report if this item needs logged
}

HRESULT
CUrlTrackingCache :: RunChildElement
(
    IXMLElement* pElement
)
{
    IXMLElementCollection *pCollection;
    long        lIndex = 0;
    long        lMax;
    VARIANT     vIndex, vEmpty;
    IDispatch   *pDisp;
    IXMLElement *pItem;
    BSTR        bstrTagName;
    HRESULT     hr = E_FAIL;

    if (SUCCEEDED(pElement->get_children(&pCollection)) && pCollection)
    {
        if (SUCCEEDED(pCollection->get_length(&lMax)))
        {
            vEmpty.vt = VT_EMPTY;

            for (; lIndex < lMax; lIndex++)
            {
                vIndex.vt = VT_UI4;
                vIndex.lVal = lIndex;

                if (SUCCEEDED(pCollection->item(vIndex, vEmpty, &pDisp)))
                {
                    if (SUCCEEDED(pDisp->QueryInterface(IID_IXMLElement, (void **)&pItem)))
                    {
                        if (SUCCEEDED(pItem->get_tagName(&bstrTagName)) && bstrTagName)
                        {
                            hr = ProcessItemInEnum(bstrTagName, pItem);
                            SysFreeString(bstrTagName);
                        }
                        pItem->Release();
                    }
                    pDisp->Release();
                }
            }

        }
        pCollection->Release();
    }

    return hr;
}

HRESULT
CUrlTrackingCache :: ProcessItemInEnum
(
    LPCWSTR pwszTagName, 
    IXMLElement *pItem
)
{
    HRESULT hr;
    LPWSTR  pwszName;

    if (!StrCmpIW(pwszTagName, L"HTTP-EQUIV"))
    {
        DBG("CUrlTrackingCache processing HTTP-EQUIV");
        
        hr = ReadAttribute(pItem, L"NAME", &pwszName);
        if (SUCCEEDED(hr) && !StrCmpIW(pwszName, L"ENCODING-TYPE"))
        {
            hr = ReadAttribute(pItem, L"VALUE", &_pwszEncodingMethod);
            if (SUCCEEDED(hr) && *_pwszEncodingMethod)
                WriteOLESTR(_pCDFStartItem, c_szPostHeader, _pwszEncodingMethod);
        }        
        
        SAFELOCALFREE(pwszName);
    }
    else if (!StrCmpIW(pwszTagName, L"PURGETIME"))
    {
        DBG("CUrlTrackingCache processing PurgeTime");

        return ReadAttribute(pItem, L"HOUR", &_pwszPurgeTime);
    }
    else if (!StrCmpIW(pwszTagName, L"LOG"))
    {
        DBG("CUrlTrackingCache processing Log");
        
        hr = ReadAttribute(pItem, L"VALUE", &pwszName);
        if (SUCCEEDED(hr))
            _bTrackIt = (!StrCmpIW(pwszName, L"document:view")) ? TRUE : FALSE;

        SAFELOCALFREE(pwszName);
    }

    return S_OK;
}

HRESULT
CUrlTrackingCache :: ReadAttribute
(
    IN  IXMLElement* pItem,
    IN  LPCWSTR      pwszAttributeName,
    OUT LPWSTR*      pwszAttributeValue
)
{
    VARIANT vProp;
    BSTR    bstrName = NULL;
    HRESULT hr = E_FAIL;
    DWORD   dwLen;

    vProp.vt = VT_EMPTY;
    
    bstrName = SysAllocString(pwszAttributeName);

    if (bstrName && SUCCEEDED(pItem->getAttribute(bstrName, &vProp)))
    {
        if (vProp.vt == VT_BSTR)
        {
            dwLen = sizeof(WCHAR) * (lstrlenW(vProp.bstrVal) + 1);
            *pwszAttributeValue = (LPWSTR)MemAlloc(LPTR, dwLen);
            if (*pwszAttributeValue)
            {
                StrCpyW(*pwszAttributeValue, vProp.bstrVal);
                hr = S_OK;
            }
            /*
            if (!StrCmpIW(pwszAttributeName, L"HREF") && !_pwszPostUrl)
            {
#error Change these MemAllocs to CoTaskMemAllocs            
                _pwszPostUrl = (LPWSTR)MemAlloc(LPTR, dwLen); // BUGBUG CoTaskMemAlloc
                StrCpyW(_pwszPostUrl, vProp.bstrVal);

                // fill it in item for post agent
                WriteOLESTR(_pCDFStartNot, c_szTrackingPostURL, _pwszPostUrl);
            }
            else if (!StrCmpIW(pwszAttributeName, L"VALUE") && !_pwszEncodingMethod)
            {
                _pwszEncodingMethod = (LPWSTR)MemAlloc(LPTR, dwLen);    // BUGBUG CoTaskMemAlloc
                StrCpyW(_pwszEncodingMethod, vProp.bstrVal);

                // fill it in notification for post agent
                WriteOLESTR(_pCDFStartNot, c_szPostHeader, _pwszEncodingMethod);
            }
            else if (!StrCmpIW(pwszAttributeName, L"SCOPE") )
            {
                if (!StrCmpIW(vProp.bstrVal, L"OFFLINE"))
                    _ConnectionScope = TRACK_OFFLINE_CACHE_ENTRY;
                else if (!StrCmpIW(vProp.bstrVal, L"ONLINE"))
                    _ConnectionScope = TRACK_ONLINE_CACHE_ENTRY;
                else
                    _ConnectionScope = TRACK_OFFLINE_CACHE_ENTRY | TRACK_ONLINE_CACHE_ENTRY;
            }
            else if (!StrCmpIW(pwszAttributeName, L"HOUR") && !_pwszPurgeTime)
            {
                _pwszPurgeTime = (LPWSTR)MemAlloc(LPTR, dwLen); // BUGBUG CoTaskMemAlloc
                StrCpyW(_pwszPurgeTime, vProp.bstrVal);
            }
            */

            VariantClear(&vProp);
        }
    }

    SysFreeString(bstrName);

    return hr;
}

/*
HRESULT
CUrlTrackingCache :: DoLogEventAttribute
(
    IXMLElement* pItem,
    LPCWSTR      pwszAttributeName
)
{
    VARIANT vProp;
    BSTR    bstrName = NULL;
    HRESULT hr = S_OK;

    vProp.vt = VT_EMPTY;
    
    bstrName = SysAllocString(pwszAttributeName);

    if (bstrName && SUCCEEDED(pItem->getAttribute(bstrName, &vProp)))
    {
        if (vProp.vt == VT_BSTR)
        {
            _bTrackIt = (!StrCmpIW(L"document:view", vProp.bstrVal)) ? TRUE : FALSE;
        }
        VariantClear(&vProp);
    }

    SysFreeString(bstrName);

    return hr;
}
*/

//--------------------------------------------------------------------------
//
// CUrlTrackingCache
//
//--------------------------------------------------------------------------
CUrlTrackingCache::CUrlTrackingCache
(
    ISubscriptionItem *pCDFItem,
    LPCWSTR pwszURL
)
{
    _lpPfx = NULL;
    _dwPurgeTime = 24;
    _pCDFStartItem = pCDFItem;

    ASSERT(_pCDFStartItem);
    _pCDFStartItem->AddRef();

    Init(pwszURL);
        
}

CUrlTrackingCache::~CUrlTrackingCache()
{

    SAFEFREEOLESTR(_pwszPostUrl);
    SAFEFREEOLESTR(_pwszEncodingMethod);
    SAFEFREEOLESTR(_pwszPurgeTime);

    SAFELOCALFREE(_pszChannelUrlSite);
    SAFELOCALFREE(_pszPostUrlSite);
    SAFELOCALFREE(_lpPfx);
         
    SAFERELEASE(_pCDFStartItem);
}

