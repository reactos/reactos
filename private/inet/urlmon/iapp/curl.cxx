//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       curl.cxx
//
//  Contents:   handle url parsing and context urls parsing
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <iapp.h>
#include <shlwapi.h>
#include <shlwapip.h>

PerfDbgTag(tagCUrl,    "Urlmon", "Log CUrl",     DEB_PROT);
PerfDbgTag(tagCUrlApi, "Urlmon", "Log CUrl API", DEB_ASYNCAPIS);

//+---------------------------------------------------------------------------
//
//  Function: IsStreamEnabled
//
//  Synopsis: returns TRUE iff the registry key for the mk: protocol is set
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:   6-7-96   craigc          Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL IsStreamEnabled()
{
    HKEY    hk;
    char    szBuf[256];
    DWORD   dwType;
    DWORD   dwSize;
    const char    szKey[] = "SOFTWARE\\Microsoft\\Internet Explorer";
    const char    szValue[] = "MkEnabled";
    const char    szYes[] = "yes";

    static BOOL   fChecked = FALSE;
    static BOOL   fEnabled = FALSE;

    CMutexSem mxs;
    CLock lck(mxs);

    if (fChecked)
       return fEnabled;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szKey, &hk) != ERROR_SUCCESS)
        return( FALSE );

    dwSize = sizeof(szBuf);
    if (RegQueryValueEx( hk, szValue, NULL, &dwType, (BYTE*)szBuf, &dwSize ) != ERROR_SUCCESS)
    {
        RegCloseKey( hk );
        return( FALSE );
    }
    RegCloseKey( hk );

    fEnabled = (dwSize && (lstrcmpi( szYes, szBuf ) == 0));
    fChecked = TRUE;

    return fEnabled;
}

//
//  GetUrlScheme() returns one of the URL_SCHEME_* constants as
//  defined in shlwapip.h
//  example "http://foo" returns URL_SCHEME_HTTP
//
DWORD GetUrlScheme(IN LPCTSTR pcszUrl)
{
    if(pcszUrl)
    {
        PARSEDURL pu;
        pu.cbSize = sizeof(pu);
        if(SUCCEEDED(ParseURL(pcszUrl, &pu)))
            return pu.nScheme;
    }
    return URL_SCHEME_INVALID;
}


//+---------------------------------------------------------------------------
//
//  Method:     CUrl::CUrl
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CUrl::CUrl()
{
    PerfDbgLog(tagCUrl, this, "+CUrl::CUrl");

    _pszBaseURL      = NULL;
    _pszPartURL      = NULL;
    _pszFullURL      = NULL;
    _pszProtocol     = NULL;
    _pszServerName   = NULL;
    _pszUserName     = NULL;
    _pszPassword     = NULL;
    _pszObject       = NULL;
    _pBasicAllocUnit = NULL;

    _ipPort = 0;
    _dwProto = 0;

    PerfDbgLog(tagCUrl, this, "-CUrl::CUrl");
}

BOOL
CUrl::CUrlInitBasic(DWORD dwBaseUrlSize)
{
    //
    // basic allocation contains the following fields:  
    //
    //  _pszServerName - URL_FIELD_SIZE
    //  _pszUserName   - URL_FIELD_SIZE
    //  _pszPassword   - URL_FIELD_SIZE
    //  _pszProtocol   - 12
    //  _pszBaseURL    - dwBaseUrlSize + 1
    //
    BOOL fRet = FALSE;
    DWORD dwBasicUnitSize = (3 * (URL_FIELD_SIZE)) + 12 + dwBaseUrlSize + 1;
    _pBasicAllocUnit = new char[dwBasicUnitSize];

    if( _pBasicAllocUnit )
    {
        _pszServerName = _pBasicAllocUnit;
        _pszUserName   = _pszServerName + URL_FIELD_SIZE;
        _pszPassword   = _pszUserName   + URL_FIELD_SIZE;
        _pszProtocol   = _pszPassword   + URL_FIELD_SIZE;
        _pszBaseURL    = _pszProtocol   + 12;

        *_pszServerName = '\0';
        *_pszUserName   = '\0';
        *_pszPassword   = '\0';
        *_pszProtocol   = '\0';
        *_pszBaseURL    = '\0';

        fRet = TRUE;
    }
          
    return fRet;
}

BOOL
CUrl::CUrlInitAll()
{
    // backword compatibility init all with URL_MAX_LENGTH
    BOOL fRet = FALSE;
    fRet = CUrlInitBasic(MAX_URL_SIZE);

    if( fRet )
    { 
        _pszPartURL = new char[MAX_URL_SIZE + 1];
        _pszFullURL = new char[MAX_URL_SIZE + 1];
        _pszObject  = new char[MAX_URL_SIZE + 1];
    
        if( !_pszPartURL || !_pszFullURL || !_pszObject )
        {
            fRet = FALSE; 
        }

        *_pszPartURL = '\0';
        *_pszFullURL = '\0';
        *_pszObject  = '\0';
    }

    return fRet;
}

CUrl::~CUrl()
{
    if( _pBasicAllocUnit )
    {
        delete [] _pBasicAllocUnit;
    }
    if( _pszPartURL )
    {
        delete [] _pszPartURL;
    }
    if( _pszFullURL )
    {
        delete [] _pszFullURL;
    }
    if( _pszObject )
    {
        delete [] _pszObject;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ParseUrl
//
//  Synopsis:   Breaks down a URL and puts servername, objectname and port
//              into the download structure.
//
//  Arguments:
//
//
//  Returns:    TRUE if the URL was successfully parsed.
//
//  History:    Created    Unknown
//              02-20-95   JohannP (Johann Posch)   Created Class
//              03-20-95   JoeS    (Joe Souza)      Special FILE: syntaxes
//
//  Notes:      URL should have already been parsed earlier by ConstructURL.
//              This function will crack the URL.
//
//----------------------------------------------------------------------------
BOOL CUrl::ParseUrl()
{
    PerfDbgLog1(tagCUrl, this, "+CUrl::ParseUrl Base:[%s]", _pszBaseURL);
    BOOL            fRet = TRUE;
    URL_COMPONENTS  url;
    DWORD           cchFullURL;
    LPSTR           szTemp;
    CHAR *          pch;
    DWORD           dwFullUrlLen;

    if (_pszPartURL && _pszPartURL[0] != '\0' )  // This string will be set for redirects.
    {
        //
        // we need re-alloc _pszFullURL and _pszObject, since
        // the the size can grow!
        //
        dwFullUrlLen = strlen(_pszBaseURL) + strlen(_pszPartURL) + 1;
        if(dwFullUrlLen > MAX_URL_SIZE)
        {
            dwFullUrlLen = MAX_URL_SIZE + 1;
        } 

        if( _pszFullURL )
        {
            delete [] _pszFullURL;
            _pszFullURL = NULL;
            _pszFullURL = new char[dwFullUrlLen];
        }

        if( _pszObject )
        {
            delete [] _pszObject;
            _pszObject = NULL;
            _pszObject = new char[dwFullUrlLen];
        }

        if( !_pszFullURL || !_pszObject  )
        {
            fRet = FALSE;
            goto Exit;
        }

        cchFullURL = dwFullUrlLen; 

        if(FAILED(UrlCombine(_pszBaseURL, _pszPartURL, _pszFullURL, &cchFullURL, URL_FILE_USE_PATHURL)))
        {
            fRet = FALSE;
            PProtAssert(FALSE && "Combine failed in ParseUrl!\n");

            goto Exit;
        }
    }
    else
    {
        // FullURL is BaseURL
        dwFullUrlLen = strlen(_pszBaseURL) + 1;
        if( !_pszFullURL )
        {
            _pszFullURL = new char[dwFullUrlLen];
        }

        if( !_pszObject )
        {
            _pszObject = new char[dwFullUrlLen];
        }

        if( !_pszFullURL || !_pszObject  )
        {
            fRet = FALSE;
            goto Exit;
        }

        lstrcpy(_pszFullURL, _pszBaseURL);
    }

    // Trim off intra-page link.
    //
    // NB: Don't use ExtraInfo below to do this because you will
    // also lose search string this way.
    //
    // Also, we need to do this before we decode the URL below,
    // so that we don't trim off the wrong '#' if there was one
    // encoded in the URL path.
    //
    //  UrlGetLocation() will intelligently find the fragment
    //  some schemes do not use the # as a fragment identifier.
    //  it returns a pointer to the #
    //

    if(pch = (CHAR *)UrlGetLocation(_pszFullURL))
    {
        *pch = TEXT('\0');
    }


    _dwProto = ProtoFromString(_pszFullURL);

    if (_dwProto == DLD_PROTOCOL_NONE)
    {
        fRet = FALSE;
        goto Exit;
    }

    
    if(DLD_PROTOCOL_FILE == _dwProto)
    {
        //
        // at this point, _pszFullURL and _pszObject should be all
        // allocated with size of dwFullUrlLen
        //
        DWORD cchObject = dwFullUrlLen;

        //do file stuff here
        fRet = SUCCEEDED(PathCreateFromUrl(_pszFullURL, _pszObject, &cchObject, 0));

    }
    else
    {
        //
        //  BUGBUG - InternetCrackUrl alters the original url - zekel - 25-JUL-97
        //  ICU is poorly behaved, and it unescapes the server and username
        //  components insitu regardless of whether it was requested or not
        //  this means that if you pass in http://host%76/, the url on return
        //  is http://hostv76/.  it happens that if you create the URL from the 
        //  components given, you will get the correct URL, but crack doesnt understand
        //  all URLs.  it is too late in the game to change the behavior of ICU,
        //  because wininet internally depends on the behavior.
        //  so our solution is to create a temp buffer that can be messed with
        //  and then throw it away after we are done.
        //
        LPSTR pszTemp = StrDup(_pszFullURL);

        if (pszTemp)
        {
            url.dwStructSize = sizeof(url);

            url.lpszScheme = _pszProtocol;
            url.dwSchemeLength =12;

            url.lpszHostName = _pszServerName;
            url.dwHostNameLength = URL_FIELD_SIZE;

            url.lpszUserName = _pszUserName;
            url.dwUserNameLength = URL_FIELD_SIZE;

            url.lpszPassword = _pszPassword;
            url.dwPasswordLength = URL_FIELD_SIZE;

            url.lpszUrlPath = _pszObject;
            url.dwUrlPathLength = dwFullUrlLen;

            url.lpszExtraInfo = NULL;
            url.dwExtraInfoLength = 0;

            fRet = InternetCrackUrl(pszTemp, 0, (_dwProto == DLD_PROTOCOL_STREAM ? ICU_DECODE : 0), &url);

            _ipPort = url.nPort;

            LocalFree(pszTemp);
        }
        else
            fRet = FALSE;
    }


Exit:

    PerfDbgLog1(tagCUrl, this, "-CUrl::ParseUrl Full:[%s]", _pszFullURL);
    return(fRet);
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrl::ProtoFromString
//
//  Synopsis:
//
//  Arguments:  [lpszProtocol] --
//
//  Returns:
//
//  History:    Created   Unknown
//              2-20-96   JohannP (Johann Posch)   Modified for class
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CUrl::ProtoFromString(LPSTR lpszProtocol)
{
    PerfDbgLog1(tagCUrl, this, "CUrl::ProtoFromString [%s]", lpszProtocol);

    switch (GetUrlScheme(lpszProtocol))
    {
    case URL_SCHEME_HTTPS:
        return (DLD_PROTOCOL_HTTPS);
    
    case URL_SCHEME_HTTP:
        return (DLD_PROTOCOL_HTTP);

    case URL_SCHEME_FTP:
        return (DLD_PROTOCOL_FTP);

    case URL_SCHEME_GOPHER:
        return (DLD_PROTOCOL_GOPHER);
    
    case URL_SCHEME_FILE:
        return (DLD_PROTOCOL_FILE);

    case URL_SCHEME_LOCAL:
        return (DLD_PROTOCOL_LOCAL);

    case URL_SCHEME_MK:
        if(IsStreamEnabled())
            return (DLD_PROTOCOL_STREAM);
        break;

    }
    return (DLD_PROTOCOL_NONE);
}


// Helper API's

//+---------------------------------------------------------------------------
//
//  Function:   ConstructURL
//
//  Synopsis:
//
//  Arguments:  [pBC] -- Pointer to BindCtx
//              [pURLBase] -- Pointer to Base URL [IN]
//              [pURLRelative] -- Pointer to Relative URL [IN]
//              [pURLFull] -- Pointer to resultant complete URL [OUT]
//
//  Returns:
//
//  History:    02-21-96    JoeS    (Joe Souza)     Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ConstructURL(LPBC pBC, LPMONIKER pmkContext, LPMONIKER pmkToLeft,
            LPWSTR pwzURLRelative, LPWSTR pwzURLFull, DWORD cURLSize,
            DWORD dwFlags)
{
    PerfDbgLog2(tagCUrlApi, NULL, "+ConstructURL (rel:%ws, pmk:%lx)", pwzURLRelative, pmkContext);
    HRESULT     hr = NOERROR;
    DWORD       dwMnk = 0;
    LPMONIKER   pmkCtx = NULL;
    LPWSTR      wzURLBase = NULL;
    WCHAR       wszURLFull[MAX_URL_SIZE + 1];
    DWORD       cchURLFull;
    DWORD       cbSize;
    BOOL        bParseOk = FALSE;

    if (!pwzURLRelative || !pwzURLFull || !cURLSize)
    {
        hr = E_INVALIDARG;
        goto ConstructExit;
    }

    pwzURLFull[0] = 0;

    if (!pmkContext && pBC)
    {
        // No Context Moniker was specified, so try to get one of those.

        hr = pBC->GetObjectParam(SZ_URLCONTEXT, (IUnknown **)&pmkCtx);
        if (hr != NOERROR)
        {
            pmkCtx = NULL;
        }
    }
    else if (pmkContext)
    {
        pmkCtx = pmkContext;
    }
    else if (pmkToLeft)
    {
        pmkCtx = pmkToLeft;
    }

    if (pmkCtx)
    {
        // There is a Context Moniker.  Make sure it is a URL moniker and
        // if it is, get the base URL from it.

        pmkCtx->IsSystemMoniker(&dwMnk);
        if (dwMnk == MKSYS_URLMONIKER)
        {
            hr = pmkCtx->GetDisplayName(pBC, NULL, &wzURLBase);
        }
    }

    hr = NOERROR;

    if (wzURLBase)
    {
        DWORD dwSizeIn = MAX_URL_SIZE;
        cchURLFull = MAX_URL_SIZE;
        bParseOk = SUCCEEDED(OInetCombineUrl(wzURLBase, pwzURLRelative, URL_FILE_USE_PATHURL, pwzURLFull, dwSizeIn, &cchURLFull, 0));

    }
    else if (dwFlags & CU_CANONICALIZE)
    {
        DWORD dwSizeIn = MAX_URL_SIZE;
        cchURLFull = MAX_URL_SIZE;
        bParseOk = SUCCEEDED(OInetParseUrl(pwzURLRelative,PARSE_CANONICALIZE, URL_FILE_USE_PATHURL, pwzURLFull, dwSizeIn,&cchURLFull,0));
    }
    else
    {
        // We did not combine a relative and a base URL, and caller
        // does not want to canonicalize, so we just copy the given URL
        // into the return buffer.
#ifndef unix
        wcsncpy(pwzURLFull, pwzURLRelative, cURLSize / 2);
#else
        wcsncpy(pwzURLFull, pwzURLRelative, cURLSize / sizeof(wchar_t));
#endif /* unix */
        goto ConstructExit;
    }


    if (!bParseOk || !wcslen(pwzURLFull))
    {
        hr = MK_E_SYNTAX;
    }

ConstructExit:

    if (wzURLBase)
    {
        delete wzURLBase;
    }

    PerfDbgLog2(tagCUrlApi, NULL, "-ConstructURL [%ws], hr:%lx", pwzURLFull, hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
// UTF-8 code from wininet written by RFirth
//
//----------------------------------------------------------------------------
DWORD
CountUnicodeToUtf8(
    IN LPCWSTR pwszIn,
    IN DWORD dwInLen,
    IN BOOL bEncode
    )

/*++

Routine Description:

    Count number of BYTEs required for UTF-8 conversion of UNICODE string. Count
    is terminated after dwInLen characters

Arguments:

    pwszIn  - pointer to input wide-character string

    dwInLen - number of characters in pwszIn

    bEncode - TRUE if we are to hex encode characters >= 0x80

Return Value:

    DWORD   - number of BYTEs required for conversion

--*/

{
    PProtAssert(pwszIn != NULL);
    PProtAssert(dwInLen != 0);

    DWORD dwCount = 0;
    DWORD oneCharLen = bEncode ? 3 : 1;
    DWORD twoCharLen = 2 * oneCharLen;

    //
    // N.B. code arranged to reduce number of jumps in loop to 1 (while)
    //

    do {

        WORD wchar = *pwszIn++;

        dwCount += (wchar & 0xF800) ? oneCharLen : 0;
        dwCount += ((wchar & 0xFF80) ? 0xFFFFFFFF : 0) & (twoCharLen - 1);
        ++dwCount;
    } while (--dwInLen != 0);
    return dwCount;
}

DWORD
ConvertUnicodeToUtf8(
    IN LPCWSTR pwszIn,
    IN DWORD dwInLen,
    OUT LPBYTE pszOut,
    IN DWORD dwOutLen,
    IN BOOL bEncode
    )

/*++

Routine Description:

    Convert a string of UNICODE characters to UTF-8:

        0000000000000000..0000000001111111: 0xxxxxxx
        0000000010000000..0000011111111111: 110xxxxx 10xxxxxx
        0000100000000000..1111111111111111: 1110xxxx 10xxxxxx 10xxxxxx

Arguments:

    pwszIn      - pointer to input wide-character string

    dwInLen     - number of CHARACTERS in pwszIn INCLUDING terminating NUL

    pszOut      - pointer to output narrow-character buffer

    dwOutLen    - number of BYTEs in pszOut

    bEncode     - TRUE if we are to hex encode characters >= 0x80

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    Not enough space in pszOut to store results

--*/

{
    PProtAssert(pwszIn != NULL);
    PProtAssert((int)dwInLen > 0);
    PProtAssert(pszOut != NULL);
    PProtAssert((int)dwOutLen > 0);

    DWORD outputSize = bEncode ? 3 : 1;
    static char hexArray[] = "0123456789ABCDEF";

    while (dwInLen-- && dwOutLen) {

        WORD wchar = *pwszIn++;
        BYTE bchar;

        if (wchar <= 0x007F) {
            *pszOut++ = (BYTE)(wchar);
            --dwOutLen;
            continue;
        }

        BYTE lead = ((wchar >= 0x0800) ? 0xE0 : 0xC0);
        int shift = ((wchar >= 0x0800) ? 12 : 6);

        if ((int)(dwOutLen - outputSize) < 0) {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        bchar = lead | (BYTE)(wchar >> shift);
        if (bEncode) {
            *pszOut++ = '%';
            *pszOut++ = hexArray[bchar >> 4];
            bchar = hexArray[bchar & 0x0F];
        }
        *pszOut++ = bchar;

        if (wchar >= 0x0800) {
            if ((int)(dwOutLen - outputSize) < 0) {
                return ERROR_INSUFFICIENT_BUFFER;
            }
            bchar = 0x80 | (BYTE)((wchar >> 6) & 0x003F);
            if (bEncode) {
                *pszOut++ = '%';
                *pszOut++ = hexArray[bchar >> 4];
                bchar = hexArray[bchar & 0x0F];
            }
            *pszOut++ = bchar;
        }
        if ((int)(dwOutLen - outputSize) < 0) {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        bchar = 0x80 | (BYTE)(wchar & 0x003F);
        if (bEncode) {
            *pszOut++ = '%';
            *pszOut++ = hexArray[bchar >> 4];
            bchar = hexArray[bchar & 0x0F];
        }
        *pszOut++ = bchar;
    }
    return ERROR_SUCCESS;
}

BOOL
StringContainsHighAnsi(
    IN LPSTR pszIn,
    IN DWORD dwInLen
    )

/*++

Routine Description:

    Determine if string contains ANSI characters in range 0x80..0xFF. Search is
    stopped when we hit the first high-ANSI character, when we hit the terminator
    or when we have decremented dwInLen to zero

Arguments:

    pszIn   - pointer to string to test

    dwInLen - length of pszIn

Return Value:

    BOOL
        TRUE    - pszIn contains one or more high-ANSI characters

        FALSE   - pszIn (or substring of length dwInLen) does not contain
                  high-ANSI characters

--*/

{
    PProtAssert(pszIn != NULL);
    PProtAssert(dwInLen != 0);

    // only need to search the base portion 
    while (dwInLen-- && *pszIn && *pszIn != '?') {
        if (*pszIn++ & 0x80) {
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
StringContainsHighAnsiW(
    IN LPCWSTR pwzIn
    )
/*--
    Unicode version of StringContainsHighAnsi()
--*/
{
    PProtAssert(pwzIn != NULL);

    // only need to search the base portion 
    while (*pwzIn && *pwzIn != L'?') 
    {
        if (*pwzIn >= 0x80) 
        {
            return TRUE;
        }
        pwzIn++;
    }
    return FALSE;
}


BOOL
ConvertUnicodeUrl(
    LPCWSTR pwzFrom, 
    LPSTR   pszTo,
    INT     cchTo,
    DWORD   dwCodePage,
    BOOL    fUTF8Enabled
    
    )
{
    BOOL fSuccess = FALSE;

    //
    // In multibyte string, if we have any bytes(in the base url portion)
    // over 0x80, we will have to convert the base portion to utf-8
    // (leave the query portion as multi-byte)
    //
    // S_FALSE from the conversion above indicates that some wide chars couldn't be
    // mapped to the destination code page

    if( fUTF8Enabled && StringContainsHighAnsiW(pwzFrom))
    {
        // utf-8 conversion

        //
        // do we have a query portion? (by searching the UNICODE URL string
        // for '?') also we can get the UNICODED string's BasePortion 
        // URL Length 
        // 
        DWORD  dwBaseUrlLen = 0;        // BasePortion length (in UNICODE)
        LPWSTR pBase = (LPWSTR) pwzFrom;
        while( *pBase && *pBase != '?')
        {
            pBase++;
            dwBaseUrlLen++;
        }

        DWORD  dwMBQryUrlLen  = 0;      // QueryPortion length (in Multibyte)
        LPSTR  pszQry = NULL;           // multibyte query string 
        if (*pBase)
        {
            //
            // we have a query portion, need to get length of
            // multi-byte query portion
            // In this case, we don't care whether or not pszTo is able to convert 
            // everything
            W2A(pBase, pszTo, cchTo, dwCodePage);
            dwMBQryUrlLen = strlen(pszTo);
            pszQry = pszTo;
        }
        
        //
        // we are converting the base portion of UNICODE URL to UTF-8 
        // count UTF-8 string length for base url
        //
        DWORD dwUTF8Len = CountUnicodeToUtf8(pwzFrom, dwBaseUrlLen, TRUE);

        //
        // allocate buffer for whole converted string 
        // Buffer size = UTF8_BaseURL_Len + MultiByte_Query_Len + '\0'
        //
        DWORD dwUTFBufferSize = dwUTF8Len + dwMBQryUrlLen + 1;

        // 
        // the size can not exceed incoming buffer size ccTo
        //
        if( dwUTFBufferSize > (DWORD)(cchTo + 1) )
        {
            //
            // fallback to IE4 behavior - sending multi-byte string
            //
            goto cleanup;
        }

        char* pszUTF8 = new char[dwUTFBufferSize];
        if( !pszUTF8 )
        {
            //
            // if we failed to allocate, we automatically
            // fallback to IE4 behavior - sending multi-byte string
            //
            goto cleanup;
        }
        memset(pszUTF8, 0, dwUTFBufferSize); 

        // Coverting UNICODE->UTF8
        DWORD dwError;
        dwError = ConvertUnicodeToUtf8( pwzFrom, 
                                        dwBaseUrlLen, 
                                        (LPBYTE)pszUTF8, 
                                        dwUTF8Len,
                                        TRUE );
        if( dwError != ERROR_SUCCESS )
        {
            //
            // if we failed, delete temp string and fallback to IE4
            // behavior - sending multi-byte string
            //
            delete [] pszUTF8;
            goto cleanup;
        } 

        //
        // copy over the Multi-byte query string to final buffer
        //
        if( pszQry )
        {
            LPSTR pszURL = pszUTF8 + dwUTF8Len;       
            while( dwMBQryUrlLen-- )
            {
                *pszURL = *pszQry;
                pszURL++;
                pszQry++;    
            }
        }

        //
        // we are done, copy the content from temp buffer to 
        // szTo 
        //
        StrCpyN(pszTo, pszUTF8, dwUTFBufferSize);

        // delete temp utf8 buffer 
        delete [] pszUTF8;
        fSuccess = TRUE;
    }
cleanup:
    if (!fSuccess)
    {
        // Unicode->Multibyte 
        // IE4 behaviour. Shoot.
        W2A(pwzFrom, pszTo, cchTo, dwCodePage);
    }

    return TRUE;
}
