
//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996                    **
//*********************************************************************

// the CStubBindStatusCallback implements IBindStatusCallback,
// IHttpNegotiate.  We use it to make a "fake" bind status callback
// object when we have headers and post data we would like to apply
// to a navigation.  We supply this IBindStatusCallback object, and
// the URL moniker asks us for headers and post data and use those in
// the transaction.

#include "priv.h"
#include "sccls.h"
#include "bindcb.h"  

CStubBindStatusCallback::CStubBindStatusCallback(LPCWSTR pwzHeaders,LPCBYTE pPostData,
    DWORD cbPostData, VARIANT_BOOL bOfflineProperty, VARIANT_BOOL bSilentProperty, BOOL bHyperlink,
    DWORD grBindFlags) : _cRef(1)
    // _pszHeaders(NULL), _hszPostData(NULL), _cbPostData(0)  (don't need to zero-init)
{
    // this is a standalone COM object; need to maintain ref count on our
    // DLL to ensure it doesn't unload
    DllAddRef();

    if (pwzHeaders) {
        _pszHeaders = StrDup(pwzHeaders);    // allocate for a permanent copy
    }

    if (pPostData && cbPostData) {
        // make a copy of post data and store it
        _hszPostData = GlobalAlloc(GPTR,cbPostData);
        if (_hszPostData) {
            memcpy((LPVOID) _hszPostData,pPostData,cbPostData);
            _cbPostData = cbPostData;
        }
    }

    _bFrameIsOffline = bOfflineProperty ? TRUE : FALSE;
    _bFrameIsSilent = bSilentProperty ? TRUE : FALSE;
    _bHyperlink = bHyperlink ? TRUE : FALSE;
    _grBindFlags = grBindFlags;
    TraceMsg(TF_SHDLIFE, "ctor CStubBindStatusCallback %x", this);
}

HRESULT CStubBSC_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CStubBindStatusCallback * pbsc = new CStubBindStatusCallback(NULL, NULL, 0, FALSE, FALSE, TRUE, 0);
    if (pbsc) {
    *ppunk = (IBindStatusCallback *)pbsc;
    return S_OK;
    }

    return E_OUTOFMEMORY;
}


CStubBindStatusCallback::~CStubBindStatusCallback()
{
    TraceMsg(TF_SHDLIFE, "dtor CBindStatusCallback %x", this);

    _FreeHeadersAndPostData();  // free any data we still have in this object

    // release ref count on DLL
    DllRelease();
}

STDMETHODIMP CStubBindStatusCallback::QueryInterface(REFIID riid,
    LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IBindStatusCallback)) {
        *ppvObj = SAFECAST(this, IBindStatusCallback*);
    } else if (IsEqualIID(riid, IID_IHttpNegotiate)) {
        *ppvObj = SAFECAST(this, IHttpNegotiate*);
    } else if (IsEqualIID(riid, IID_IMarshal)) {
        *ppvObj = SAFECAST(this, IMarshal*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();  // handing out an interface on ourselves; bump up ref count

    return S_OK;
}

STDMETHODIMP_(ULONG) CStubBindStatusCallback::AddRef(void)
{
    _cRef++;
    TraceMsg(TF_SHDREF, "CStubBindStatusCallback(%x)::AddRef called, new _cRef=%d", this, _cRef);

    return _cRef;
}

STDMETHODIMP_(ULONG) CStubBindStatusCallback::Release(void)
{
    _cRef--;
    TraceMsg(TF_SHDREF, "CStubBindStatusCallback(%x)::Release called, new _cRef=%d", this, _cRef);

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

//
//  Implementation of IBindStatusCallback begins here
//

// implements IBindStatusCallback::OnStartBinding
STDMETHODIMP CStubBindStatusCallback::OnStartBinding(DWORD grfBSCOption,IBinding *pib)
{
    return S_OK;  // we don't care
}

// implements IBindStatusCallback::GetPriority
STDMETHODIMP CStubBindStatusCallback::GetPriority(LONG *pnPriority)
{
    *pnPriority = NORMAL_PRIORITY_CLASS;

    return S_OK;
}

// implements IBindStatusCallback::OnLowResource
STDMETHODIMP CStubBindStatusCallback::OnLowResource(DWORD reserved)
{
    return S_OK;  // we don't care
}

// implements IBindStatusCallback::OnProgress
STDMETHODIMP CStubBindStatusCallback::OnProgress(ULONG ulProgress,ULONG ulProgressMax,
        ULONG ulStatusCode,LPCWSTR szStatusText)
{
    return S_OK;  // we don't care
}

// implements IBindStatusCallback::OnStopBinding
STDMETHODIMP CStubBindStatusCallback::OnStopBinding(HRESULT hresult,LPCWSTR szError)
{
    return S_OK;  // we don't care
}

// implements IBindStatusCallback::GetBindInfo
STDMETHODIMP CStubBindStatusCallback::GetBindInfo(DWORD *grfBINDF,BINDINFO *pbindinfo)
{
    HRESULT hr;

    if ( !grfBINDF || !pbindinfo || !pbindinfo->cbSize )
        return E_INVALIDARG;

    // call helper function to do fill in BINDINFO struct with appropriate
    // binding data
    *grfBINDF = _grBindFlags;
    hr = BuildBindInfo(grfBINDF,pbindinfo,_hszPostData,_cbPostData, _bFrameIsOffline, _bFrameIsSilent, _bHyperlink,
        (IBindStatusCallback *) this);

    return hr;
}

// implements IBindStatusCallback::OnDataAvailable
STDMETHODIMP CStubBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    ASSERT(FALSE);  // should never get called here!

    return S_OK;
}

STDMETHODIMP CStubBindStatusCallback::OnObjectAvailable(REFIID riid,IUnknown *punk)
{
    return S_OK;
}

//
//  Implementation of IHttpNegotiate begins here
//

// implements IHttpNegotiate::BeginningTransaction
STDMETHODIMP CStubBindStatusCallback::BeginningTransaction(LPCWSTR szURL, LPCWSTR szHeaders,
    DWORD dwReserved, LPWSTR *ppwzAdditionalHeaders)
{
    // call helper function
    return BuildAdditionalHeaders(_pszHeaders,(LPCWSTR *) ppwzAdditionalHeaders);
}

// implements IHttpNegotiate::OnResponse
STDMETHODIMP CStubBindStatusCallback::OnResponse(DWORD dwResponseCode, LPCWSTR szResponseHeaders,
    LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{

    return S_OK;
}

//
//  Additional methods on our class begin here
//

STDMETHODIMP CStubBindStatusCallback::_FreeHeadersAndPostData()
{
    if (_pszHeaders) {
        LocalFree((HGLOBAL) _pszHeaders);
        _pszHeaders = NULL;
    }

    if (_hszPostData) {
        GlobalFree(_hszPostData);
        _hszPostData = NULL;
        _cbPostData = 0;
    }
    
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Method:     CStubBindStatusCallback::_CanMarshalIID
//
//  Synopsis:   Checks whether this object supports marshalling this IID.
//
//  Arguments:  [riid] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL CStubBindStatusCallback::_CanMarshalIID(REFIID riid)
{
    // keep this in sync with the QueryInterface
    return (BOOL) (IsEqualIID(riid,IID_IBindStatusCallback) || 
                   IsEqualIID(riid,IID_IUnknown) ||
                   IsEqualIID(riid, IID_IHttpNegotiate));
}

//+---------------------------------------------------------------------------
//
//  Method:     CStubBindStatusCallback::_ValidateMarshalParams
//
//  Synopsis:   Validates the standard set parameters that are passed into most
//              of the IMarshal methods
//
//  Arguments:  [riid] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CStubBindStatusCallback::_ValidateMarshalParams(REFIID riid,void *pvInterface,
                    DWORD dwDestContext,void *pvDestContext,DWORD mshlflags)
{

    HRESULT hr = NOERROR;
 
    if (_CanMarshalIID(riid))
    {
        // BUGBUG 10/02/96 chrisfra: ask johannp, should we be supporting future contexts
        // via CoGetStandardMarshal?

        ASSERT((dwDestContext == MSHCTX_INPROC || dwDestContext == MSHCTX_LOCAL || dwDestContext == MSHCTX_NOSHAREDMEM));
        ASSERT((mshlflags == MSHLFLAGS_NORMAL || mshlflags == MSHLFLAGS_TABLESTRONG));

        if (   (dwDestContext != MSHCTX_INPROC && dwDestContext != MSHCTX_LOCAL && dwDestContext != MSHCTX_NOSHAREDMEM)
            || (mshlflags != MSHLFLAGS_NORMAL && mshlflags != MSHLFLAGS_TABLESTRONG))
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// IMarshal methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CStubBindStatusCallback::GetUnmarshalClass
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//              [pCid] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CStubBindStatusCallback::GetUnmarshalClass(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid)
{
    HRESULT hr;

    hr = _ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
    if (hr == NOERROR)
    {
        *pCid = (CLSID) CLSID_CStubBindStatusCallback;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CStubBindStatusCallback::GetMarshalSizeMax
//
//  Synopsis:
//
//  Arguments:  [void] --
//              [pvInterface] --
//              [dwDestContext] --
//              [pvDestContext] --
//              [mshlflags] --
//              [pSize] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CStubBindStatusCallback::GetMarshalSizeMax(REFIID riid,void *pvInterface,
        DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize)
{
    HRESULT hr;

    if (pSize == NULL)
    {
        hr = E_INVALIDARG;

    }
    else
    {

        hr = _ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
        if (hr == NOERROR)
        {

            // size of fBSCBFlags, grBindFlags, postdata, headers.
            *pSize = (sizeof(DWORD) + 3 * sizeof(DWORD)) + _cbPostData ;
            if (_pszHeaders)
                *pSize += lstrlen(_pszHeaders) + 1;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CStubBindStatusCallback::MarshalInterface
//
//  Synopsis:
//
//  Arguments:  [REFIID] --
//              [riid] --
//              [DWORD] --
//              [void] --
//              [DWORD] --
//              [mshlflags] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CStubBindStatusCallback::MarshalInterface(IStream *pistm,REFIID riid,
                                void *pvInterface,DWORD dwDestContext,
                                void *pvDestContext,DWORD mshlflags)
{
    HRESULT hr;
    DWORD cbLen;
    DWORD fBSCBFlags;

    hr = _ValidateMarshalParams(riid, pvInterface, dwDestContext,pvDestContext, mshlflags);
    if (hr != NOERROR) goto exitPoint;

    //  Write _grBindFlags
    hr = pistm->Write(&_grBindFlags, sizeof(DWORD), NULL);
    if (hr != NOERROR) goto exitPoint;

    //  Write fBSCBFlags

    fBSCBFlags = (_bFrameIsOffline ? 1 : 0) + (_bFrameIsSilent ? 2 : 0) ;

    hr = pistm->Write(&fBSCBFlags, sizeof(DWORD), NULL);
    if (hr != NOERROR) goto exitPoint;

    //  Write headers

    cbLen = (_pszHeaders ? (lstrlen(_pszHeaders) + 1) * sizeof(TCHAR) : 0);
    hr = pistm->Write(&cbLen, sizeof(DWORD), NULL);
    if (hr != NOERROR) goto exitPoint;
    if (cbLen != 0)
    {
        hr = pistm->Write(_pszHeaders, cbLen, NULL);
        if (hr != NOERROR) goto exitPoint;
    }

    //  Write PostData

    hr = pistm->Write(&_cbPostData, sizeof(DWORD), NULL);
    if (hr != NOERROR) goto exitPoint;
    if (_cbPostData != 0)
    {
        hr = pistm->Write(_hszPostData, _cbPostData, NULL);
        if (hr != NOERROR) goto exitPoint;
    }

exitPoint:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CStubBindStatusCallback::UnmarshalInterface
//
//  Synopsis:   Unmarshals an Urlmon interface out of a stream
//
//  Arguments:  [REFIID] --
//              [void] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CStubBindStatusCallback::UnmarshalInterface(IStream *pistm,REFIID riid,void ** ppvObj)
{
    HRESULT hr = NOERROR;
    DWORD fBSCBFlags;

    if (ppvObj == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (! _CanMarshalIID(riid))
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    else
    {
        *ppvObj = NULL;
        DWORD cbLen;

        //  Free old values, if any

        _FreeHeadersAndPostData();

        //  Read _grBindFlags

        hr = pistm->Read(&fBSCBFlags, sizeof(DWORD), NULL);
        if (hr != NOERROR) goto exitPoint;
        _grBindFlags = fBSCBFlags;

        //  Read m_fBSCBFlags

        hr = pistm->Read(&fBSCBFlags, sizeof(DWORD), NULL);
        if (hr != NOERROR) goto exitPoint;
        _bFrameIsOffline = fBSCBFlags & 1 ? 1:0;
        _bFrameIsSilent = fBSCBFlags & 2 ? 1:0;

        //  Read headers

        hr = pistm->Read(&cbLen, sizeof(DWORD), NULL);
        if (hr != NOERROR) goto exitPoint;
        if (cbLen != 0)
        {
            LPTSTR pszData;

            pszData = (LPTSTR) LocalAlloc(LPTR, cbLen);
            if (pszData == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto exitPoint;
            }
            hr = pistm->Read(pszData, cbLen, 0);
            if (hr != NOERROR)
            {
                LocalFree(pszData);
                goto exitPoint;
            }
            _pszHeaders = pszData;
        }

        //  Read PostData

        hr = pistm->Read(&cbLen, sizeof(DWORD), NULL);
        if (hr != NOERROR) goto exitPoint;
        if (cbLen != 0)
        {
            HGLOBAL hszData;

            // POST data must be HGLOBAL because the StgMedium requires it
            // see bindcb.cpp ::GetBindInfo()
            // This will be freed by the Moniker when it's done with it.


            hszData = GlobalAlloc(GPTR,cbLen);
            if (hszData == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto exitPoint;
            }
            hr = pistm->Read(hszData, cbLen, 0);
            if (hr != NOERROR)
            {
                GlobalFree(hszData);
                goto exitPoint;
            }
            _hszPostData = hszData;
            _cbPostData = cbLen;
        }


        // call QI to get the requested interface
        hr = QueryInterface(riid, ppvObj);
    }
exitPoint:
    return hr;
}

STDMETHODIMP CStubBindStatusCallback::ReleaseMarshalData(IStream *pStm)
{
    //  BUGBUG:  10/02/96 chrisfra: ask Johannp if this should be seeking past EOD
    return NOERROR;
}

STDMETHODIMP CStubBindStatusCallback::DisconnectObject(DWORD dwReserved)
{
    return NOERROR;
}


//
//  Global helper functions
//

/*******************************************************************

    NAME:       fOnProxy

    SYNOPSIS:   returns TRUE if we are have proxy enabled


********************************************************************/
BOOL fOnProxy()
{
    // are we on a proxy?
    BOOL fRetOnProxy = FALSE;
    DWORD dwValue;
    DWORD dwSize = SIZEOF(dwValue);
    BOOL  fDefault = FALSE;

    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
        TEXT("ProxyEnable"), NULL, (LPBYTE)&dwValue, &dwSize, FALSE, (LPVOID) &fDefault, SIZEOF(fDefault));
    fRetOnProxy = dwValue;

    return fRetOnProxy;
}

/*******************************************************************

    NAME:       SetBindfFlagsBasedOnAmbient

    SYNOPSIS:   sets BINDF_OFFLINE if ambient offline and
                not-connected and sets BINDF_GETFROMCACHE_IF_NET_FAIL
                if ambient offline and connected


********************************************************************/
void SetBindfFlagsBasedOnAmbient(BOOL fAmbientOffline, DWORD *grfBINDF)
{
    if(fAmbientOffline)
    {
        DWORD dwConnectedStateFlags;
        
        // We want to set the offline bindf flag if the ambient flag is set
        // and we're currently not connected.
        //
        // If either of these conditions is not true, clear the offline flag
        // as mshtml may have previously set it.
        if(FALSE == InternetGetConnectedState(&dwConnectedStateFlags, 0))
        {
            *grfBINDF |= BINDF_OFFLINEOPERATION;
            *grfBINDF &= ~BINDF_GETFROMCACHE_IF_NET_FAIL;
        }
        else
        {
            *grfBINDF |= BINDF_GETFROMCACHE_IF_NET_FAIL;
            *grfBINDF &= ~BINDF_OFFLINEOPERATION;   
        }
    }
    else
    {
        *grfBINDF &= ~BINDF_OFFLINEOPERATION;
    }
}


/*******************************************************************

    NAME:       BuildBindInfo

    SYNOPSIS:   Fills out a BINDINFO structure for a URL moniker

    NOTES:      The point of having this in a global helper function is
                so we don't have to duplicate this code in multiple
                implementations of IBindStatusCallback.

                The caller must pass in an IUnknown to be used as the
                pUnkForRelease in the STGMEDIUM for post data.  If there
                is post data, this function will AddRef the passed-in
                IUnknown and return it in the STGMEDIUM structure.  The
                caller (or someone else, if the caller hands it off) must
                ultimately call Release on pbindinfo->stgmediumData.pUnkForRelease.

********************************************************************/
HRESULT BuildBindInfo(DWORD *grfBINDF,BINDINFO *pbindinfo,HGLOBAL hszPostData,
    DWORD cbPostData, BOOL bFrameIsOffline, BOOL bFrameIsSilent, BOOL bHyperlink, LPUNKNOWN pUnkForRelease)
{
    DWORD dwConnectedStateFlags = 0;
    ASSERT(grfBINDF);
    ASSERT(pbindinfo);
    ASSERT(pUnkForRelease);

    HRESULT hres=S_OK;

    if ( !grfBINDF || !pbindinfo || !pbindinfo->cbSize )
        return E_INVALIDARG;

    // clear BINDINFO except cbSize
    ASSERT(sizeof(*pbindinfo) == pbindinfo->cbSize);
    DWORD cbSize = pbindinfo->cbSize;
    ZeroMemory( pbindinfo, cbSize );
    pbindinfo->cbSize = cbSize;

    *grfBINDF |= BINDF_ASYNCHRONOUS;

    if (bHyperlink)
        *grfBINDF |= BINDF_HYPERLINK;

   
    SetBindfFlagsBasedOnAmbient(bFrameIsOffline, grfBINDF);
    
    if(bFrameIsSilent)
        *grfBINDF |= BINDF_NO_UI;   

    // default method is GET.  Valid ones are _GET, _PUT, _POST, _CUSTOM
    pbindinfo->dwBindVerb = BINDVERB_GET;

    // get IE-wide UTF-8 policy by calling urlmon
    DWORD dwIE = URL_ENCODING_NONE;
    DWORD dwOutLen = sizeof(DWORD);
    if( S_OK == UrlMkGetSessionOption(
        URLMON_OPTION_URL_ENCODING,
        &dwIE, 
        sizeof(DWORD),
        &dwOutLen,
        NULL )  )
    {
        if( dwIE == URL_ENCODING_ENABLE_UTF8 )
        {
            pbindinfo->dwOptions |= BINDINFO_OPTIONS_ENABLE_UTF8;
        }
        else 
        if( dwIE == URL_ENCODING_DISABLE_UTF8 )
        {
            pbindinfo->dwOptions |= BINDINFO_OPTIONS_DISABLE_UTF8;
        }
    }

    // if we have postdata set, then we assume this is a POST verb

    if (hszPostData)
    {
        pbindinfo->dwBindVerb = BINDVERB_POST;
        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.hGlobal = hszPostData;
        //  this count should *NOT* include the terminating NULL
        pbindinfo->cbstgmedData = cbPostData;
        pbindinfo->stgmedData.pUnkForRelease = pUnkForRelease;
        // addref on the IUnknown that's holding onto this data so
        // it knows to stick around; caller must call Release
        // on the pUnkForRelease when done.
        pUnkForRelease->AddRef(); 

        // We will still cache the response, but we do not want to
        // read from cache for a POST transaction.  This will keep us
        // from reading from the cache.
        *grfBINDF |= BINDF_GETNEWESTVERSION | BINDF_CONTAINER_NOWRITECACHE;
    } else {
        ASSERT(pbindinfo->stgmedData.tymed == TYMED_NULL);
        ASSERT(pbindinfo->stgmedData.hGlobal == NULL);
        ASSERT(pbindinfo->stgmedData.pUnkForRelease == NULL);
    }

    return hres;
}

#define HDR_LANGUAGE     TEXT("Accept-Language:")
#define CRLF             TEXT("\x0D\x0A")
#define HDR_LANGUAGE_CRLF     TEXT("Accept-Language: %s\x0D\x0A")

/*******************************************************************

    NAME:       BuildAdditionalHeaders

    SYNOPSIS:   Builds HTTP headers to be given to URL moniker

    ENTRY:      pszOurExtraHeaders - headers that we explicitly want to add
                *ppwzCombinedHeadersOut - on exit, filled in with
                   buffer of default headers plus pszOurExtraHeaders.

    NOTES:      The point of having this in a global helper function is
                so we don't have to duplicate this code in multiple
                implementations of IBindStatusCallback.

                The caller must free *ppwzCombinedHeaders by passing
                to URLMON, or calling OleFree

********************************************************************/
HRESULT BuildAdditionalHeaders(LPCTSTR pszOurExtraHeaders,LPCWSTR * ppwzCombinedHeadersOut)
{

    TCHAR   szLanguage[80];   // BUGBUG: what limit on language?
    DWORD   dwLanguage = ARRAYSIZE(szLanguage);
    static const TCHAR hdr_language[] = HDR_LANGUAGE_CRLF;
    TCHAR szHeader[ARRAYSIZE(hdr_language) + ARRAYSIZE(szLanguage)]; // NOTE format string length > wnsprintf length
    int cchHeaders = 0;
    int cchAddedHeaders = 1;  // implied '\0'
    HRESULT hres = NOERROR;

    if (!ppwzCombinedHeadersOut)
        return E_FAIL;

    *ppwzCombinedHeadersOut = NULL;

    // If there is no language in the registry, *WE DO NOT SEND THIS HEADER*

    // S_OK means szLanguage filled in and returned
    // S_FALSE means call succeeded, but there was no language set
    // E_* is an error
    // We treat S_FALSE and E_* the same, no language header sent.
    if (GetAcceptLanguages(szLanguage, &dwLanguage) == S_OK)
    {
        wnsprintf(szHeader, ARRAYSIZE(szHeader), hdr_language, szLanguage);
        cchHeaders = lstrlen(szHeader) + 1;
    }

    if (pszOurExtraHeaders)
    {
        cchAddedHeaders = lstrlen(pszOurExtraHeaders) + 1;
    }

    // If we have headers we added or were sent in, we need to Wide 'em and
    // give 'em back
    if (cchAddedHeaders > 1 || cchHeaders > 0)
    {
        WCHAR *pwzHeadersForUrlmon = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR) * (cchHeaders  + cchAddedHeaders - 1));
        if (pwzHeadersForUrlmon)
        {
            if (cchHeaders)
            {
                StrCpyN(pwzHeadersForUrlmon, szHeader, cchHeaders);
            }
            if (pszOurExtraHeaders)
            {
                if (cchHeaders)
                {
                    StrCpyN(pwzHeadersForUrlmon + cchHeaders - 1,
                            pszOurExtraHeaders, cchAddedHeaders);
                }
                else
                {
                    StrCpyN(pwzHeadersForUrlmon, pszOurExtraHeaders, cchAddedHeaders - 1);
                }
            }
            if (cchHeaders || pszOurExtraHeaders)
                *ppwzCombinedHeadersOut = pwzHeadersForUrlmon;
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else
        hres = pszOurExtraHeaders == NULL ? S_OK : E_FAIL;

    return hres;
}

/*******************************************************************

    NAME:       GetHeadersAndPostData

    SYNOPSIS:   Gets HTTP headers and post data from an IBindStatusCallback

    ENTRY:      IBindStatusCallback - object to ask for headers and post data
                ppszHeaders - on exit, filled in with pointer to headers,
                    or NULL if none
                pstgPostData - pointer to a STGMEDIUM to be filled in with post
                    data, if any.


    NOTES:      The caller is responsible for:
                    - calling LocalFree on *ppszHeaders when done with them
                    - calling ReleaseStgMedium on pstgPostData when done
                      with it

********************************************************************/
HRESULT GetHeadersAndPostData(IBindStatusCallback * pBindStatusCallback,
    LPTSTR * ppszHeaders, STGMEDIUM * pstgPostData, DWORD * pdwPostData, BOOL* pfIsPost)
{
    HRESULT hr = S_OK;

    ASSERT(pBindStatusCallback);
    ASSERT(ppszHeaders);
    ASSERT(pstgPostData);
    ASSERT(pdwPostData);

    // clear the out parameters
    *ppszHeaders = NULL;

    DWORD grfBINDF;
    IHttpNegotiate *pinegotiate;
    BINDINFO binfo;
    binfo.cbSize = sizeof(binfo);
    ZeroMemory(pstgPostData,sizeof(*pstgPostData));
    *pdwPostData = 0;

    hr=pBindStatusCallback->GetBindInfo(&grfBINDF, &binfo);

    if (SUCCEEDED(hr)) {
        // copy STGMEDIUM with post data to caller
        *pstgPostData = binfo.stgmedData;
        *pdwPostData = binfo.cbstgmedData;

        // clear these so ReleaseBindInfo won't wack it since we are giving it to the caller
        ZeroMemory(&binfo.stgmedData, sizeof(STGMEDIUM));
        binfo.cbstgmedData = 0;

        if (pfIsPost)
        {
            if(*pdwPostData)
                *pfIsPost = TRUE;
            else
                *pfIsPost = FALSE;
        }
 
        hr = pBindStatusCallback->QueryInterface(IID_IHttpNegotiate, (LPVOID *)&pinegotiate);
        if (SUCCEEDED(hr))
        {
            WCHAR *pwzAdditionalHeaders = NULL;
            WCHAR wzNull[1];

            wzNull[0] = 0;
            hr=pinegotiate->BeginningTransaction(wzNull, wzNull, 0, &pwzAdditionalHeaders);
            if (SUCCEEDED(hr) && pwzAdditionalHeaders)
            {
                DWORD cchHeaders;

                cchHeaders = lstrlen(pwzAdditionalHeaders) + 1;

                //  they should *NEVER* be specifying more than a few hundred
                //  bytes or they're going to fail with a number of HTTP servers!

                LPTSTR pszHeaders = (TCHAR *)LocalAlloc(LPTR, cchHeaders*sizeof(TCHAR));
                if (pszHeaders)
                {
                    LPTSTR pszNext;
                    LPTSTR pszLine;
                    LPTSTR pszLast;

                    StrCpyN(pszHeaders, pwzAdditionalHeaders, cchHeaders);
                    pszLine = pszHeaders;
                    pszLast = pszHeaders + lstrlen(pszHeaders);
                    while (pszLine < pszLast)
                    {
                        pszNext = StrStrI(pszLine, CRLF);
                        if (pszNext == NULL)
                        {
                            // All Headers must be terminated in CRLF!
                            pszLine[0] = '\0';
                            break;
                        }
                        pszNext += 2;
                        if (!StrCmpNI(pszLine,HDR_LANGUAGE,16))
                        {
                            MoveMemory(pszLine, pszNext, ((pszLast - pszNext) + 1)*sizeof(TCHAR));
                            break;
                        }
                        pszLine = pszNext;
                    }
                    // Don't include empty headers
                    if (pszHeaders[0] == '\0')
                    {
                        LocalFree(pszHeaders);
                        pszHeaders = NULL;
                    }
                }
                OleFree(pwzAdditionalHeaders);
                *ppszHeaders = pszHeaders;
            }
            pinegotiate->Release();
        }

        ReleaseBindInfo(&binfo);
    }

    return hr;
}

/*******************************************************************

    NAME:       GetTopLevelBindStatusCallback

    ENTRY:      psp - IServiceProvider of ShellBrowser container to query
                ppBindStatusCallback - if successful, filled in with
                   an IBindStatusCallback on exit

    SYNOPSIS:   Gets the IBindStatusCallback associated with this top
                level browser.  This works from within nested frames.

********************************************************************/
HRESULT GetTopLevelBindStatusCallback(IServiceProvider * psp,
    IBindStatusCallback ** ppBindStatusCallback)
{
    HRESULT hr;

    IHlinkFrame *phf;
    hr = psp->QueryService(SID_SHlinkFrame, IID_IHlinkFrame, (LPVOID*)&phf);
    if (SUCCEEDED(hr)) {
        hr = IUnknown_QueryService(phf, IID_IHlinkFrame, IID_IBindStatusCallback, (LPVOID*)ppBindStatusCallback);
        phf->Release();
    }

    return hr;
}

/*******************************************************************

    NAME:       GetTopLevelPendingBindStatusCallback

    ENTRY:      psp - IServiceProvider of ShellBrowser container to query
                ppBindStatusCallback - if successful, filled in with
                   an IBindStatusCallback on exit

    SYNOPSIS:   Gets the IBindStatusCallback associated with this top
                level browser.  This works from within nested frames.

********************************************************************/
HRESULT GetTopLevelPendingBindStatusCallback(IServiceProvider * psp,
    IBindStatusCallback ** ppBindStatusCallback)
{
    HRESULT hr;

    IHlinkFrame *phf;
    hr = psp->QueryService(SID_SHlinkFrame, IID_IHlinkFrame, (LPVOID*)&phf);
    if (SUCCEEDED(hr)) {
        hr = IUnknown_QueryService(phf, SID_PendingBindStatusCallback, IID_IBindStatusCallback, (LPVOID*)ppBindStatusCallback);
        phf->Release();
    }

    return hr;
}

// Global helper function to create a CStubBindStatusCallback
HRESULT CStubBindStatusCallback_Create(LPCWSTR pwzHeaders, LPCBYTE pPostData,
    DWORD cbPostData, VARIANT_BOOL bFrameIsOffline, VARIANT_BOOL bFrameIsSilent, BOOL bHyperlink, 
    DWORD grBindFlags,
    CStubBindStatusCallback ** ppBindStatusCallback)
{
    ASSERT(ppBindStatusCallback);

    *ppBindStatusCallback = new CStubBindStatusCallback(pwzHeaders,pPostData,
        cbPostData, bFrameIsOffline, bFrameIsSilent, bHyperlink, grBindFlags);

    return (*ppBindStatusCallback ? S_OK : E_OUTOFMEMORY);
}
