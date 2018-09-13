//=================================================
//
//  File : Utils.cxx
//
//  purpose : implementation of helpful stuff
//
//=================================================


#include "headers.h"
#include "utils.hxx"

#define BUFFEREDSTR_SIZE 1024

// VARIANT conversion interface exposed by script engines (VBScript/JScript).
EXTERN_C const GUID SID_VariantConversion = 
                { 0x1f101481, 0xbccd, 0x11d0, { 0x93, 0x36,  0x0,  0xa0,  0xc9,  0xd,  0xca,  0xa9 } };

//+------------------------------------------------------------------------
//
//  Function:   GetHTMLDocument
//
//  Synopsis:   Gets the IHTMLDocument2 interface from the client site.
//
//+------------------------------------------------------------------------

STDMETHODIMP 
GetHTMLDocument(IElementBehaviorSite * pSite, IHTMLDocument2 **ppDoc)
{
    HRESULT hr = E_FAIL;

    if (!ppDoc)
        return E_POINTER;

    if (pSite != NULL)
    {
        IHTMLElement *pElement = NULL;
        hr = pSite->GetElement(&pElement);
        if (SUCCEEDED(hr))
        {
            IDispatch * pDispDoc = NULL;
            hr = pElement->get_document(&pDispDoc);
            if (SUCCEEDED(hr))
            {
                hr = pDispDoc->QueryInterface(IID_IHTMLDocument2, (void **)ppDoc);
                pDispDoc->Release();
            }
            pElement->Release();
        }
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   GetHTMLWindow
//
//  Synopsis:   Gets the IHTMLWindow2 interface from the client site.
//
//+------------------------------------------------------------------------

STDMETHODIMP 
GetHTMLWindow(IElementBehaviorSite * pSite, IHTMLWindow2 **ppWindow)
{
    HRESULT hr = E_FAIL;
    IHTMLDocument2 *pDoc = NULL;

    hr = GetHTMLDocument(pSite, &pDoc);

    if (SUCCEEDED(hr))
    {
        hr = pDoc->get_parentWindow(ppWindow);
        pDoc->Release();
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   GetClientSiteWindow
//
//  Synopsis:   Gets the window handle of the client site passed in.
//
//+------------------------------------------------------------------------

STDMETHODIMP 
GetClientSiteWindow(IElementBehaviorSite *pSite, HWND *phWnd)
{
    HRESULT hr = E_FAIL;
    IWindowForBindingUI *pWindowForBindingUI = NULL;

    if (pSite != NULL) {

        // Get IWindowForBindingUI ptr
        hr = pSite->QueryInterface(IID_IWindowForBindingUI,
                (LPVOID *)&pWindowForBindingUI);

        if (FAILED(hr)) {
            IServiceProvider *pServProv;
            hr = pSite->QueryInterface(IID_IServiceProvider, (LPVOID *)&pServProv);

            if (hr == NOERROR) {
                pServProv->QueryService(IID_IWindowForBindingUI,IID_IWindowForBindingUI,
                    (LPVOID *)&pWindowForBindingUI);
                pServProv->Release();
            }
        }

        if (pWindowForBindingUI) {
            pWindowForBindingUI->GetWindow(IID_IWindowForBindingUI, phWnd);
            pWindowForBindingUI->Release();
        }
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:   ClearInterfaceFn
//
//  Synopsis:   Sets an interface pointer to NULL, after first calling
//              Release if the pointer was not NULL initially
//
//  Arguments:  [ppUnk]     *ppUnk is cleared
//
//-------------------------------------------------------------------------

void
ClearInterfaceFn(IUnknown ** ppUnk)
{
    IUnknown * pUnk;

    pUnk = *ppUnk;
    *ppUnk = NULL;
    if (pUnk)
        pUnk->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ReplaceInterfaceFn
//
//  Synopsis:   Replaces an interface pointer with a new interface,
//              following proper ref counting rules:
//
//              = *ppUnk is set to pUnk
//              = if *ppUnk was not NULL initially, it is Release'd
//              = if pUnk is not NULL, it is AddRef'd
//
//              Effectively, this allows pointer assignment for ref-counted
//              pointers.
//
//  Arguments:  [ppUnk]
//              [pUnk]
//
//-------------------------------------------------------------------------

void
ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk)
{
    IUnknown * pUnkOld = *ppUnk;

    *ppUnk = pUnk;

    //  Note that we do AddRef before Release; this avoids
    //    accidentally destroying an object if this function
    //    is passed two aliases to it

    if (pUnk)
        pUnk->AddRef();

    if (pUnkOld)
        pUnkOld->Release();
}



//+------------------------------------------------------------------------
//
//  Function:   ReleaseInterface
//
//  Synopsis:   Releases an interface pointer if it is non-NULL
//
//  Arguments:  [pUnk]
//
//-------------------------------------------------------------------------

void
ReleaseInterface(IUnknown * pUnk)
{
    if (pUnk)
        pUnk->Release();
}


//+------------------------------------------------------------------------
//
//  Member:     CBufferedStr::Set
//
//  Synopsis:   Initilizes a CBufferedStr
//
//-------------------------------------------------------------------------
HRESULT
CBufferedStr::Set (LPCTSTR pch, UINT uiCch)
{
    HRESULT hr = S_OK;

    Free();

    if (!uiCch)
        _cchIndex = pch ? _tcslen (pch) : 0;
    else
        _cchIndex = uiCch;

    _cchBufSize = _cchIndex > BUFFEREDSTR_SIZE ? _cchIndex : BUFFEREDSTR_SIZE;
    _pchBuf = new TCHAR [ _cchBufSize ];
    if (!_pchBuf)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (pch)
    {
        _tcsncpy (_pchBuf, pch, _cchIndex);
    }

    _pchBuf[_cchIndex] = '\0';

Cleanup:
    return (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CBufferedStr::QuickAppend
//
//  Parameters: pchNewStr   string to be added to _pchBuf
//
//  Synopsis:   Appends pNewStr into _pchBuf starting at
//              _pchBuf[uIndex].  Increments index to reference
//              new end of string.  If _pchBuf is not large enough,
//              reallocs _pchBuf and updates _cchBufSize.
//
//-------------------------------------------------------------------------
HRESULT
CBufferedStr::QuickAppend (const TCHAR* pchNewStr, ULONG newLen)
{
    HRESULT hr = S_OK;

    if (!_pchBuf)
    {
        hr = Set();
        if (hr)
            goto Cleanup;
    }

    if (_cchIndex + newLen >= _cchBufSize)    // we can't fit the new string in the current buffer
    {                                         // so allocate a new buffer, and copy the old string
        _cchBufSize += (newLen > BUFFEREDSTR_SIZE) ? newLen : BUFFEREDSTR_SIZE;
        TCHAR * pchTemp = new TCHAR [ _cchBufSize ];
        if (!pchTemp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _tcsncpy (pchTemp, _pchBuf, _cchIndex);

        Free();
        _pchBuf = pchTemp;
    }

    // append the new string
    _tcsncpy (_pchBuf + _cchIndex, pchNewStr, newLen);
    _cchIndex += newLen;
    _pchBuf[_cchIndex] = '\0';

Cleanup:
    return (hr);
}
HRESULT
CBufferedStr::QuickAppend(long lValue)
{
    TCHAR   strValue[40];

#ifdef UNICODE
    return QuickAppend( _ltow(lValue, strValue, 10) );
#else
    return QuickAppend( _ltoa(lValue, strValue, 10) );
#endif
}

//+---------------------------------------------------------------------------
//
//  method : ConvertGmtTimeToString
//
//  Synopsis: This function produces a standard(?) format date, of the form
// Tue, 02 Apr 1996 02:04:57 UTC  The date format *will not* be tailored
// for the locale.  This is for cookie use and Netscape compatibility
//
//----------------------------------------------------------------------------
static const TCHAR* achMonth[] = {
    _T("Jan"),_T("Feb"),_T("Mar"),_T("Apr"),_T("May"),_T("Jun"),
        _T("Jul"),_T("Aug"),_T("Sep"),_T("Oct"),_T("Nov"),_T("Dec") 
};
static const TCHAR* achDay[] = {
    _T("Sun"), _T("Mon"),_T("Tue"),_T("Wed"),_T("Thu"),_T("Fri"),_T("Sat")
};

HRESULT 
ConvertGmtTimeToString(FILETIME Time, TCHAR * pchDateStr, DWORD cchDateStr)
{
    SYSTEMTIME SystemTime;
    CBufferedStr strBuf;

    if (cchDateStr < DATE_STR_LENGTH)
        return E_INVALIDARG;

    FileTimeToSystemTime(&Time, &SystemTime);

    strBuf.QuickAppend(achDay[SystemTime.wDayOfWeek]);
    strBuf.QuickAppend(_T(", "));
    strBuf.QuickAppend(SystemTime.wDay);
    strBuf.QuickAppend(_T(" ") );
    strBuf.QuickAppend(achMonth[SystemTime.wMonth - 1] );
    strBuf.QuickAppend(_T(" ") );
    strBuf.QuickAppend(SystemTime.wYear );
    strBuf.QuickAppend(_T(" ") );
    strBuf.QuickAppend(SystemTime.wHour );
    strBuf.QuickAppend(_T(":") );
    strBuf.QuickAppend(SystemTime.wMinute );
    strBuf.QuickAppend(_T(":") );
    strBuf.QuickAppend(SystemTime.wSecond );
    strBuf.QuickAppend(_T(" UTC") );

    if (strBuf.Length() >cchDateStr)
        return E_FAIL;

    _tcscpy(pchDateStr, strBuf);

    return S_OK;
}

HRESULT 
ParseDate(BSTR strDate, FILETIME * pftTime)
{
    HRESULT      hr = S_FALSE;
    SYSTEMTIME   stTime ={0};
    LPCTSTR      pszToken = NULL;
    BOOL         fFound;
    int          idx, cch;
    CDataListEnumerator  dle(strDate, _T(':'));

    if (!pftTime)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // get the dayOfTheWeek:  3 digits max plus comma
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 4)
        goto Cleanup;
    else
    {
        for (idx=0; idx < ARRAY_SIZE(achDay); idx++)
        {
            fFound = !_tcsnicmp( pszToken, achDay[idx], 3);
            if (fFound)
            {
                stTime.wDayOfWeek = (WORD)idx;
                break;
            }
        }

        if (!fFound)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // get the Day 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wDay = (WORD)_ttoi(pszToken);

    // get the Month: 3 characters
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 3)
        goto Cleanup;
    else
    {
        for (idx=0; idx < ARRAY_SIZE(achMonth); idx++)
        {
            fFound = !_tcsnicmp( pszToken, achMonth[idx], 3);
            if (fFound)
            {
                stTime.wMonth = (WORD)idx + 1;
                break;
            }
        }

        if (!fFound)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // get the Year 4 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 4)
        goto Cleanup;

    stTime.wYear = (WORD)_ttoi(pszToken);

    // get the Hour 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wHour = (WORD)_ttoi(pszToken);

    // get the Minute 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wMinute = (WORD)_ttoi(pszToken);

    // get the Second 2 digits max
    //--------------------------------------------------
    if (! dle.GetNext(&pszToken, &cch) || cch > 2)
        goto Cleanup;

    stTime.wSecond = (WORD)_ttoi(pszToken);

    // now we have SYSTEMTIME, lets return the FILETIME
    if (!SystemTimeToFileTime(&stTime, pftTime))
        hr = GetLastError();
    else
        hr = S_OK;

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   MbcsFromUnicode
//
//  Synopsis:   Converts a string to MBCS from Unicode.
//
//  Arguments:  [pstr]  -- The buffer for the MBCS string.
//              [cch]   -- The size of the MBCS buffer, including space for
//                              NULL terminator.
//
//              [pwstr] -- The Unicode string to convert.
//              [cwch]  -- The number of characters in the Unicode string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pstr].
//
//----------------------------------------------------------------------------

int
MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    if (!pstr || cch <= 0 || !pwstr || cwch<-1)
        return 0;

    ret = WideCharToMultiByte(CP_ACP, 0, pwstr, cwch, pstr, cch, NULL, NULL);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
    }
#endif /* } */

    return ret;
}


//+---------------------------------------------------------------------------
//
//  Function:   UnicodeFromMbcs
//
//  Synopsis:   Converts a string to Unicode from MBCS.
//
//  Arguments:  [pwstr] -- The buffer for the Unicode string.
//              [cwch]  -- The size of the Unicode buffer, including space for
//                              NULL terminator.
//
//              [pstr]  -- The MBCS string to convert.
//              [cch]  -- The number of characters in the MBCS string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pwstr] is NULL or [cwch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pwstr].
//
//----------------------------------------------------------------------------

int
UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    if (!pstr || cwch <= 0 || !pwstr || cch<-1)
        return 0;

    ret = MultiByteToWideChar(CP_ACP, 0, pstr, cch, pwstr, cwch);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
    }
#endif /* } */

    return ret;
}


//+--------------------------------------------------------------------
//
//  Function:    _tcsistr
//
//---------------------------------------------------------------------

const TCHAR * __cdecl _tcsistr (const TCHAR * tcs1,const TCHAR * tcs2)
{
    const TCHAR *cp;
    int cc,count;
    int n2Len = _tcslen ( tcs2 );
    int n1Len = _tcslen ( tcs1 );

    if ( n1Len >= n2Len )
    {
        for ( cp = tcs1, count = n1Len - n2Len; count>=0 ; cp++,count-- )
        {
            cc = CompareString(LCID_SCRIPTING,
                NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE,
                cp, n2Len,tcs2, n2Len);
            if ( cc > 0 )
                cc-=2;
            if ( !cc )
                return cp;
        }
    }
    return NULL;
}

//+--------------------------------------------------------------------
//
//  Function:    AccessAllowed
//
//---------------------------------------------------------------------

BOOL
AccessAllowed(BSTR bstrUrl, IUnknown * pUnkSite)
{
    BOOL                fAccessAllowed = FALSE;
    HRESULT             hr;
    CComPtr<IBindHost>	pBindHost;
    CComPtr<IMoniker>	pMoniker;
    LPTSTR              pchUrl = NULL;
    BYTE                abSID1[MAX_SIZE_SECURITY_ID];
    BYTE                abSID2[MAX_SIZE_SECURITY_ID];
    DWORD               cbSID1 = ARRAY_SIZE(abSID1);
    DWORD               cbSID2 = ARRAY_SIZE(abSID2);
    CComPtr<IInternetSecurityManager>                   pSecurityManager;
    CComPtr<IInternetHostSecurityManager>               pHostSecurityManager;
    CComQIPtr<IServiceProvider, &IID_IServiceProvider>  pServiceProvider(pUnkSite);

    if (!pServiceProvider)
        goto Cleanup;

    //
    // expand url
    //

    hr = pServiceProvider->QueryService(SID_IBindHost, IID_IBindHost, (void**)&pBindHost);
    if (hr)
        goto Cleanup;

    hr = pBindHost->CreateMoniker(bstrUrl, NULL, &pMoniker, NULL);
    if (hr)
        goto Cleanup;

    hr = pMoniker->GetDisplayName(NULL, NULL, &pchUrl);
    if (hr)
        goto Cleanup;

    //
    // get security id of the url
    //

    hr = CoInternetCreateSecurityManager(NULL, &pSecurityManager, 0);
    if (hr)
        goto Cleanup;

    hr = pSecurityManager->GetSecurityId(pchUrl, abSID1, &cbSID1, NULL);
    if (hr)
        goto Cleanup;

    //
    // get security id of the document
    //

    hr = pServiceProvider->QueryService(
        IID_IInternetHostSecurityManager, IID_IInternetHostSecurityManager, (void**)&pHostSecurityManager);
    if (hr)
        goto Cleanup;

    hr = pHostSecurityManager->GetSecurityId(abSID2, &cbSID2, NULL);

    //
    // the security check itself
    //

    fAccessAllowed = (cbSID1 == cbSID2 && (0 == memcmp(abSID1, abSID2, cbSID1)));

Cleanup:
    if (pchUrl)
        CoTaskMemFree(pchUrl);

    return fAccessAllowed;
}