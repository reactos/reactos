//+ ---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dwnbind.cxx
//
//  Contents:   CDwnPost, CDwnBind
//
// ----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

extern class CResProtocolCF g_cfResProtocol;
extern class CAboutProtocolCF g_cfAboutProtocol;
extern class CViewSourceProtocolCF g_cfViewSourceProtocol;

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnBindInfo,      "Dwn", "Trace CDwnBindInfo")
PerfDbgTag(tagDwnBindData,      "Dwn", "Trace CDwnBindData")
PerfDbgTag(tagDwnBindDataIO,    "Dwn", "Trace CDwnBindData Peek/Read")
PerfDbgTag(tagDwnBindSlow,      "Dwn", "! Don't use InternetSession")
PerfDbgTag(tagNoWriteCache,     "Dwn", "! Force BINDF_NOWRITECACHE")
PerfDbgTag(tagNoReadCache,      "Dwn", "! Force BINDF_GETNEWESTVERSION")
PerfDbgTag(tagPushData,         "Dwn", "! Force PUSHDATA");
DeclareTag(tagDwnBindTrace,     "Dwn", "Trace CDwnBindInfo refs (next instance)")
DeclareTag(tagDwnBindTraceAll,  "Dwn", "Trace CDwnBindInfo refs (all instances)")
MtDefine(CDwnBindInfo, Dwn, "CDwnBindInfo")
MtDefine(CDwnBindData, Dwn, "CDwnBindData")
MtDefine(CDwnBindData_pbPeek, CDwnBindData, "CDwnBindData::_pbPeek")
MtDefine(CDwnBindData_pbRawEcho, Dwn, "CDwnBindData::_pbRawEcho")
MtDefine(CDwnBindData_pSecConInfo, Dwn, "CDwnBindData::_pSecConInfo")

PerfDbgExtern(tagPerfWatch)

// Globals --------------------------------------------------------------------

CDwnBindInfo *      g_pDwnBindTrace = NULL;

#if DBG==1 || defined(PERFTAGS)
static char * g_rgpchBindStatus[] = { "",
    "FINDINGRESOURCE","CONNECTING","REDIRECTING","BEGINDOWNLOADDATA",
    "DOWNLOADINGDATA","ENDDOWNLOADDATA","BEGINDOWNLOADCOMPONENTS",
    "INSTALLINGCOMPONENTS","ENDDOWNLOADCOMPONENTS","USINGCACHEDCOPY",
    "SENDINGREQUEST","CLASSIDAVAILABLE","MIMETYPEAVAILABLE",
    "CACHEFILENAMEAVAILABLE","BEGINSYNCOPERATION","ENDSYNCOPERATION",
    "BEGINUPLOADDATA","UPLOADINGDATA","ENDUPLOADDATA","PROTOCOLCLASSID",
    "ENCODING","VERIFIEDMIMETYPEAVAILABLE","CLASSINSTALLLOCATION",
    "DECODING","LOADINGMIMEHANDLER","CONTENTDISPOSITIONATTACH",
	"FILTERREPORTMIMETYPE","CLSIDCANINSTANTIATE","DLLNAMEAVAILABLE",
	"DIRECTBIND","RAWMIMETYPE","?","?","?","?","?","?","?","?","?",
    "?","?","?","?","?","?","?","?","?","?","?","?","?","?","?"
};
#endif

// Definitions ----------------------------------------------------------------

#define Align64(n)              (((n) + 63) & ~63)

// Utilities ------------------------------------------------------------------

BOOL GetFileLastModTime(TCHAR * pchFile, FILETIME * pftLastMod)
{
    WIN32_FIND_DATA fd;
    HANDLE hFF = FindFirstFile(pchFile, &fd);

    if (hFF != INVALID_HANDLE_VALUE)
    {
        *pftLastMod = fd.ftLastWriteTime;
        FindClose(hFF);
        return(TRUE);
    }

    return(FALSE);
}

BOOL GetUrlLastModTime(TCHAR * pchUrl, UINT uScheme, DWORD dwBindf, FILETIME * pftLastMod)
{
    BOOL    fRet = FALSE;
    HRESULT hr;

    Assert(uScheme == GetUrlScheme(pchUrl));

    if (uScheme == URL_SCHEME_FILE)
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(pchUrl, PARSE_PATH_FROM_URL, 0,
                    achPath, ARRAY_SIZE(achPath), &cchPath, 0));

        if (hr == S_OK)
        {
            fRet = GetFileLastModTime(achPath, pftLastMod);
        }
    }
    else if (uScheme == URL_SCHEME_HTTP || uScheme == URL_SCHEME_HTTPS)
    {
        fRet = !IsUrlCacheEntryExpired(pchUrl, dwBindf & BINDF_FWD_BACK, pftLastMod)
               && pftLastMod->dwLowDateTime
               && pftLastMod->dwHighDateTime;
    }

    return(fRet);
}

// CDwnBindInfo ---------------------------------------------------------------

CDwnBindInfo::CDwnBindInfo()
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::CDwnBindInfo");

    #if DBG==1
    if (    (g_pDwnBindTrace == NULL && IsTagEnabled(tagDwnBindTrace))
        ||  IsTagEnabled(tagDwnBindTraceAll))
    {
        g_pDwnBindTrace = this;
        TraceTag((0, "DwnBindInfo [%lX] Construct %d", this, GetRefs()));
        TraceCallers(0, 1, 12);
    }
    #endif
}

CDwnBindInfo::~CDwnBindInfo()
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::~CDwnBindInfo");

    #if DBG==1
    if (g_pDwnBindTrace == this || IsTagEnabled(tagDwnBindTraceAll))
    {
        g_pDwnBindTrace = NULL;
        TraceTag((0, "DwnBindInfo [%lX] Destruct", this));
        TraceCallers(0, 1, 12);
    }
    #endif

    if (_pDwnDoc)
        _pDwnDoc->Release();

    ReleaseInterface((IUnknown *)_pDwnPost);

    PerfDbgLog(tagDwnBindInfo, this, "-CDwnBindInfo::~CDwnBindInfo");
}

// CDwnBindInfo (IUnknown) --------------------------------------------------------

STDMETHODIMP
CDwnBindInfo::QueryInterface(REFIID iid, void **ppv)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::QueryInterface");
    Assert(CheckThread());

    if (iid == IID_IUnknown || iid == IID_IBindStatusCallback)
        *ppv = (IBindStatusCallback *)this;
    else if (iid == IID_IServiceProvider)
        *ppv = (IServiceProvider *)this;
    else if (iid == IID_IHttpNegotiate)
        *ppv = (IHttpNegotiate *)this;
    else if (iid == IID_IMarshal)
        *ppv = (IMarshal *)this;
    else if (iid == IID_IInternetBindInfo)
        *ppv = (IInternetBindInfo *)this;
    else if (iid == IID_IDwnBindInfo)
    {
        *ppv = this;
        AddRef();
        return(S_OK);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CDwnBindInfo::AddRef()
{
    ULONG ulRefs = super::AddRef();

    #if DBG==1
    if (this == g_pDwnBindTrace || IsTagEnabled(tagDwnBindTraceAll))
    {
        TraceTag((0, "[%lX] DwnBindInfo %lX AR  %ld",
            GetCurrentThreadId(), this, ulRefs));
        TraceCallers(0, 1, 12);
    }
    #endif

    PerfDbgLog1(tagDwnBindInfo, this, "CDwnBindInfo::AddRef (cRefs=%ld)",
        ulRefs);

    return(ulRefs);
}

STDMETHODIMP_(ULONG)
CDwnBindInfo::Release()
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::Release");

    ULONG ulRefs = super::Release();

    #if DBG==1
    if (this == g_pDwnBindTrace || IsTagEnabled(tagDwnBindTraceAll))
    {
        TraceTag((0, "[%lX] DwnBindInfo %lX Rel %ld",
            GetCurrentThreadId(), this, ulRefs));
        TraceCallers(0, 1, 12);
    }
    #endif

    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::Release (cRefs=%ld)",
        ulRefs);

    return(ulRefs);
}

void
CDwnBindInfo::SetDwnDoc(CDwnDoc * pDwnDoc)
{
    if (_pDwnDoc)
        _pDwnDoc->Release();

    _pDwnDoc = pDwnDoc;

    if (_pDwnDoc)
        _pDwnDoc->AddRef();
}

void
CDwnBindInfo::SetDwnPost(CDwnPost * pDwnPost)
{
    if (_pDwnPost)
        _pDwnPost->Release();

    _pDwnPost = pDwnPost;

    if (_pDwnPost)
        _pDwnPost->AddRef();
}

UINT
CDwnBindInfo::GetScheme()
{
    return(URL_SCHEME_UNKNOWN);
}

// CDwnBindInfo (IBindStatusCallback) -----------------------------------------

STDMETHODIMP
CDwnBindInfo::OnStartBinding(DWORD grfBSCOption, IBinding *pbinding)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnStartBinding");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::GetPriority(LONG *pnPriority)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::GetPriority");
    Assert(CheckThread());
    *pnPriority = NORMAL_PRIORITY_CLASS;
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnLowResource(DWORD dwReserved)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnLowResource");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode,
    LPCWSTR pszText)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnProgress");
    Assert(CheckThread());

    if (pszText && (ulCode == BINDSTATUS_MIMETYPEAVAILABLE || ulCode == BINDSTATUS_RAWMIMETYPE))
    {
        _cstrContentType.Set(pszText);
    }

    if (pszText && (ulCode == BINDSTATUS_CACHEFILENAMEAVAILABLE))
    {
        _cstrCacheFilename.Set(pszText);
    }
    
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnStopBinding(HRESULT hrReason, LPCWSTR szReason)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnStopBinding");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::GetBindInfo");
    Assert(CheckThread());

    HRESULT hr;

    if (pbindinfo->cbSize != sizeof(BINDINFO))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    memset(pbindinfo, 0, sizeof(BINDINFO));

    pbindinfo->cbSize = sizeof(BINDINFO);

    *pdwBindf = BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA;

    if (_pDwnDoc)
    {
        if (_fIsDocBind)
        {
            *pdwBindf |= _pDwnDoc->GetDocBindf();
            pbindinfo->dwCodePage = _pDwnDoc->GetURLCodePage();
        }
        else
        {
            *pdwBindf |= _pDwnDoc->GetBindf();
            pbindinfo->dwCodePage = _pDwnDoc->GetDocCodePage();
        }

        if (_pDwnDoc->GetLoadf() & DLCTL_URL_ENCODING_DISABLE_UTF8)
            pbindinfo->dwOptions = BINDINFO_OPTIONS_DISABLE_UTF8;
        else if (_pDwnDoc->GetLoadf() & DLCTL_URL_ENCODING_ENABLE_UTF8)
            pbindinfo->dwOptions = BINDINFO_OPTIONS_ENABLE_UTF8;
    }

#ifdef WINCE	// WINCEREVIEW - temp until we have caching support
	*pdwBindf |= BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE;
#endif // WINCE

    #if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagNoWriteCache))
        *pdwBindf |= BINDF_NOWRITECACHE;
    if (IsPerfDbgEnabled(tagNoReadCache))
        *pdwBindf |= BINDF_GETNEWESTVERSION;
    if (IsPerfDbgEnabled(tagPushData))
        *pdwBindf &= ~BINDF_PULLDATA;
    #endif

    if (_pDwnPost)
    {
        hr = THR(_pDwnPost->GetBindInfo(pbindinfo));
        if (hr)
            goto Cleanup;

        // If local cache is not demanded,
        // Then require POSTs to go all the way to the originating server
        if (!(*pdwBindf & BINDF_OFFLINEOPERATION))
        {
            *pdwBindf |= BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE;
            *pdwBindf &= ~BINDF_RESYNCHRONIZE;
        }

        pbindinfo->dwBindVerb = BINDVERB_POST;
    }
    else
    {
        // If a GET method for a form, always hit the server
        if (!(*pdwBindf & BINDF_OFFLINEOPERATION) && (*pdwBindf & BINDF_FORMS_SUBMIT))
        {
            *pdwBindf &= ~(BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE);
            *pdwBindf |= BINDF_RESYNCHRONIZE;
        }
        pbindinfo->dwBindVerb = BINDVERB_GET;
    }



    hr = S_OK;

Cleanup:
    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::GetBindInfo (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC * pformatetc, STGMEDIUM * pstgmed)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnDataAvailable");
    Assert(CheckThread());
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::OnObjectAvailable");
    Assert(CheckThread());
    return(S_OK);
}

// CDwnBindInfo (IInternetBindInfo) -------------------------------------------

STDMETHODIMP
CDwnBindInfo::GetBindString(ULONG ulStringType, LPOLESTR * ppwzStr,
    ULONG cEl, ULONG * pcElFetched)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::GetBindString %s",
        ulStringType == BINDSTRING_HEADERS          ? "HEADERS" :
        ulStringType == BINDSTRING_ACCEPT_MIMES     ? "ACCEPT_MIMES" :
        ulStringType == BINDSTRING_EXTRA_URL        ? "EXTRA_URL" :
        ulStringType == BINDSTRING_LANGUAGE         ? "LANGUAGE" :
        ulStringType == BINDSTRING_USERNAME         ? "USERNAME" :
        ulStringType == BINDSTRING_PASSWORD         ? "PASSWORD" :
        ulStringType == BINDSTRING_UA_PIXELS        ? "UA_PIXELS" :
        ulStringType == BINDSTRING_UA_COLOR         ? "UA_COLOR" :
        ulStringType == BINDSTRING_OS               ? "OS" :
        ulStringType == BINDSTRING_ACCEPT_ENCODINGS ? "ACCEPT_ENCODINGS" :
        ulStringType == BINDSTRING_POST_DATA_MIME   ? "POST_DATA_MIME" :
        "???");

    HRESULT hr = S_OK;

    *pcElFetched = 0;

    switch (ulStringType)
    {
        case BINDSTRING_ACCEPT_MIMES:
            {
                if (cEl >= 1)
                {
                    ppwzStr[0] = (LPOLESTR)CoTaskMemAlloc(4 * sizeof(TCHAR));

                    if (ppwzStr[0] == 0)
                    {
                        hr = E_OUTOFMEMORY;
                        goto Cleanup;
                    }

                    memcpy(ppwzStr[0], _T("*/*"), 4 * sizeof(TCHAR));
                    *pcElFetched = 1;
                }
            }
            break;

        case BINDSTRING_POST_COOKIE:
            {
                if (cEl >= 1 && _pDwnPost)
                {
                    hr = THR(_pDwnPost->GetHashString(&(ppwzStr[0])));

                    *pcElFetched = hr ? 0 : 1;
                }
            }
            break;

        case BINDSTRING_POST_DATA_MIME:
            {
                if (cEl >= 1 && _pDwnPost)
                {
                    LPCTSTR pcszEncoding = _pDwnPost->GetEncodingString();
                    
                    if (pcszEncoding)
                    {
                        DWORD   dwSize = sizeof(TCHAR) + 
                                    _tcslen(pcszEncoding) * sizeof(TCHAR);

                        ppwzStr[0] = (LPOLESTR)CoTaskMemAlloc(dwSize);
                        if (ppwzStr[0])
                        {
                            memcpy(ppwzStr[0], pcszEncoding, dwSize);
                            *pcElFetched = 1;
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {   // we don't know, use the default instead
                        hr = INET_E_USE_DEFAULT_SETTING;
                    }
                }

            }
            break;
    }

Cleanup:
    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::GetBindString (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnBindInfo (IServiceProvider) --------------------------------------------

STDMETHODIMP
CDwnBindInfo::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::QueryService");
    Assert(CheckThread());

    HRESULT hr;

    if (rguidService == IID_IHttpNegotiate)
    {
        hr = QueryInterface(riid, ppvObj);
    }
    else if (rguidService == IID_IInternetBindInfo)
    {
        hr = QueryInterface(riid, ppvObj);
    }
    else if (_pDwnDoc)
    {
        hr = _pDwnDoc->QueryService(IsBindOnApt(), rguidService, riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::QueryService (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnBindInfo (IHttpNegotiate) ----------------------------------------------

STDMETHODIMP
CDwnBindInfo::BeginningTransaction(LPCWSTR pszUrl, LPCWSTR pszHeaders,
    DWORD dwReserved, LPWSTR * ppszHeaders)
{
    PerfDbgLog1(tagDwnBindInfo, this, "+CDwnBindInfo::BeginningTransaction "
        "\"%ls\"", pszUrl ? pszUrl : g_Zero.ach);
    Assert(CheckThread());

    LPCTSTR     apch[16];
    UINT        acch[16];
    LPCTSTR *   ppch = apch;
    UINT *      pcch = acch;
    HRESULT     hr   = S_OK;

    // If we have been told the exact http headers to use, use them now
    
    if (_fIsDocBind && _pDwnDoc && _pDwnDoc->GetRequestHeaders())
    {
        TCHAR *     pch;
        UINT        cch = _pDwnDoc->GetRequestHeadersLength();
        
        pch = (TCHAR *)CoTaskMemAlloc(cch * sizeof(TCHAR));

        if (pch == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // note: we don't need to convert an extra zero terminator, so cch-1
        
        AnsiToWideTrivial((char *)_pDwnDoc->GetRequestHeaders(), pch, cch - 1);

        *ppszHeaders = pch;

        goto Cleanup;
    }
    
    // Otherwise, assemble the http headers
    
    *ppszHeaders = NULL;

    if (_pDwnDoc)
    {
        LPCTSTR pch;

        pch = _fIsDocBind ? _pDwnDoc->GetDocReferer() :
                _pDwnDoc->GetSubReferer();

        if (pch)
        {
            UINT uSchemeSrc;
            UINT uSchemeDst;

            if (_fIsDocBind)
                uSchemeSrc = _pDwnDoc->GetDocRefererScheme();
            else
                uSchemeSrc = _pDwnDoc->GetSubRefererScheme();

            uSchemeDst = GetScheme();

            if (uSchemeDst == URL_SCHEME_UNKNOWN && pszUrl)
                uSchemeDst = GetUrlScheme(pszUrl);

            // Don't send a referer which isn't http: or https:, it's none
            // of the servers' business.  Further, don't send an https:
            // referer when requesting an http: file.

            if (    (   uSchemeSrc == URL_SCHEME_HTTP
                    ||  uSchemeSrc == URL_SCHEME_HTTPS)
                &&  (   uSchemeSrc == uSchemeDst
                    ||  uSchemeDst == URL_SCHEME_HTTPS))
            {
                UINT    cch = _tcslen(pch);
                UINT    ich = 0;

                *ppch++ = _T("Referer: ");
                *pcch++ = 9;
                *ppch++ = pch;

                for (; ich < cch; ++ich, ++pch)
                {
                    if (*pch == ':')
                    {
                        // Skip past all slashes to find the beginning of the host

                        for (++ich, ++pch; ich < cch; ++ich, ++pch)
                        {
                            if (*pch != '/')
                                break;
                        }
                        break;
                    }
                }

                *pcch++ = ich;

                if (ich < cch)
                {
                    for (; ich < cch; ++ich, ++pch)
                    {
                        if (*pch == '@')
                        {
                            *ppch++ = pch + 1;
                            *pcch++ = cch - ich - 1;
                            goto zapped;
                        }
                        else if (*pch == '/')
                            break;
                    }

                    // No username or password, so just change the last
                    // fragment to include the entire string.

                    pcch[-1] = cch;
                }
                
            zapped:

                *ppch++ = _T("\r\n");
                *pcch++ = 2;
            }
        }

        pch = _pDwnDoc->GetAcceptLanguage();

        if (pch)
        {
            *ppch++ = _T("Accept-Language: ");
            *pcch++ = 17;
            *ppch++ = pch;
            *pcch++ = _tcslen(pch);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }

        pch = _pDwnDoc->GetUserAgent();

        if (pch)
        {
            *ppch++ = _T("User-Agent: ");
            *pcch++ = 12;
            *ppch++ = pch;
            *pcch++ = _tcslen(pch);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }
    }

    if (_pDwnPost)
    {
        LPCTSTR pchEncoding = _pDwnPost->GetEncodingString();

        if (pchEncoding)
        {
            *ppch++ = _T("Content-Type: ");
            *pcch++ = 14;
            *ppch++ = pchEncoding;
            *pcch++ = _tcslen(pchEncoding);
            *ppch++ = _T("\r\n");
            *pcch++ = 2;
        }
        // KB: If we can't determine the Content-Type, we should not submit
        // anything thereby allowing the server to sniff the incoming data
        // and make its own determination.
    }

    if (ppch > apch)
    {
        LPCTSTR *   ppchEnd = ppch;
        TCHAR *     pch;
        UINT        cch = 0;

        for (; ppch > apch; --ppch)
            cch += *--pcch;

        pch = (TCHAR *)CoTaskMemAlloc((cch + 1) * sizeof(TCHAR));

        if (pch == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        *ppszHeaders = pch;

        for (; ppch < ppchEnd; pch += *pcch++, ppch++)
        {
            memcpy(pch, *ppch, *pcch * sizeof(TCHAR));
        }

        *pch = 0;

        Assert((UINT)(pch - *ppszHeaders) == cch);
    }

Cleanup:
    PerfDbgLog1(tagDwnBindInfo, this, "-CDwnBindInfo::BeginningTransaction (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders, LPWSTR * ppszAdditionalRequestHeaders)
{
    PerfDbgLog(tagDwnBindInfo, this, "+CDwnBindInfo::OnResponse");
    Assert(CheckThread());
    PerfDbgLog(tagDwnBindInfo, this, "-CDwnBindInfo::OnResponse (hr=0)");
    return(S_OK);
}

// CDwnBindInfo (IMarshal) --------------------------------------------------------

STDMETHODIMP
CDwnBindInfo::GetUnmarshalClass(REFIID riid, void *pvInterface,
    DWORD dwDestContext, void * pvDestContext, DWORD mshlflags, CLSID * pclsid)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::GetUnmarshalClass");
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,
            pvDestContext, mshlflags);

    if (hr == S_OK)
    {
        *pclsid = CLSID_CDwnBindInfo;
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::GetMarshalSizeMax(REFIID riid, void * pvInterface,
    DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD * pdwSize)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::GetMarshalSizeMax");
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,
            pvDestContext, mshlflags);

    if (hr == S_OK)
    {
        *pdwSize = sizeof(BYTE) +                       // fByRef
            ((dwDestContext == MSHCTX_INPROC) ?
                sizeof(CDwnBindInfo *) :                // _pDwnBindInfo
                CDwnDoc::GetSaveSize(_pDwnDoc)          // _pDwnDoc
                + CDwnPost::GetSaveSize(_pDwnPost));    // _pDwnPost
    }
    else
    {
        *pdwSize = 0;
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::MarshalInterface(IStream * pstm, REFIID riid,
    void *pvInterface, DWORD dwDestContext,
    void *pvDestContext, DWORD mshlflags)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::MarshalInterface");
    BYTE fByRef = (dwDestContext == MSHCTX_INPROC);
    HRESULT hr;

    hr = ValidateMarshalParams(riid, pvInterface, dwDestContext,
            pvDestContext, mshlflags);

    if (hr == S_OK)
        hr = pstm->Write(&fByRef, sizeof(BYTE), NULL);
    if (hr == S_OK)
    {
        if (fByRef)
        {
            CDwnBindInfo * pDwnBindInfo = this;
            hr = pstm->Write(&pDwnBindInfo, sizeof(CDwnBindInfo *), NULL);
            if (hr == S_OK)
                pDwnBindInfo->AddRef();
        }
        else
        {
            hr = CDwnDoc::Save(_pDwnDoc, pstm);
            if (hr == S_OK)
                hr = CDwnPost::Save(_pDwnPost, pstm);
         }
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::UnmarshalInterface(IStream * pstm, REFIID riid, void ** ppvObj)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::UnmarshalInterface");
    BYTE fByRef = FALSE;
    HRESULT hr;

    *ppvObj = NULL;

    hr = CanMarshalIID(riid) ? S_OK : E_NOINTERFACE;
    if (hr == S_OK)
        hr = pstm->Read(&fByRef, sizeof(BYTE), NULL);
    if (hr == S_OK)
    {
        CDwnBindInfo * pDwnBindInfo = NULL;

        if (fByRef)
        {
            hr = pstm->Read(&pDwnBindInfo, sizeof(CDwnBindInfo *), NULL);
        }
        else
        {
            hr = CDwnDoc::Load(pstm, &_pDwnDoc);
            if (hr == S_OK)
                hr = CDwnPost::Load(pstm, &_pDwnPost);

            pDwnBindInfo = this;
            pDwnBindInfo->AddRef();
        }

        if (hr == S_OK)
        {
            hr = pDwnBindInfo->QueryInterface(riid, ppvObj);
            pDwnBindInfo->Release();
        }
    }

    RRETURN(hr);
}

STDMETHODIMP
CDwnBindInfo::ReleaseMarshalData(IStream * pstm)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::ReleaseMarshalData");
    return(S_OK);
}

STDMETHODIMP
CDwnBindInfo::DisconnectObject(DWORD dwReserved)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindInfo::DisconnectObject");
    return(S_OK);
}

BOOL
CDwnBindInfo::CanMarshalIID(REFIID riid)
{
    return(riid == IID_IUnknown
        || riid == IID_IBindStatusCallback
        || riid == IID_IServiceProvider
        || riid == IID_IHttpNegotiate);
}

HRESULT
CDwnBindInfo::ValidateMarshalParams(REFIID riid, void *pvInterface,
    DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    HRESULT hr = S_OK;
 
    if (!CanMarshalIID(riid))
        hr = E_NOINTERFACE;
    else if (   (   dwDestContext != MSHCTX_INPROC
                &&  dwDestContext != MSHCTX_LOCAL
                &&  dwDestContext != MSHCTX_NOSHAREDMEM)
            || (    mshlflags != MSHLFLAGS_NORMAL
                &&  mshlflags != MSHLFLAGS_TABLESTRONG))
        hr = E_INVALIDARG;
    else
        hr = S_OK;

    RRETURN(hr);
}

// CDwnBindInfo (Internal) ----------------------------------------------------

HRESULT
CreateDwnBindInfo(IUnknown *pUnkOuter, IUnknown **ppUnk)
{
    PerfDbgLog(tagDwnBindInfo, NULL, "CreateDwnBindInfo");

    *ppUnk = NULL;

    if (pUnkOuter != NULL)
        RRETURN(CLASS_E_NOAGGREGATION);

    CDwnBindInfo * pDwnBindInfo = new CDwnBindInfo;

    if (pDwnBindInfo == NULL)
        RRETURN(E_OUTOFMEMORY);

    *ppUnk = (IBindStatusCallback *)pDwnBindInfo;
    
    return(S_OK);
}

// CDwnBindData (IUnknown) ----------------------------------------------------

STDMETHODIMP
CDwnBindData::QueryInterface(REFIID iid, void **ppv)
{
    PerfDbgLog(tagDwnBindInfo, this, "CDwnBindData::QueryInterface");
    Assert(CheckThread());

    *ppv = NULL;

    if (iid == IID_IInternetBindInfo)
        *ppv = (IInternetBindInfo *)this;
    else if (iid == IID_IInternetProtocolSink)
        *ppv = (IInternetProtocolSink *)this;
    else
        return(super::QueryInterface(iid, ppv));

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CDwnBindData::AddRef()
{
    return(super::AddRef());
}

STDMETHODIMP_(ULONG)
CDwnBindData::Release()
{
    return(super::Release());
}

// CDwnBindData (Internal) ----------------------------------------------------

CDwnBindData::~CDwnBindData()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::~CDwnBindData");

    Assert(!_fBindOnApt || _u.pts == NULL);

    if (_pDwnStm)
        _pDwnStm->Release();

    MemFree(_pbPeek);

    if (!_fBindOnApt && _o.pInetProt)
    {
        _o.pInetProt->Release();
    }

    if (_hLock)
    {
        InternetUnlockRequestFile(_hLock);
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::~CDwnBindData");
}

void
CDwnBindData::Passivate()
{
    if (!_fBindTerm)
    {
        Terminate(E_ABORT);
    }

    super::Passivate();
}

void
CDwnBindData::Terminate(HRESULT hrErr)
{
    if (_fBindTerm)
        return;

    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::Terminate (hrErr=%lX)", hrErr);

    BOOL fTerminate = FALSE;

    g_csDwnBindTerm.Enter();

    if (_fBindTerm)
        goto Cleanup;

    if (_hrErr == S_OK)
    {
        _hrErr = hrErr;
    }

    if (_fBindOnApt)
    {
        if (    !_u.pstm
            &&  !_u.punkForRel
            &&  !_u.pbc
            &&  !_u.pbinding
            &&  !_u.pts)
            goto Cleanup;

        if (_u.dwTid != GetCurrentThreadId())
        {
            if (!_u.pts || _u.fTermPosted)
                goto Cleanup;

            // We're not on the apartment thread, so we can't access
            // the objects we're binding right now.  Post a callback
            // on the apartment thread.

            PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Terminate "
                "GWPostMethodCall");

            // SubAddRef and set flags before posting the message to avoid race
            SubAddRef();
            _u.fTermPosted = TRUE;
            _u.fTermReceived = FALSE;

            HRESULT hr = GWPostMethodCallEx(_u.pts, this,
                ONCALL_METHOD(CDwnBindData, TerminateOnApt, terminateonapt), 0, FALSE, "CDwnBindData::TerminateOnApt");

            PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::Terminate "
                "GWPostMethodCall (hr=%lX)", hr);

            if (hr)
            {
            	_u.fTermReceived = TRUE;
                SubRelease();
            }

            goto Cleanup;
        }
    }

    fTerminate = TRUE;

Cleanup:
    g_csDwnBindTerm.Leave();

    if (fTerminate)
    {
        TerminateBind();
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::Terminate");
}

void BUGCALL
CDwnBindData::TerminateOnApt(DWORD_PTR dwContext)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::TerminateOnApt");

    Assert(!_u.fTermReceived);

    _u.fTermReceived = TRUE;
    TerminateBind();
    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::TerminateOnApt");
}

void
CDwnBindData::TerminateBind()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::TerminateBind");
    Assert(CheckThread());

    SubAddRef();

    g_csDwnBindTerm.Enter();

    if (_pDwnStm && !_pDwnStm->IsEofWritten())
        _pDwnStm->WriteEof(_hrErr);

    SetEof();

    if (_fBindOnApt)
    {
        IUnknown * punkRel1 = _u.pstm;
        IUnknown * punkRel2 = _u.punkForRel;
        IBindCtx * pbc      = NULL;
        IBinding * pbinding = NULL;
        BOOL       fDoAbort = FALSE;
        BOOL       fRelTls  = FALSE;

        _u.pstm             = NULL;
        _u.punkForRel       = NULL;

        if (_u.pts && _u.fTermPosted && !_u.fTermReceived)
        {
            // We've posted a method call to TerminateOnApt which hasn't been received
            // yet.  This happens if Terminate gets called on the apartment thread
            // before messages are pumped.  To keep reference counts happy, we need
            // to simulate the receipt of the method call here by first killing any
            // posted method call and then undoing the SubAddRef.

            GWKillMethodCallEx(_u.pts, this,
                  ONCALL_METHOD(CDwnBindData, TerminateOnApt, terminateonapt), 0);
                  
            // note: no danger of post/kill/set-flag race because
            // we're protected by g_csDwnBindTerm
            
            _u.fTermReceived = TRUE;
            SubRelease();
        }

        if (_fBindDone)
        {
            pbc         = _u.pbc;
            _u.pbc      = NULL;
            pbinding    = _u.pbinding;
            _u.pbinding = NULL;
            fRelTls     = !!_u.pts;
            _u.pts      = NULL;
            _fBindTerm  = TRUE;
        }
        else if (!_fBindAbort && _u.pbinding)
        {
            pbinding    = _u.pbinding;
            fDoAbort    = TRUE;
            _fBindAbort = TRUE;
        }

        g_csDwnBindTerm.Leave();

        ReleaseInterface(punkRel1);
        ReleaseInterface(punkRel2);

        if (fDoAbort)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Aborting IBinding)");
            pbinding->Abort();
        }
        else
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Release IBinding)");
            ReleaseInterface(pbinding);
        }

        if (pbc)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Release IBindCtx)");
            IGNORE_HR(RevokeBindStatusCallback(pbc, this));
            ReleaseInterface(pbc);
        }

        if (fRelTls)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (ReleaseThreadState)");
            ReleaseThreadState(&_u.dwObjCnt);
        }
    }
    else if (_o.pInetProt)
    {
        BOOL fDoTerm    = _fBindDone && !_fBindTerm;
        BOOL fDoAbort   = !_fBindDone && !_fBindAbort;

        if (_fBindDone)
            _fBindTerm = TRUE;
        else
            _fBindAbort = TRUE;

        g_csDwnBindTerm.Leave();

        if (fDoAbort)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Abort IInternetProtocol)");
            _o.pInetProt->Abort(E_ABORT, 0);
        }
        else if (fDoTerm)
        {
            PerfDbgLog(tagDwnBindData, this, "CDwnBindData::TerminateBind (Terminate IInternetProtocol)");
            _o.pInetProt->Terminate(0);
        }
    }
    else
    {
        _fBindTerm = TRUE;
        g_csDwnBindTerm.Leave();
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::TerminateBind");
}

HRESULT
CDwnBindData::SetBindOnApt()
{
    HRESULT hr;

    if (!(_dwFlags & DWNF_NOAUTOBUFFER))
    {
        _pDwnStm = new CDwnStm;

        if (_pDwnStm == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(AddRefThreadState(&_u.dwObjCnt));
    if (hr)
        goto Cleanup;

    _u.pts      = GetThreadState();
    _u.dwTid    = GetCurrentThreadId();
    _fBindDone  = TRUE;
    _fBindOnApt = TRUE;

Cleanup:
    RRETURN(hr);
}

#if DBG==1
BOOL
CDwnBindData::CheckThread()
{
    return(!_fBindOnApt || _u.dwTid == GetCurrentThreadId());
}
#endif

void
CDwnBindData::OnDwnDocCallback(void * pvArg)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::OnDwnDocCallback");

    Assert(!_fBindOnApt);

    if (_o.pInetProt)
    {
        HRESULT hr = THR(_o.pInetProt->Continue((PROTOCOLDATA *)pvArg));

        if (hr)
        {
            SignalDone(hr);
        }
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnDwnDocCallback");
}

void
CDwnBindData::SetEof()
{
    PerfDbgLog(tagDwnBindDataIO, this, "+CDwnBindData::SetEof");

    g_csDwnBindPend.Enter();

    _fPending = FALSE;
    _fEof = TRUE;

    g_csDwnBindPend.Leave();

    PerfDbgLog(tagDwnBindDataIO, this, "-CDwnBindData::SetEof");
}

void
CDwnBindData::SetPending(BOOL fPending)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::SetPending %s",
        fPending ? "TRUE" : "FALSE");

    g_csDwnBindPend.Enter();

    if (!_fEof)
    {
        _fPending = fPending;
    }

    g_csDwnBindPend.Leave();

    PerfDbgLog(tagDwnBindDataIO, this, "-CDwnBindData::SetPending");
}

// CDwnBindData (Binding) -----------------------------------------------------

void
CDwnBindData::Bind(DWNLOADINFO * pdli, DWORD dwFlagsExtra)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::Bind");
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Bind");

    LPTSTR      pchAlloc = NULL;
    IMoniker *  pmkAlloc = NULL;
    IBindCtx *  pbcAlloc = NULL;
    IStream *   pstm     = NULL;
    CDwnDoc *   pDwnDoc  = pdli->pDwnDoc;
    IBindHost * pbh      = pDwnDoc ? pDwnDoc->GetBindHost() : NULL;
    LPCTSTR     pchUrl;
    IMoniker *  pmk;
    IBindCtx *  pbc;
    HRESULT     hr;
    
    _dwFlags = dwFlagsExtra;

    if (pDwnDoc)
    {
        _dwFlags |= pDwnDoc->GetDownf();
    }

    if (_dwFlags & DWNF_ISDOCBIND)
    {
        SetIsDocBind();
    }

    // Case 1: Binding to an user-supplied IStream

    if (pdli->pstm)
    {
        Assert(!pdli->fForceInet);

        _dwSecFlags = (!pdli->fUnsecureSource && IsUrlSecure(pdli->pchUrl)) ? SECURITY_FLAG_SECURE : 0;

        hr = THR(SetBindOnApt());
        if (hr)
            goto Cleanup;

        ReplaceInterface(&_u.pstm, pdli->pstm);
        BufferData();
        SignalData();
        goto Cleanup;
    }

    // Case 2: Not binding to a moniker or URL.  This is a manual binding
    // where data will be provided externally and shunted to the consumer.
    // Actual configuration of buffering will occur outside this function.
        
    if (pdli->fClientData || (!pdli->pmk && !pdli->pchUrl))
    {
        _dwSecFlags = (!pdli->fUnsecureSource && IsUrlSecure(pdli->pchUrl)) ? SECURITY_FLAG_SECURE : 0;
        hr = S_OK;
        goto Cleanup;
    }

    // Case 3: Binding asynchronously with IInternetSession

    pchUrl = pdli->pchUrl;

    if (!pchUrl && IsAsyncMoniker(pdli->pmk) == S_OK)
    {
        hr = THR(pdli->pmk->GetDisplayName(NULL, NULL, &pchAlloc));
        if (hr)
            goto Cleanup;

        pchUrl = pchAlloc;
    }

    // Since INTERNET_OPTION_SECURITY_FLAGS doesn't work for non-wininet URLs
    _uScheme = pchUrl ? GetUrlScheme(pchUrl) : URL_SCHEME_UNKNOWN;

    // Check _uScheme first before calling IsUrlSecure to avoid second (slow) GetUrlScheme call
    _dwSecFlags = (_uScheme != URL_SCHEME_HTTP &&
                   _uScheme != URL_SCHEME_FILE &&
                   !pdli->fUnsecureSource &&
                   IsUrlSecure(pchUrl)) ? SECURITY_FLAG_SECURE : 0;
    
    if (    _uScheme == URL_SCHEME_FILE
        &&  (_dwFlags & (DWNF_GETFILELOCK|DWNF_GETMODTIME)))
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(pchUrl, PARSE_PATH_FROM_URL, 0,
                    achPath, ARRAY_SIZE(achPath), &cchPath, 0));
        if (hr)
            goto Cleanup;

        hr = THR(_cstrFile.Set(achPath));
        if (hr)
            goto Cleanup;

        if (_cstrFile && (_dwFlags & DWNF_GETMODTIME))
        {
            GetFileLastModTime(_cstrFile, &_ftLastMod);
        }
    }

#ifndef _MAC // Temporarily disable this code so we can get images to load until IInternetSession is fully implemented
    if (    pchUrl
        &&  pdli->pInetSess
        &&  !pbh
        &&  !pdli->pbc
        &&  (   _uScheme == URL_SCHEME_FILE
            ||  _uScheme == URL_SCHEME_HTTP
            ||  _uScheme == URL_SCHEME_HTTPS)
        #if DBG==1 || defined(PERFTAGS)
        &&  !IsPerfDbgEnabled(tagDwnBindSlow)
        #endif
        )
    {
        hr = THR(pdli->pInetSess->CreateBinding(NULL, pchUrl, NULL,
                    NULL, &_o.pInetProt, OIBDG_DATAONLY));
        if (hr)
            goto Cleanup;

        if (!_fIsDocBind)
        {
            IOInetPriority * pOInetPrio = NULL;

            _o.pInetProt->QueryInterface(IID_IInternetPriority, (void **)&pOInetPrio);

            if (pOInetPrio)
            {
                IGNORE_HR(pOInetPrio->SetPriority(THREAD_PRIORITY_BELOW_NORMAL));
                pOInetPrio->Release();
            }
        }

        hr = THR(_o.pInetProt->Start(pchUrl, this, this,
                    PI_MIMEVERIFICATION, 0));

        // That's it.  We're off ...

        goto Cleanup;
    }

    if (pdli->fForceInet)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
#endif // _MAC

    // Case 4: Binding through URL moniker on apartment thread

    pmk = pdli->pmk;

    if (pmk == NULL)
    {
        hr = THR(CreateURLMoniker(NULL, pchUrl, &pmkAlloc));
        if (hr)
            goto Cleanup;
    
        pmk = pmkAlloc;
    }

    hr = THR(SetBindOnApt());
    if (hr)
        goto Cleanup;

    pbc = pdli->pbc;

    if (pbc == NULL)
    {
        hr = THR(CreateBindCtx(0, &pbcAlloc));
        if (hr)
            goto Cleanup;

        pbc = pbcAlloc;
    }

    ReplaceInterface(&_u.pbc, pbc);

    if (pbh)
    {
        hr = THR(pbh->MonikerBindToStorage(pmk, pbc, this,
                IID_IStream, (void **)&pstm));
        if (FAILED(hr))
            goto Cleanup;
    }
    else
    {
        hr = THR(RegisterBindStatusCallback(pbc, this, 0, 0));
        if (FAILED(hr))
            goto Cleanup;

        // If aggregated, save the pUnkOuter in the bind context
        if (_pDwnDoc->GetCDoc() && _pDwnDoc->GetCDoc()->IsAggregated())
        {
            // we're being aggregated
            // don't need to add ref, should be done by routine
            hr = pbc->RegisterObjectParam(L"AGG", _pDwnDoc->GetCDoc()->PunkOuter());
            if (FAILED(hr))
                goto Cleanup;
        }

        hr = THR(pmk->BindToStorage(pbc, NULL, IID_IStream, (void **)&pstm));

        if (FAILED(hr))
        {
            IGNORE_HR(RevokeBindStatusCallback(pbc, this));
            goto Cleanup;
        }
    }

    if (pstm)
    {
        ReplaceInterface(&_u.pstm, pstm);
        BufferData();
        SignalData();
    }

    hr = S_OK;

Cleanup:

    // If failed to start binding, signal done
    if (hr)
    {
        SignalDone(hr);
    }
    
    ReleaseInterface(pbcAlloc);
    ReleaseInterface(pmkAlloc);
    ReleaseInterface(pstm);
    CoTaskMemFree(pchAlloc);

    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::Bind (returning void, hr=%lX)", hr);
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::Bind");
}

// CDwnBindData (Reading) ---------------------------------------------------------

HRESULT
CDwnBindData::Peek(void * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::Peek (req %ld)", cb);

    ULONG   cbRead;
    ULONG   cbPeek = _pbPeek ? *(ULONG *)_pbPeek : 0;
    HRESULT hr = S_OK;

    *pcb = 0;

    if (cb > cbPeek)
    {
        if (Align64(cb) > Align64(cbPeek))
        {
            hr = THR(MemRealloc(Mt(CDwnBindData_pbPeek),
                        (void **)&_pbPeek, sizeof(ULONG) + Align64(cb)));
            if (hr)
                goto Cleanup;
        }

        cbRead = 0;

        hr = THR(ReadFromData(_pbPeek + sizeof(ULONG) + cbPeek,
                    cb - cbPeek, &cbRead));
        if (hr)
            goto Cleanup;

        cbPeek += cbRead;
        *(ULONG *)_pbPeek = cbPeek;

        if (cbPeek == 0)
        {
            // We don't want the state where _pbPeek exists but has no peek
            // data.  The IsEof and IsPending functions assume this won't
            // happen.

            MemFree(_pbPeek);
            _pbPeek = NULL;
        }

    }

    if (cb > cbPeek)
        cb = cbPeek;

    if (cb > 0)
        memcpy(pv, _pbPeek + sizeof(ULONG), cb);

    *pcb = cb;

Cleanup:
    PerfDbgLog3(tagDwnBindDataIO, this, "-CDwnBindData::Peek (%ld bytes) %c%c",
        *pcb, IsPending() ? 'P' : ' ', IsEof() ? 'E' : ' ');
    RRETURN(hr);
}

HRESULT
CDwnBindData::Read(void * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::Read (req %ld)", cb);

    ULONG   cbRead  = 0;
    ULONG   cbPeek  = _pbPeek ? *(ULONG *)_pbPeek : 0;
    HRESULT hr      = S_OK;

    if (cbPeek)
    {
        cbRead = (cb > cbPeek) ? cbPeek : cb;

        memcpy(pv, _pbPeek + sizeof(ULONG), cbRead);

        if (cbRead == cbPeek)
        {
            MemFree(_pbPeek);
            _pbPeek = NULL;
        }
        else
        {
            memmove(_pbPeek + sizeof(ULONG), _pbPeek + sizeof(ULONG) + cbRead, 
                cbPeek - cbRead);
            *(ULONG *)_pbPeek -= cbRead;
        }

        cb -= cbRead;
        pv = (BYTE *)pv + cbRead;
    }

    *pcb = cbRead;

    if (cb)
    {
        hr = THR(ReadFromData(pv, cb, pcb));

        if (hr)
            *pcb  = cbRead;
        else
            *pcb += cbRead;
    }

    if (_pDwnDoc)
        _pDwnDoc->AddBytesRead(*pcb);

    PerfDbgLog3(tagDwnBindDataIO, this, "-CDwnBindData::Read (got %ld) %c%c",
        *pcb, IsPending() ? 'P' : ' ', IsEof() ? 'E' : ' ');

    RRETURN(hr);
}
    
HRESULT
CDwnBindData::ReadFromData(void * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::ReadFromData (cb=%ld)", cb);

    BOOL fBindDone = _fBindDone;
    HRESULT hr;

    if (_pDwnStm)
        hr = THR(_pDwnStm->Read(pv, cb, pcb));
    else
        hr = THR(ReadFromBind(pv, cb, pcb));

    if (hr || (fBindDone && IsEof()))
    {
        SignalDone(hr);
    }

    PerfDbgLog2(tagDwnBindDataIO, this,
        "-CDwnBindData::ReadFromData (*pcb=%ld,hr=%lX)", *pcb, hr);

    RRETURN(hr);
}

HRESULT
CDwnBindData::ReadFromBind(void * pv, ULONG cb, ULONG * pcb)
{
    PerfDbgLog1(tagDwnBindDataIO, this, "+CDwnBindData::ReadFromBind (cb=%ld)", cb);
    Assert(CheckThread());

    HRESULT hr;

    #if DBG==1 || defined(PERFTAGS)
    BOOL fBindDone = _fBindDone;
    #endif

    *pcb = 0;

    if (_fEof)
        hr = S_FALSE;
    else if (!_fBindOnApt)
    {
        if (_o.pInetProt)
        {
            SetPending(TRUE);

            hr = _o.pInetProt->Read(pv, cb, pcb);

            PerfDbgLog2(tagDwnBindDataIO, this,
                "CDwnBindData::ReadFromBind InetProt::Read (*pcb=%ld,hr=%lX)",
                *pcb, hr);
        }
        else
            hr = S_FALSE;
    }
    else if (_u.pstm)
    {
        SetPending(TRUE);

        hr = _u.pstm->Read(pv, cb, pcb);

        PerfDbgLog2(tagDwnBindDataIO, this,
            "CDwnBindData::ReadFromBind IStream::Read (*pcb=%ld,hr=%lX)",
            *pcb, hr);
    }
    else
        hr = S_FALSE;

    AssertSz(hr != E_PENDING || !fBindDone, 
        "URLMON reports data pending after binding done!");

    if (!hr && cb && *pcb == 0)
    {
        PerfDbgLog(tagDwnBindData, this,
            "CDwnBindData::ReadFromBind (!hr && !*pcb) == Implied Eof!");
        hr = S_FALSE;
    }
 
    if (hr == E_PENDING)
    {
        hr = S_OK;
    }
    else if (hr == S_FALSE)
    {
        SetEof();

        if (_fBindOnApt)
        {
            ClearInterface(&_u.pstm);
        }

        hr = S_OK;
    }
    else if (hr == S_OK)
    {
        SetPending(FALSE);
    }

    #if DBG==1 || defined(PERFTAGS)
    _cbBind += *pcb;
    #endif

    PerfDbgLog6(tagDwnBindDataIO, this,
        "-CDwnBindData::ReadFromBind (*pcb=%ld,cbBind=%ld,hr=%lX) %c%c%c",
        *pcb, _cbBind, hr, _fPending ? 'P' : ' ', _fEof ? 'E' : ' ',
        fBindDone ? 'D' : ' ');

    RRETURN(hr);
}

void
CDwnBindData::BufferData()
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::BufferData");

    if (_pDwnStm)
    {
        void *  pv;
        ULONG   cbW, cbR;
        HRESULT hr = S_OK;

        for (;;)
        {
            hr = THR(_pDwnStm->WriteBeg(&pv, &cbW));
            if (hr)
                break;

            Assert(cbW > 0);

            hr = THR(ReadFromBind(pv, cbW, &cbR));
            if (hr)
                break;

            Assert(cbR <= cbW);

            _pDwnStm->WriteEnd(cbR);

            if (cbR == 0)
                break;
        }

        if (hr || _fEof)
        {
            _pDwnStm->WriteEof(hr);
        }

        if (hr)
        {
            SignalDone(hr);
        }
    }

    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::BufferData");
}

BOOL
CDwnBindData::IsPending()
{
    return(!_pbPeek && (_pDwnStm ? _pDwnStm->IsPending() : _fPending));
}

BOOL
CDwnBindData::IsEof()
{
    return(!_pbPeek && (_pDwnStm ? _pDwnStm->IsEof() : _fEof));
}

// CDwnBindData (Callback) ----------------------------------------------------

void
CDwnBindData::SetCallback(CDwnLoad * pDwnLoad)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SetCallback");

    g_csDwnBindSig.Enter();

    _wSig     = _wSigAll;
    _pDwnLoad = pDwnLoad;
    _pDwnLoad->SubAddRef();

    g_csDwnBindSig.Leave();

    if (_wSig)
    {
        Signal(0);
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SetCallback");
}

void
CDwnBindData::Disconnect()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Disconnect");

    g_csDwnBindSig.Enter();

    CDwnLoad * pDwnLoad = _pDwnLoad;

    _pDwnLoad = NULL;
    _wSig     = 0;

    g_csDwnBindSig.Leave();

    if (pDwnLoad)
        pDwnLoad->SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::Disconnect");
}

void
CDwnBindData::Signal(WORD wSig)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::Signal (wSig=%04lX)", wSig);

    SubAddRef();

    for (;;)
    {
        CDwnLoad *  pDwnLoad = NULL;
        WORD        wSigCur  = 0;

        g_csDwnBindSig.Enter();

        // DBF_DONE is sent exactly once, even though it may be signalled
        // more than once.  If we are trying to signal it and have already
        // done so, turn off the flag.

        wSig &= ~(_wSigAll & DBF_DONE);

        // Remember all flags signalled since we started in case we disconnect
        // and reconnect to a new client.  That way we can replay all the
        // missed flags.

        _wSigAll |= wSig;

        if (_pDwnLoad)
        {
            // Someone is listening, so lets figure out what to tell the
            // callback.  Notice that if _dwSigTid is not zero, then a
            // different thread is currently in a callback.  In that case,
            // we just let it do the callback again when it returns.

            _wSig |= wSig;

            if (_wSig && !_dwSigTid)
            {
                wSigCur   = _wSig;
                _wSig     = 0;
                _dwSigTid = GetCurrentThreadId();
                pDwnLoad  = _pDwnLoad;
                pDwnLoad->SubAddRef();
            }
        }

        g_csDwnBindSig.Leave();

        if (!pDwnLoad)
            break;

        if (_pDwnStm && (wSigCur & DBF_DATA))
        {
            BufferData();
        }

        pDwnLoad->OnBindCallback(wSigCur);
        pDwnLoad->SubRelease();

        _dwSigTid = 0;
        wSig      = 0;
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::Signal");
}

void
CDwnBindData::SignalRedirect(LPCTSTR pszText, IUnknown *punkBinding)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::SignalRedirect %ls", pszText);

    IWinInetHttpInfo * pwhi = NULL;
    char  achA[16];
    TCHAR achW[16];
    ULONG cch = ARRAY_SIZE(achA);
    HRESULT hr;

    //$ Need protection here.  Redirection can happen more than once.
    //$ This means that GetRedirect() returns a potentially dangerous
    //$ string.

    // Discover the last HTTP request method

    _cstrMethod.Free();
    hr = THR(punkBinding->QueryInterface(IID_IWinInetHttpInfo, (void **)&pwhi));
    if (!hr && pwhi)
    {
        hr = THR(pwhi->QueryInfo(HTTP_QUERY_REQUEST_METHOD, &achA, &cch, NULL, 0));
        if (!hr && cch)
        {
            AnsiToWideTrivial(achA, achW, cch);
            hr = THR(_cstrMethod.Set(achW));
            if (hr)
                goto Cleanup;
        }
    }

    // In case the new URL isn't covered by wininet: clear security flags
    
    _dwSecFlags = IsUrlSecure(pszText) ? SECURITY_FLAG_SECURE : 0;
    
    // Set the redirect url
    
    hr = THR(_cstrRedirect.Set(pszText));

    // Redirection means that our POST request, if any, may have become a GET
    
    if (!_cstrMethod || !_tcsequal(_T("POST"), _cstrMethod))
        SetDwnPost(NULL);

Cleanup:

    ReleaseInterface(pwhi);
    
    if (hr)
        SignalDone(hr);
    else
        Signal(DBF_REDIRECT);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalRedirect");
}

void
CDwnBindData::SignalProgress(DWORD dwPos, DWORD dwMax, DWORD dwStatus)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SignalProgress");

    _DwnProg.dwMax    = dwMax;
    _DwnProg.dwPos    = dwPos;
    _DwnProg.dwStatus = dwStatus;

    Signal(DBF_PROGRESS);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalProgress");
}

void
CDwnBindData::SignalFile(LPCTSTR pszText)
{
    if (    pszText
        &&  *pszText
        &&  (_dwFlags & DWNF_GETFILELOCK)
        &&  (   _uScheme == URL_SCHEME_HTTP
            ||  _uScheme == URL_SCHEME_HTTPS
            ||  _uScheme == URL_SCHEME_FTP
            ||  _uScheme == URL_SCHEME_GOPHER))
    {
        HRESULT hr = THR(_cstrFile.Set(pszText));

        if (hr)
        {
            SignalDone(hr);
        }
    }
}

void
CDwnBindData::SignalHeaders(IUnknown * punkBinding)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SignalHeaders");

    IWinInetHttpInfo * pwhi = NULL;
    IWinInetInfo *pwi = NULL;
    BOOL fSignal = FALSE;
    ULONG cch;
    HRESULT hr = S_OK;

    punkBinding->QueryInterface(IID_IWinInetHttpInfo, (void **)&pwhi);

    if (pwhi)
    {
        CHAR    achA[256];
        TCHAR   achW[256];
 
        if (_dwFlags & DWNF_GETCONTENTTYPE)
        {
            cch = sizeof(achA);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_CONTENT_TYPE, achA, &cch, NULL, 0));

            if (hr == S_OK && cch > 0)
            {
                AnsiToWideTrivial(achA, achW, cch);

                hr = THR(_cstrContentType.Set(achW));
                if (hr)
                    goto Cleanup;

                fSignal = TRUE;
            }

            hr = S_OK;
        }

        if (_dwFlags & DWNF_GETREFRESH)
        {
            cch = sizeof(achA);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_REFRESH, achA, &cch, NULL, 0));

            if (hr == S_OK && cch > 0)
            {
                AnsiToWideTrivial(achA, achW, cch);

                hr = THR(_cstrRefresh.Set(achW));
                if (hr)
                    goto Cleanup;

                fSignal = TRUE;
            }

            hr = S_OK;
        }

        if (_dwFlags & DWNF_GETMODTIME)
        {
            SYSTEMTIME st;
            cch = sizeof(SYSTEMTIME);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_LAST_MODIFIED|HTTP_QUERY_FLAG_SYSTEMTIME,
                        &st, &cch, NULL, 0));

            if (hr == S_OK && cch == sizeof(SYSTEMTIME))
            {
                if (SystemTimeToFileTime(&st, &_ftLastMod))
                {
                    fSignal = TRUE;
                }
            }

            hr = S_OK;
        }
        
        Assert(!(_dwFlags & DWNF_HANDLEECHO) || (_dwFlags & DWNF_GETSTATUSCODE));

        if (_dwFlags & DWNF_GETSTATUSCODE)
        {
            cch = sizeof(_dwStatusCode);

            hr = THR(pwhi->QueryInfo(HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
                        &_dwStatusCode, &cch, NULL, 0));

            if (!hr && (_dwFlags & DWNF_HANDLEECHO) && _dwStatusCode == 449)
            {
                ULONG cb = 0;

                hr = THR(pwhi->QueryInfo(HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_ECHO_HEADERS_CRLF, NULL, &cb, NULL, 0));
                if ((hr && hr != S_FALSE) || !cb)
                    goto NoHeaders;

                Assert(!_pbRawEcho);
                MemFree(_pbRawEcho);
                
                _pbRawEcho = (BYTE *)MemAlloc(Mt(CDwnBindData_pbRawEcho), cb);
                if (!_pbRawEcho)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                _cbRawEcho = cb;

                hr = THR(pwhi->QueryInfo(HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_ECHO_HEADERS_CRLF, _pbRawEcho, &cb, NULL, 0));
                Assert(!hr && cb+1 == _cbRawEcho);
                
            NoHeaders:
                ;
            }

            fSignal = TRUE;
            hr = S_OK;
        }
    }
    
    if (!pwhi)
    {
        punkBinding->QueryInterface(IID_IWinInetInfo, (void **)&pwi);
    }
    else
    {
        pwi = pwhi;
        pwhi = NULL;
    }

    if (_dwFlags & DWNF_GETFLAGS)
    {
        if (pwi)
        {
            DWORD dwFlags;
            cch = sizeof(DWORD);

            hr = THR_NOTRACE(pwi->QueryOption(INTERNET_OPTION_REQUEST_FLAGS, &dwFlags, &cch));
            if (hr == S_OK && cch == sizeof(DWORD))
            {
                _dwReqFlags = dwFlags;
            }

            // BUGBUG: wininet does not remember security for cached files,
            // if it's from the cache, don't ask wininet; just use security-based-on-url.

            cch = sizeof(DWORD);

            hr = THR_NOTRACE(pwi->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, &cch));
            if (hr == S_OK && cch == sizeof(DWORD))
            {
                if ((dwFlags & SECURITY_FLAG_SECURE) || !(_dwReqFlags & INTERNET_REQFLAG_FROM_CACHE))
                    _dwSecFlags = dwFlags;
            }
        }
        
        fSignal = TRUE; // always pick up security flags

        hr = S_OK;
    }

    if ((_dwFlags & DWNF_GETSECCONINFO) && pwi)
    {
        ULONG cb = sizeof(INTERNET_SECURITY_CONNECTION_INFO);
        INTERNET_SECURITY_CONNECTION_INFO isci;
        
        Assert(!_pSecConInfo);
        MemFree(_pSecConInfo);

        isci.dwSize = cb;
        
        hr = THR_NOTRACE(pwi->QueryOption(INTERNET_OPTION_SECURITY_CONNECTION_INFO, &isci, &cb));
        if (!hr && cb == sizeof(INTERNET_SECURITY_CONNECTION_INFO))
        {
            _pSecConInfo = (INTERNET_SECURITY_CONNECTION_INFO *)MemAlloc(Mt(CDwnBindData_pSecConInfo), cb);
            if (!_pSecConInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            memcpy(_pSecConInfo, &isci, cb);
        }

        hr = S_OK;
    }


    if (_cstrFile)
    {
        fSignal = TRUE;

        if (pwi && _uScheme != URL_SCHEME_FILE)
        {
            cch = sizeof(HANDLE);
            IGNORE_HR(pwi->QueryOption(WININETINFO_OPTION_LOCK_HANDLE, &_hLock, &cch));
        }
    }

Cleanup:

    if (pwhi)
        pwhi->Release();

    if (pwi)
        pwi->Release();

    if (hr)
        SignalDone(hr);
    else if (fSignal)
        Signal(DBF_HEADERS);

    PerfDbgLog2(tagDwnBindData, this, "-CDwnBindData::SignalHeaders ReqFlags:%lx SecFlags:%lx", _dwReqFlags, _dwSecFlags);
}

void
CDwnBindData::SignalData()
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::SignalData");

    Signal(DBF_DATA);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalData");
}

void
CDwnBindData::SignalDone(HRESULT hrErr)
{
    PerfDbgLog1(tagDwnBindData, this, "+CDwnBindData::SignalDone (hrErr=%lX)", hrErr);

    Terminate(hrErr);

    Signal(DBF_DONE);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::SignalDone");
}

// CDwnBindData (Misc) --------------------------------------------------------

LPCTSTR
CDwnBindData::GetFileLock(HANDLE * phLock, BOOL *pfPretransform)
{
    // Don't grab the file if there is a MIME filter or we have a transformed HTML file
    // because we don't want to look at the file directly.
    BOOL fTransform = FALSE;
    CDwnDoc *pDwnDoc;
    if ((pDwnDoc = GetDwnDoc()) != NULL)
        fTransform = pDwnDoc->GetDwnTransform();
    *pfPretransform = _fMimeFilter || fTransform;
    if (_cstrFile)  // note that the file will always be returned
    {
        *phLock = _hLock;
        _hLock = NULL;

        return(_cstrFile);
    }

    *phLock = NULL;
    return(NULL);
}

void
CDwnBindData::GiveRawEcho(BYTE **ppb, ULONG *pcb)
{
    Assert(!*ppb);
    
    *ppb = _pbRawEcho;
    *pcb = _cbRawEcho;
    _pbRawEcho = NULL;
    _cbRawEcho = 0;
}

void
CDwnBindData::GiveSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    Assert(!*ppsci);
    
    *ppsci = _pSecConInfo;
    _pSecConInfo = NULL;
}

// CDwnBindData (IBindStatusCallback) -----------------------------------------

STDMETHODIMP
CDwnBindData::OnStartBinding(DWORD grfBSCOption, IBinding *pbinding)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnStartBinding");
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::OnStartBinding");

    Assert(_fBindOnApt && CheckThread());

    _fBindDone = FALSE;

    ReplaceInterface(&_u.pbinding, pbinding);

    if (!_fIsDocBind)
    {
        IGNORE_HR(pbinding->SetPriority(THREAD_PRIORITY_BELOW_NORMAL));
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnStartBinding");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnProgress");

    return(S_OK);
}

STDMETHODIMP
CDwnBindData::OnProgress(ULONG ulPos, ULONG ulMax, ULONG ulCode, LPCWSTR pszText)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnProgress");
    PerfDbgLog4(tagDwnBindData, this, "+CDwnBindData::OnProgress %ld %ld %s \"%ls\"",
        ulPos, ulMax, g_rgpchBindStatus[ulCode], pszText ? pszText : g_Zero.ach);

    Assert(_fBindOnApt && CheckThread());

    switch (ulCode)
    {
        case BINDSTATUS_REDIRECTING:
            SignalRedirect(pszText, _u.pbinding);
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            SignalFile(pszText);
            break;

        case BINDSTATUS_RAWMIMETYPE:
            _pRawMimeInfo = GetMimeInfoFromMimeType(pszText);
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            _pmi = GetMimeInfoFromMimeType(pszText);
            break;

        case BINDSTATUS_LOADINGMIMEHANDLER:
            _fMimeFilter = TRUE;
            break;

        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
        case BINDSTATUS_BEGINDOWNLOADDATA:
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
            SignalProgress(ulPos, ulMax, ulCode);
            break;
    }

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnProgress");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnProgress");
    return(S_OK);
}

STDMETHODIMP
CDwnBindData::OnStopBinding(HRESULT hrReason, LPCWSTR szReason)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnStopBinding");
    PerfDbgLog2(tagDwnBindData, this, "+CDwnBindData::OnStopBinding %lX \"%ls\"",
        hrReason, szReason ? szReason : g_Zero.ach);
    Assert(_fBindOnApt && CheckThread());

    LPWSTR pchError = NULL;

    SubAddRef();

    _fBindDone = TRUE;

    if (hrReason || _fBindAbort)
    {
        CLSID clsid;
        HRESULT hrUrlmon = S_OK;
        
        IGNORE_HR(_u.pbinding->GetBindResult(&clsid, (DWORD *)&hrUrlmon, &pchError, NULL));

        // BUGBUG: URLMON returns a native Win32 error.
        if (SUCCEEDED(hrUrlmon))
            hrUrlmon = HRESULT_FROM_WIN32(hrUrlmon);
            
        SignalDone(hrUrlmon);
    }
    else
    {
        SetPending(FALSE);
        SignalData();
    }

    SubRelease();
    CoTaskMemFree(pchError);

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnStopBinding");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnStopBinding");
    return(S_OK);
}

STDMETHODIMP
CDwnBindData::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC * pformatetc, STGMEDIUM * pstgmed)
{
    PerfDbgLog(tagPerfWatch, this, "+CDwnBindData::OnDataAvailable");
    PerfDbgLog5(tagDwnBindData, this, "+CDwnBindData::OnDataAvailable %c%c%c%c %ld",
        (grfBSCF & BSCF_FIRSTDATANOTIFICATION) ? 'F' : ' ',
        (grfBSCF & BSCF_INTERMEDIATEDATANOTIFICATION) ? 'I' : ' ',
        (grfBSCF & BSCF_LASTDATANOTIFICATION) ? 'L' : ' ',
        (grfBSCF & BSCF_DATAFULLYAVAILABLE) ? 'A' : ' ',
        dwSize);
    Assert(_fBindOnApt && CheckThread());

    HRESULT hr = S_OK;

    if (pstgmed->tymed == TYMED_ISTREAM)
    {
        ReplaceInterface(&_u.pstm, pstgmed->pstm);
        ReplaceInterface(&_u.punkForRel, pstgmed->pUnkForRelease);
    }

    if (grfBSCF & (BSCF_DATAFULLYAVAILABLE|BSCF_LASTDATANOTIFICATION))
    {
        _fFullyAvail = TRUE;

        // Clients assume that they can find out how many bytes are fully
        // available in the callback to SignalHeaders.  Fill it in here if
        // we haven't already.

        if (_DwnProg.dwMax == 0)
            _DwnProg.dwMax = dwSize;
    }

    if (!_fSigHeaders)
    {
        _fSigHeaders = TRUE;
        SignalHeaders(_u.pbinding);
    }

    if (!_fSigMime)
    {
        _fSigMime = TRUE;

        if (_pmi == NULL)
            _pmi = GetMimeInfoFromClipFormat(pformatetc->cfFormat);

        Signal(DBF_MIME);
    }

    SetPending(FALSE);
    SignalData();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnDataAvailable");
    PerfDbgLog(tagPerfWatch, this, "-CDwnBindData::OnDataAvailable");
    RRETURN(hr);
}

// CDwnBindData (IHttpNegotiate) ----------------------------------------------

STDMETHODIMP
CDwnBindData::OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders, LPWSTR * ppszAdditionalRequestHeaders)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::OnResponse");
    Assert(CheckThread());

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::OnResponse (hr=0)");
    return(S_OK);
}

// CDwnBindData (IInternetBindInfo) -------------------------------------------

STDMETHODIMP
CDwnBindData::GetBindInfo(DWORD * pdwBindf, BINDINFO * pbindinfo)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::GetBindInfo");
    Assert(CheckThread());

    HRESULT hr;

    hr = THR(super::GetBindInfo(pdwBindf, pbindinfo));

    if (hr == S_OK)
    {
        if (!_fBindOnApt)
            *pdwBindf |= BINDF_DIRECT_READ;

        if (_dwFlags & DWNF_IGNORESECURITY)
            *pdwBindf |= BINDF_IGNORESECURITYPROBLEM;
    }

    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::GetBindInfo (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnBindData (IInternetProtocolSink) ---------------------------------------

STDMETHODIMP
CDwnBindData::Switch(PROTOCOLDATA * ppd)
{
    PerfDbgLog(tagDwnBindData, this, "+CDwnBindData::Switch");

    HRESULT hr;

    if (!_pDwnDoc || _pDwnDoc->IsDocThread())
    {
        hr = THR(_o.pInetProt->Continue(ppd));
    }
    else
    {
        hr = THR(_pDwnDoc->AddDocThreadCallback(this, ppd));
    }

    PerfDbgLog1(tagDwnBindData, this, "-CDwnBindData::Switch (hr=%lX)", hr);
    RRETURN(hr);
}

STDMETHODIMP
CDwnBindData::ReportProgress(ULONG ulCode, LPCWSTR pszText)
{
    PerfDbgLog2(tagDwnBindData, this, "+CDwnBindData::ReportProgress %s \"%ls\"",
        g_rgpchBindStatus[ulCode], pszText ? pszText : g_Zero.ach);

    SubAddRef();

    switch (ulCode)
    {
        case BINDSTATUS_REDIRECTING:
            SignalRedirect(pszText, _o.pInetProt);
            break;

        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            SignalFile(pszText);
            break;

        case BINDSTATUS_RAWMIMETYPE:
            _pRawMimeInfo = GetMimeInfoFromMimeType(pszText);
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
            _pmi = GetMimeInfoFromMimeType(pszText);
            break;

        case BINDSTATUS_LOADINGMIMEHANDLER:
            _fMimeFilter = TRUE;
            break;

        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
            SignalProgress(0, 0, ulCode);
            break;
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::ReportProgress (hr=0)");
    return(S_OK);
}

STDMETHODIMP
CDwnBindData::ReportData(DWORD grfBSCF, ULONG ulPos, ULONG ulMax)
{
    PerfDbgLog6(tagDwnBindData, this, "+CDwnBindData::ReportData %c%c%c%c ulPos=%ld ulMax=%ld", 
        (grfBSCF & BSCF_FIRSTDATANOTIFICATION) ? 'F' : ' ',
        (grfBSCF & BSCF_INTERMEDIATEDATANOTIFICATION) ? 'I' : ' ',
        (grfBSCF & BSCF_LASTDATANOTIFICATION) ? 'L' : ' ',
        (grfBSCF & BSCF_DATAFULLYAVAILABLE) ? 'A' : ' ',
        ulPos, ulMax);

    SubAddRef();

    if (grfBSCF & (BSCF_DATAFULLYAVAILABLE|BSCF_LASTDATANOTIFICATION))
    {
        _fFullyAvail = TRUE;

        // Clients assume that they can find out how many bytes are fully
        // available in the callback to SignalHeaders.  Fill it in here if
        // we haven't already.

        if (_DwnProg.dwMax == 0)
            _DwnProg.dwMax = ulMax;
    }

    if (!_fSigHeaders)
    {
        _fSigHeaders = TRUE;
        SignalHeaders(_o.pInetProt);
    }

    if (!_fSigData)
    {
        _fSigData = TRUE;

        Assert(_pDwnStm == NULL);

        // If the data is coming from the network, then read it immediately
        // into a buffers in order to release the socket connection as soon
        // as possible.

        if (    !_fFullyAvail
            &&  (_uScheme == URL_SCHEME_HTTP || _uScheme == URL_SCHEME_HTTPS)
            &&  !(_dwFlags & (DWNF_DOWNLOADONLY|DWNF_NOAUTOBUFFER)))
        {
            // No problem if this fails.  We just end up not buffering.

            _pDwnStm = new CDwnStm;
        }
    }

    if (!_fSigMime)
    {
        _fSigMime = TRUE;
        Signal(DBF_MIME);
    }

    SignalProgress(ulPos, ulMax, BINDSTATUS_DOWNLOADINGDATA);

    SetPending(FALSE);
    SignalData();

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::ReportData (hr=0)");
    return(S_OK);
}

STDMETHODIMP
CDwnBindData::ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    PerfDbgLog3(tagDwnBindData, this, "+CDwnBindData::ReportResult (hrErr=%lX,"
        "dwErr=%ld,szRes=%ls)", hrResult, dwError,
        szResult ? szResult : g_Zero.ach);

    SubAddRef();

    _fBindDone = TRUE;

    if (hrResult || _fBindAbort)
    {
        // Mimics urlmon's GetBindResult
        if (dwError)
            hrResult = HRESULT_FROM_WIN32(dwError);

        SignalDone(hrResult);
    }
    else
    {
        SetPending(FALSE);
        SignalData();
    }

    SubRelease();

    PerfDbgLog(tagDwnBindData, this, "-CDwnBindData::ReportResult (hr=0)");
    return(S_OK);
}

// Public Functions -----------------------------------------------------------

HRESULT
NewDwnBindData(DWNLOADINFO * pdli, CDwnBindData ** ppDwnBindData,
    DWORD dwFlagsExtra)
{
    HRESULT hr = S_OK;

    if (pdli->pDwnBindData)
    {
        *ppDwnBindData = pdli->pDwnBindData;
        pdli->pDwnBindData->AddRef();
        return(S_OK);
    }

    PerfDbgLog(tagDwnBindData, NULL, "+NewDwnBindData");

    CDwnBindData * pDwnBindData = new CDwnBindData;

    if (pDwnBindData == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDwnBindData->SetDwnDoc(pdli->pDwnDoc);
    pDwnBindData->SetDwnPost(pdli->pDwnPost);
    pDwnBindData->Bind(pdli, dwFlagsExtra);

    *ppDwnBindData = pDwnBindData;
    pDwnBindData = NULL;

Cleanup:
    if (pDwnBindData)
        pDwnBindData->Release();
    PerfDbgLog1(tagDwnBindData, NULL, "-NewDwnBindData (hr=%lX)", hr);
    RRETURN(hr);
}

// TlsGetInternetSession ------------------------------------------------------

IInternetSession *
TlsGetInternetSession()
{
    IInternetSession * pInetSess = TLS(pInetSess);

    if (pInetSess == NULL)
    {
        IGNORE_HR(CoInternetGetSession(0, &pInetSess, 0));
        TLS(pInetSess) = pInetSess;

        if (pInetSess)
        {
            pInetSess->RegisterNameSpace((IClassFactory *) &g_cfResProtocol, CLSID_ResProtocol, _T("res"), 0, NULL, 0);
            pInetSess->RegisterNameSpace((IClassFactory *) &g_cfAboutProtocol, CLSID_AboutProtocol, _T("about"), 0, NULL, 0);
            pInetSess->RegisterNameSpace((IClassFactory *) &g_cfViewSourceProtocol, CLSID_ViewSourceProtocol, _T("view-source"), 0, NULL, 0);
        }
    }

    return(pInetSess);
}
