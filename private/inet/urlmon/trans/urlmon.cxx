//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       urlmon.cxx
//
//  Contents:   contains URL moniker implementation
//
//  Classes:
//
//  Functions:
//
//  History:    12-11-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>
#ifndef unix
#include "..\iapp\curl.hxx"
#else
#include "../iapp/curl.hxx"
#endif /* unix */
#include "urlmk.hxx"

PerfDbgTag(tagCUrlMon, "Urlmon", "Log CUrlMon", DEB_URLMON);

BOOL IsOInetProtocol(IBindCtx *pbc, LPCWSTR wzProtocol);

// prototypes of helper functions (used by RelativePathTo)
CUrlMon *CreateEmptyPathUrlMon();
HRESULT HrCreateCUrlFromUrlStr(LPCWSTR pwzUrl, BOOL fParseUrl, CUrl **ppUrl);
HRESULT HrCreateCUrlFromUrlMon(LPMONIKER pmkUrl, BOOL fParseUrl, CUrl **ppUrl);
HRESULT HrGetRelativePath(LPSTR lpszBase, LPSTR lpszOther, DWORD dwProto, LPSTR lpszHost,LPSTR lpszRelPath);

// #define URLMON_RELPATHTO_PARSE_QUERY_PARAMS
#ifdef URLMON_RELPATHTO_PARSE_QUERY_PARAMS
// These are helper routines used by RelativePathTo to deal with the Query
// and Params sub-strings of a URL, according to rfc 1808.
// These routines are not enabled because ComposeWith does not deal with
// these sub-strings in any special way, and we want RelativePathTo to be
// compatible with ComposeWith.
void ParseUrlQuery(LPSTR pszURL, LPSTR *ppszQuery);
void ParseUrlParams(LPSTR pszURL, LPSTR *ppszParams);
void AddParamsAndQueryToRelPath(LPSTR szRelPath,
    LPSTR pszParamsBase, LPSTR pszParamsOther,
    LPSTR pszQueryBase, LPSTR pszQueryOther);
#endif // URLMON_RELPATHTO_PARSE_QUERY_PARAMS


// Macros for Double-Byte Character Support (DBCS)
#if 1
    // Beware of double evaluation
    #define IncLpch(sz)          ((sz)=CharNext((sz)))
    #define DecLpch(szStart, sz) ((sz)=CharPrev ((szStart),(sz)))
#else
    #define IncLpch(sz)         (++(sz))
    #define DecLpch(szStart,sz) (--(sz))
#endif


CUrlMon::CUrlMon(LPWSTR pszUrl) : _CRefs()
{
    _pwzUrl = pszUrl;
    DllAddRef();
}

CUrlMon::~CUrlMon()
{

    if (_pwzUrl)
    {
        delete [] _pwzUrl;
    }
    DllRelease();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    HRESULT hr = NOERROR;
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::QueryInterface");

    if (   riid == IID_IUnknown
        || riid == IID_IMoniker
        || riid == IID_IAsyncMoniker
        || riid == IID_IPersist
        || riid == IID_IPersistStream)
    {
        *ppvObj = this;
    }
    else if (riid == IID_IROTData)
    {
        *ppvObj = (void*)(IROTData *) this;
    }
    else if (riid == IID_IMarshal)
    {
        *ppvObj = (void*) (IMarshal *) this;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }
    if (hr == NOERROR)
    {
        AddRef();
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::QueryInterface (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlMon::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlMon::AddRef(void)
{
    LONG lRet = ++_CRefs;

    PerfDbgLog1(tagCUrlMon, this, "CUrlMon::AddRef (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlMon::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlMon::Release(void)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Release");

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::Release (cRefs:%ld)", lRet);
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetClassID
//
//  Synopsis:
//
//  Arguments:  [pClassID] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetClassID(CLSID *pClassID)
{
    VDATEPTRIN(pClassID, CLSID);
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::GetClassID");

    *pClassID = CLSID_StdURLMoniker;
    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::GetClassID (hr:%lx)", NOERROR);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::IsDirty
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::IsDirty()
{
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::Load
//
//  Synopsis:
//
//  Arguments:  [pistm] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::Load(IStream *pistm)
{
    VDATEIFACE(pistm);
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Load");
    HRESULT hr = NOERROR;
    ULONG cbLen = 0;

    // Read in the new URL from the stream
    hr = pistm->Read(&cbLen, sizeof(ULONG), NULL);

    if ((hr == NOERROR) && (cbLen > 0))
    {
        LPWSTR wszUrlLocal = new WCHAR [cbLen / sizeof(WCHAR)];
        DbgLog2(tagCUrlMon, this, "=== CUrlMon::Load (cbBytes:%ld, cbLen:%ld)", cbLen,  cbLen / sizeof(WCHAR));

        if (wszUrlLocal)
        {
            hr = pistm->Read(wszUrlLocal, cbLen, NULL);
            DbgLog2(tagCUrlMon, this, "=== CUrlMon::Load (cbLen:%ld, hr:%lx)", cbLen, hr);

            if (hr == NOERROR)
            {
                // If we already had a URL, delete it
                if (_pwzUrl)
                {
                    delete [] _pwzUrl;
                }

                _pwzUrl = wszUrlLocal;
            }
            else
            {
                delete [] wszUrlLocal;
            }
        }
    }

    PerfDbgLog2(tagCUrlMon, this, "-CUrlMon::Load (hr:%lx, szUrl:%ws)", hr, _pwzUrl?_pwzUrl:L"");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::Save
//
//  Synopsis:
//
//  Arguments:  [pistm] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::Save(IStream *pistm, BOOL fClearDirty)
{
    VDATEIFACE(pistm);
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Save");
    UrlMkAssert((_pwzUrl));
    HRESULT hr = E_FAIL;

    if (_pwzUrl)
    {
        ULONG cbLen = (wcslen(_pwzUrl) + 1) * sizeof(WCHAR);
        DbgLog2(tagCUrlMon, this, "=== CUrlMon::Save (cbLen:%ld, cbLen:%ld)", cbLen, cbLen / sizeof(WCHAR));

        // Write the URL to the stream
        hr = pistm->Write(&cbLen, sizeof(ULONG), NULL);
        if (hr == NOERROR)
        {
            hr = pistm->Write(_pwzUrl, cbLen, NULL);
        }
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::Save (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetSizeMax
//
//  Synopsis:
//
//  Arguments:  [pcbSize] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    VDATEPTROUT (pcbSize, ULONG);
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::GetSizeMax");

    UrlMkAssert((_pwzUrl));
    UrlMkAssert((pcbSize));

    // length of url
    ULISet32(*pcbSize, ((wcslen(_pwzUrl) + 1) * sizeof(WCHAR)) + sizeof(ULONG));

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::GetSizeMax (hr:%lx)", NOERROR);
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::BindToObject
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [riidRes] --
//              [ppvRes] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::BindToObject(IBindCtx *pbc, IMoniker *pmkToLeft,  REFIID riidRes, void **ppvRes)
{
    VDATEPTROUT(ppvRes, LPVOID);
    VDATEIFACE(pbc);
    PerfDbgLog2(tagCUrlMon, this, "+CUrlMon::BindToObject (IBindCtx:%lx, pmkToLeft:%lx)", pbc, pmkToLeft);

    *ppvRes = NULL;
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }

    HRESULT     hr;
    CLSID       clsid;
    BIND_OPTS   bindopts;
    CBinding    *pCBdg = NULL;
    CBSC        *pBSC = NULL;
    WCHAR       wzURL[MAX_URL_SIZE + 1];

    *ppvRes = NULL;

    // Step 1:  check if the object is runining
    //          if so QI for the requested interface
    //
    {
        IRunningObjectTable     *pROT = NULL;
        // check if the object is already running
        hr = IsRunningROT(pbc, pmkToLeft, &pROT);
        if (hr == NOERROR)
        {
            // object is running
            IUnknown *pUnk = NULL;

            // object is running
            // get the object and Qi for the requested interface
            hr = pROT->GetObject(this, &pUnk);
            if (SUCCEEDED(hr))
            {
                hr = pUnk->QueryInterface(riidRes, ppvRes);
                pUnk->Release();
            }

            pROT->Release();
            goto End;
        }
        else
        {
            if (pROT)
            {
                pROT->Release();
            }

            if (FAILED(hr))
            {
                // did not get ROT!!
                goto End;
            }
        }
    }

    // Step 2: get the bind options from the bind context
    bindopts.cbStruct = sizeof(BIND_OPTS);
    hr = pbc->GetBindOptions(&bindopts);
    if (FAILED(hr))
    {
        goto End;
    }

    // Step 3:  create a CBinding and releated objects and
    //          start a transaction


    hr = StartBinding(TRUE, pbc, pmkToLeft, riidRes, ppvRes);

End:

    PerfDbgLog2(tagCUrlMon, this, "-CUrlMon::BindToObject (hr:%lx, ppvobj:%lx)",
                    hr, (hr == S_OK) ? *ppvRes : NULL);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::BindToStorage
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::BindToStorage(IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, LPVOID);
    VDATEIFACE(pbc);
    PerfDbgLog2(tagCUrlMon, this, "+CUrlMon::BindToStorage (IBindCtx:%lx, pmkToLeft:%lx)", pbc, pmkToLeft);

    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }
    CBSC *pBSC = NULL;
    HRESULT       hr;
    BIND_OPTS     bindopts;
    CBinding      *pCBdg = NULL;
    FORMATETC     fetc;
    WCHAR         wzURL[MAX_URL_SIZE + 1];

    IID iidLocal = riid;

    *ppvObj = NULL;

    #if DBG==1
    {
        LPOLESTR pszStr;
        StringFromCLSID(riid, &pszStr);
        DbgLog2(tagCUrlMon, this, "CUrlMon::BindToStorage (szUrl:%ws)(iid:%ws)",
                GetUrl(), pszStr);
        delete pszStr;
    }
    #endif

    hr = StartBinding(FALSE, pbc, pmkToLeft, riid, ppvObj);

    PerfDbgLog2(tagCUrlMon, this, "-CUrlMon::BindToStorage (hr:%lx, ppvobj:%lx)",
        hr, (hr == S_OK) ? *ppvObj : NULL);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::StartBinding
//
//  Synopsis:   sets up the cbinding and starts the transaction
//
//  Arguments:  [fBindToObject] --
//              [pbc] --
//              [pmkToLeft] --
//              [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    8-20-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::StartBinding(BOOL fBindToObject, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid, void **ppvObj)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::StartBinding");

    HRESULT     hr = NOERROR;
    WCHAR       wzURL[MAX_URL_SIZE + 1];
    CBSC        *pBSC = NULL;
    CBinding    *pCBdg = NULL;
    BOOL        fUnknown = FALSE;

    do 
    {
        // No need to canonicalize URL here.  This should have already been done
        // by CreateURLMoniker.

        hr = ConstructURL(pbc, NULL, pmkToLeft, GetUrl(), wzURL, sizeof(wzURL),CU_NO_CANONICALIZE);

        if (hr != NOERROR)
        {
            break;
        }

        // moved to CBinding::StartBinding
        // BUG-WORK
        //if (!IsOInetProtocol(pbc, wzURL))
        //{
        //    hr = INET_E_UNKNOWN_PROTOCOL;
        //    break;
        //}

        // check if a BSC is registerted if not register our own one - for Office!
        IUnknown *pUnk = NULL;
        hr = GetObjectParam(pbc, REG_BSCB_HOLDER, IID_IBindStatusCallback, (IUnknown**)&pUnk);
        if ((hr == NOERROR) && pUnk)
        {
            // release - nothing to do
            pUnk->Release();
        }
        else
        {
            hr = NOERROR;
            if (fBindToObject)
            {
                pBSC = new CBSC(Medium_Unknown);
            }
            else
            {
                Medium medium = (riid == IID_IStorage) ? Medium_Storage : Medium_Stream;
                pBSC = new CBSC(medium);
                if (medium == Medium_Storage)
                {
                    fUnknown = TRUE;
                }

            }

            // no IBSC - create our own one and register 
            if (pBSC)
            {
                hr = RegisterBindStatusCallback(pbc, pBSC, 0, 0);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            
        }
        
        if (hr != NOERROR)
        {
            break;
        }
        // Create a CBinding object
        hr = CBinding::Create(NULL, wzURL, pbc, (fUnknown) ? IID_IUnknown : riid, fBindToObject, &pCBdg);

        if (hr != NOERROR)
        {
            break;
        }
        
        UrlMkAssert((pCBdg != NULL));

        if (fBindToObject)
        {
            pCBdg->SetMoniker(this);
        }

        {
            LPWSTR pwzExtra = NULL;
            // start the transaction now
            hr = pCBdg->StartBinding(wzURL, pbc, (fUnknown) ? IID_IUnknown : riid, fBindToObject, &pwzExtra, ppvObj);

            if (pwzExtra)
            {
                SetUrl(GetUrl(), pwzExtra);
            }
        }

        if( hr == INET_E_USE_EXTEND_BINDING )
        {
            // rosebud
            hr = NOERROR;

            // there is no need to return IBinding to client, se should
            // free it here.
            pCBdg->Release();
            break;
        }

        if (SUCCEEDED(hr))
        {
            if (pCBdg->IsAsyncTransaction() == FALSE)
            {
                hr = pCBdg->CompleteTransaction();
                if (SUCCEEDED(hr))
                {
                    // retrieve the requested object
                    if (pBSC)
                    {
                        hr = pBSC->GetRequestedObject(pbc, ppvObj);
                    }
                    else
                    {
                        hr = pCBdg->GetRequestedObject(pbc, (IUnknown **)ppvObj);
                    }
                }
            }
            else 
            {
                hr = pCBdg->GetRequestedObject(pbc, (IUnknown **)ppvObj);
            }
        }

        // in case the transaction could not be started,
        // the following release we terminate all releated objects
        pCBdg->Release();
    
        break;
    } while (TRUE);

    if (pBSC)
    {
        RevokeBindStatusCallback(pbc, pBSC);
        pBSC->Release();
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::StartBinding (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::Reduce
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [dwReduceHowFar] --
//              [IMoniker] --
//              [ppmkReduced] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::Reduce(IBindCtx *pbc, DWORD dwReduceHowFar,
                            IMoniker **ppmkToLeft,IMoniker **ppmkReduced)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Reduce");

    // There is nothing we can do to reduce our moniker
    *ppmkReduced = this;
    AddRef();

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::Reduce (hr:%lx)", MK_S_REDUCED_TO_SELF);
    return MK_S_REDUCED_TO_SELF;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::ComposeWith
//
//  Synopsis:
//
//  Arguments:  [pmkRight] --
//              [fOnlyIfNotGeneric] --
//              [ppmkComposite] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::ComposeWith(IMoniker *pmkRight, BOOL fOnlyIfNotGeneric, IMoniker **ppmkComposite)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::ComposeWith");
    VDATEIFACE(pmkRight);
    VDATEPTROUT(ppmkComposite, LPMONIKER);
    HRESULT hr = NOERROR;
    DWORD   dwMnk = 0;
    LPWSTR  wzURLRelative = NULL;

    *ppmkComposite = NULL;

    pmkRight->IsSystemMoniker(&dwMnk);
    if (dwMnk == MKSYS_URLMONIKER)
    {
        hr = pmkRight->GetDisplayName(NULL, NULL, &wzURLRelative);
        if (hr == NOERROR)
        {
            hr = CreateURLMoniker(this, wzURLRelative, ppmkComposite);
        }
    }
    else if (fOnlyIfNotGeneric)
    {
        hr = MK_E_NEEDGENERIC;
    }
    else
    {
        hr = CreateGenericComposite(this, pmkRight, ppmkComposite);
    }
    if (wzURLRelative)
    {
        delete wzURLRelative;
    }

    PerfDbgLog(tagCUrlMon, this, "-CUrlMon::ComposeWith");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::Enum
//
//  Synopsis:
//
//  Arguments:  [fForward] --
//              [ppenumMoniker] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::Enum(BOOL fForward, IEnumMoniker **ppenumMoniker)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Enum");
    VDATEPTROUT(ppenumMoniker, LPENUMMONIKER);
    *ppenumMoniker = NULL;

    PerfDbgLog(tagCUrlMon, this, "-CUrlMon::Enum (hr:0)");
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::IsEqual
//
//  Synopsis:
//
//  Arguments:  [pMnkOther] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//          REVIEW: this code will not work cross process.  What is the
//          correct implementation?
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::IsEqual(IMoniker *pMnkOther)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::IsEqual");
    HRESULT hr = S_FALSE;
    VDATEIFACE(pMnkOther);

    // We only worry about URL monikers
    if (this == pMnkOther)
    {
        // same object
        hr = NOERROR;
    }
    else if (IsUrlMoniker(pMnkOther))
    {
        LPWSTR szDispName = NULL;
        // The other moniker is a URL moniker.
        // get and compare the display names
        hr = pMnkOther->GetDisplayName(NULL, NULL, &szDispName);
        // Compare the URL's
        if (hr == NOERROR)
        {
            UrlMkAssert((_pwzUrl));
            UrlMkAssert((szDispName));

            hr = wcscmp(_pwzUrl, szDispName) ? S_FALSE : NOERROR;
        }

        if (szDispName)
        {
            delete szDispName;
        }
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::IsEqual (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::Hash
//
//  Synopsis:
//
//  Arguments:  [pdwHash] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//              9-02-95   AdriaanC (Adriaan Canter)
//                        Modified to use Pearson's Method
//
//  Notes:
//  32 bit hashing operator for IMoniker::Hash(DWORD*)
//
//  Method based upon "Fast Hashing of Variable Length Text Strings" ,
//  by Peter K. Pearson, Communications of the ACM,
//  June 1990 Vol. 33, Number 6. pp 677-680.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::Hash(DWORD* pdwHash)
{
  PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Hash");
  UrlMkAssert((_pwzUrl != NULL));

  HRESULT hr = NOERROR;

  UCHAR c0, c1, c2, c3;
  UCHAR* idx = (UCHAR*) _pwzUrl;

  static UCHAR T[256] =
    {
      1, 14,110, 25, 97,174,132,119,138,170,125,118, 27,233,140, 51,
      87,197,177,107,234,169, 56, 68, 30,  7,173, 73,188, 40, 36, 65,
      49,213,104,190, 57,211,148,223, 48,115, 15,  2, 67,186,210, 28,
      12,181,103, 70, 22, 58, 75, 78,183,167,238,157,124,147,172,144,
      176,161,141, 86, 60, 66,128, 83,156,241, 79, 46,168,198, 41,254,
      178, 85,253,237,250,154,133, 88, 35,206, 95,116,252,192, 54,221,
      102,218,255,240, 82,106,158,201, 61,  3, 89,  9, 42,155,159, 93,
      166, 80, 50, 34,175,195,100, 99, 26,150, 16,145,  4, 33,  8,189,
      121, 64, 77, 72,208,245,130,122,143, 55,105,134, 29,164,185,194,
      193,239,101,242,  5,171,126, 11, 74, 59,137,228,108,191,232,139,
      6, 24, 81, 20,127, 17, 91, 92,251,151,225,207, 21, 98,113,112,
      84,226, 18,214,199,187, 13, 32, 94,220,224,212,247,204,196, 43,
      249,236, 45,244,111,182,153,136,129, 90,217,202, 19,165,231, 71,
      230,142, 96,227, 62,179,246,114,162, 53,160,215,205,180, 47,109,
      44, 38, 31,149,135,  0,216, 52, 63, 23, 37, 69, 39,117,146,184,
      163,200,222,235,248,243,219, 10,152,131,123,229,203, 76,120,209
      };

  c0 = T[*idx];
  c1 = T[*idx+1 % 256];
  c2 = T[*idx+2 % 256];
  c3 = T[*idx+3 % 256];

#ifndef unix
  while ((WCHAR) *(WCHAR*) ++idx != L'\0')
#else
  // We are trying to cast a UCHAR as a WCHAR in the windows code. We will need
  // to handle alignments correctly here, as we cant randomly cast
  // a UCHAR * to a WCHAR *. 
  WCHAR wend = 0;
  while (memcmp(++idx,&wend,sizeof(WCHAR)))
#endif /* unix */
    {
      c0 = T[c0^*idx]; c1 = T[c1^*idx];
      c2 = T[c2^*idx]; c3 = T[c3^*idx];
    }

  *(((UCHAR*) pdwHash)+0) = c0;
  *(((UCHAR*) pdwHash)+1) = c1;
  *(((UCHAR*) pdwHash)+2) = c2;
  *(((UCHAR*) pdwHash)+3) = c3;

  PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::Hash (hr:%lx)", hr);
  return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::IsRunning
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [pmkNewlyRunning] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::IsRunning(IBindCtx *pbc, IMoniker *pmkToLeft, IMoniker *pmkNewlyRunning)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::IsRunning");
    HRESULT hr = NOERROR;
    VDATEIFACE(pbc);

    if (pmkToLeft)
        VDATEIFACE(pmkToLeft);
    if (pmkNewlyRunning)
        VDATEIFACE(pmkNewlyRunning);

    // This implementation was shamelessly stolen from the OLE sources.

    if (pmkToLeft == NULL)
    {
        if (pmkNewlyRunning != NULL)
        {
            hr = pmkNewlyRunning->IsEqual(this);

        }
        else
        {
            hr = IsRunningROT(pbc, NULL, NULL);
        }
    }
    else
    {
        UrlMkAssert((FALSE));
        hr = S_FALSE;
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::IsRunning (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetTimeOfLastChange
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [pFileTime] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetTimeOfLastChange(IBindCtx *pbc, IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::GetTimeOfLastChange");
    VDATEIFACE(pbc);
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }
    VDATEPTROUT(pFileTime, FILETIME);

    HRESULT hr = MK_E_UNAVAILABLE;
    IRunningObjectTable *pROT;

    hr = pbc->GetRunningObjectTable(&pROT);

    if (SUCCEEDED(hr))
    {
        hr = pROT->GetTimeOfLastChange(this, pFileTime);
        pROT->Release();
    }
    else
    {
        hr = MK_E_UNAVAILABLE;
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::GetTimeOfLastChange (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::Inverse
//
//  Synopsis:
//
//  Arguments:  [ppmk] --
//
//  Returns:
//
//  History:    1-16-96   JohannP (Johann Posch)   Created
//
//  Notes:     urlmon does not have aninverse
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::Inverse(IMoniker **ppmk)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::Inverse");
    VDATEPTROUT(ppmk, LPMONIKER);

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::Inverse (hr:%lx)", MK_E_NOINVERSE);
    return MK_E_NOINVERSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::CommonPrefixWith
//
//  Synopsis:
//
//  Arguments:  [pmkOther] --
//              [ppmkPrefix] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::CommonPrefixWith(IMoniker *pmkOther, IMoniker **ppmkPrefix)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::CommonPrefixWith");
    HRESULT hr = E_NOTIMPL;
    VDATEPTROUT(ppmkPrefix, LPMONIKER);
    *ppmkPrefix = NULL;
    VDATEIFACE(pmkOther);

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::CommonPrefixWith (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::RelativePathTo
//
//  Synopsis:
//
//  Arguments:  [pmkOther] --
//              [ppmkRelPath] --
//
//  Returns:
//
//  History:    1-16-96   JohannP (Johann Posch)   Created
//              8-20-96   ClarG   (Clarence Glasse) Implemented
//
//  Notes: Code is based on composition algorithm in rfc 1808 (this code does
//         the reverse of that).
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::RelativePathTo(IMoniker *pmkOther, IMoniker **ppmkRelPath)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::RelativePathTo");
    HRESULT hr = NOERROR;
    VDATEPTROUT(ppmkRelPath, LPMONIKER);
    *ppmkRelPath = NULL;
    VDATEIFACE(pmkOther);

    CUrl *pUrlThis = NULL;
    CUrl *pUrlOther = NULL;
    LPSTR pch = NULL;
    LPSTR pszQueryThis = NULL;
    LPSTR pszQueryOther = NULL;
    LPSTR pszParamsThis = NULL;
    LPSTR pszParamsOther = NULL;

    char szRelPath[MAX_URL_SIZE + 1];

    if (!_pwzUrl)
    {
        hr = MK_E_NOTBINDABLE;
        goto End;
    }

    if (IsEqual(pmkOther) == S_OK)
    {
        // we are equal to pmkOther, so create an empty path URL moniker.
        if ((*ppmkRelPath = CreateEmptyPathUrlMon()) == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto End;
        }
        // CUrlmon has refcount of 1 now
    }
    else if (IsUrlMoniker(pmkOther))
    {
        if ((hr = HrCreateCUrlFromUrlMon(pmkOther, TRUE, &pUrlOther)) != NOERROR)
        {
            if (hr == MK_E_SYNTAX)
                hr = MK_E_NOTBINDABLE;
            goto End;
        }

        if ((hr = HrCreateCUrlFromUrlStr(_pwzUrl, TRUE, &pUrlThis)) != NOERROR)
        {
            if (hr == MK_E_SYNTAX)
                hr = MK_E_NOTBINDABLE;
            goto End;
        }

        // Each URL has been parsed into its separate components.
        // Now compute the relative path.
        if ((pUrlThis->_dwProto == DLD_PROTOCOL_NONE) ||
            (pUrlOther->_dwProto == DLD_PROTOCOL_NONE))
        {
            // Unrecognized protocol; return MK_S_HIM or MK_E_NOTBINDABLE
            if (pUrlThis->_pszProtocol[0] && pUrlOther->_pszProtocol[0])
            {
                *ppmkRelPath = pmkOther;
                pmkOther->AddRef();
                hr = MK_S_HIM;
            }
            else
            {
                hr = MK_E_NOTBINDABLE;
            }
            goto End;
        }

        // if the scheme and net_loc portion of the url are not equal,
        // return MK_S_HIM
        if ((pUrlThis->_dwProto != pUrlOther->_dwProto) ||
            (lstrcmpi(pUrlThis->_pszServerName, pUrlOther->_pszServerName) != 0) ||
            (pUrlThis->_ipPort != pUrlOther->_ipPort) ||
            (lstrcmpi(pUrlThis->_pszUserName, pUrlOther->_pszUserName) != 0) ||
            (lstrcmpi(pUrlThis->_pszPassword, pUrlOther->_pszPassword) != 0))
        {
            *ppmkRelPath = pmkOther;
            pmkOther->AddRef();
            hr = MK_S_HIM;
            goto End;
        }

#ifdef URLMON_RELPATHTO_PARSE_QUERY_PARAMS
        // parse the query and params info
        ParseUrlQuery(pUrlThis->_pszObject, &pszQueryThis);
        ParseUrlQuery(pUrlOther->_pszObject, &pszQueryOther);
        ParseUrlParams(pUrlThis->_pszObject, &pszParamsThis);
        ParseUrlParams(pUrlOther->_pszObject, &pszParamsOther);
#endif // URLMON_RELPATHTO_PARSE_QUERY_PARAMS

        // compute the relative path
        hr = HrGetRelativePath(
                pUrlThis->_pszObject, pUrlOther->_pszObject,
                pUrlThis->_dwProto, pUrlThis->_pszServerName, szRelPath);
        if (FAILED(hr))
            goto End;

#ifdef URLMON_RELPATHTO_PARSE_QUERY_PARAMS
        // append the appropriate query and params info
        AddParamsAndQueryToRelPath(
            szRelPath, pszParamsThis, pszParamsOther, pszQueryThis, pszQueryOther);
#endif // URLMON_RELPATHTO_PARSE_QUERY_PARAMS

        if (szRelPath[0])
        {
            HRESULT hr2 = NOERROR;
            WCHAR wzObjRel[MAX_URL_SIZE + 1];

            A2W(szRelPath, wzObjRel, MAX_URL_SIZE);
            hr2 = CreateURLMoniker(NULL, wzObjRel, ppmkRelPath);
            if (FAILED(hr2))
                hr = hr2;
        }
        else
        {
            *ppmkRelPath = CreateEmptyPathUrlMon();
            if (!(*ppmkRelPath))
                hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = MonikerRelativePathTo(this, pmkOther, ppmkRelPath, TRUE);
    }

End:
    if (pUrlThis)
        delete pUrlThis;
    if (pUrlOther)
        delete pUrlOther;
    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::RelativePathTo (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::GetDisplayName
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [ppszDisplayName] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::GetDisplayName(IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::GetDisplayName");
    HRESULT hr = NOERROR;
    VDATEPTROUT(ppszDisplayName, LPSTR);
    *ppszDisplayName = NULL;
    if (pbc)
    {
        VDATEIFACE(pbc);
    }
    if (pmkToLeft)
    {
        VDATEIFACE(pmkToLeft);
    }

    *ppszDisplayName = OLESTRDuplicate(_pwzUrl);

    if (*ppszDisplayName == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    PerfDbgLog2(tagCUrlMon, this, "-CUrlMon::GetDisplayName (hr:%lx) [%ws]", hr, *ppszDisplayName ? *ppszDisplayName : L"");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::ParseDisplayName
//
//  Synopsis:
//
//  Arguments:  [IMoniker] --
//              [pmkToLeft] --
//              [ULONG] --
//              [IMoniker] --
//              [ppmkOut] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::ParseDisplayName(IBindCtx *pbc,IMoniker *pmkToLeft,
                LPOLESTR pozDisplayName,ULONG *pchEaten,IMoniker **ppmkOut)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::ParseDisplayName");

    HRESULT hr;
    WCHAR wzURL[MAX_URL_SIZE + 1];
    VDATEPTROUT(ppmkOut, LPMONIKER);
    *ppmkOut = NULL;
    VDATEIFACE(pbc);
    if (pmkToLeft) VDATEIFACE(pmkToLeft);
    VDATEPTRIN(pozDisplayName, char);
    VDATEPTROUT(pchEaten, ULONG);

    hr = ConstructURL(pbc, this, pmkToLeft, pozDisplayName, wzURL,
            sizeof(wzURL), CU_CANONICALIZE);

    if (hr != NOERROR)
    {
        goto End;
    }

    if (!wcscmp(_pwzUrl, wzURL))
    {
        // Return ourselves if new URL is the same.

        *ppmkOut = this;
        AddRef();

        hr = NOERROR;
    }
    else
    {
        hr = CreateURLMoniker(NULL, wzURL, ppmkOut);
    }

    if (hr == NOERROR)
    {
        // We have eaten all the characters
        *pchEaten = wcslen(pozDisplayName);
    }

End:

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::ParseDisplayName (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::IsSystemMoniker
//
//  Synopsis:
//
//  Arguments:  [pdwMksys] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlMon::IsSystemMoniker(DWORD *pdwMksys)
{
    PerfDbgLog(tagCUrlMon, this, "CUrlMon::IsSystemMoniker");
    VDATEPTROUT(pdwMksys, DWORD);
    *pdwMksys = MKSYS_URLMONIKER;
    return NOERROR;
}


// Delete the URL string if we have one
void CUrlMon::DeleteUrl()
{
    if (_pwzUrl)
        delete [] _pwzUrl;

    _pwzUrl = NULL;
}

HRESULT CUrlMon::SetUrl(LPWSTR pwzUrl, LPWSTR pwzExtra)
{
    int clen;
    HRESULT hr = NOERROR;

    TransAssert((pwzUrl));

    clen  = wcslen(pwzUrl) + 1;
    if (pwzExtra)
    {
        clen  += wcslen(pwzExtra);
    }
    LPWSTR pwzStr = (LPWSTR) new WCHAR [clen];
    if (pwzStr)
    {
        wcscpy(pwzStr, pwzUrl);
        if (pwzExtra)
        {
            wcscat(pwzStr, pwzExtra);
        }
        if (_pwzUrl)
        {
            delete [] _pwzUrl;
        }
        _pwzUrl = pwzStr;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


//    Helper function for IMoniker::IsRunning and BindToObject.
//    Return NOERROR if running, and S_FALSE if not.
//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::IsRunningROT
//
//  Synopsis:   Checks if the moniker is running
//
//  Arguments:  [pbc] --
//              [pmkToLeft] --
//              [ppROT] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CUrlMon::IsRunningROT(IBindCtx *pbc, IMoniker *pmkToLeft, IRunningObjectTable **ppROT)
{
    HRESULT hr;
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::IsRunningROT");
    IRunningObjectTable *pROT;

    hr = pbc->GetRunningObjectTable(&pROT);
    if (SUCCEEDED(hr))
    {
        hr = pROT->IsRunning(this);

        if (ppROT != NULL && SUCCEEDED(hr))
        {
            *ppROT = pROT;
        }
        else
        {
            pROT->Release();
        }
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::IsRunningROT (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlMon::IsUrlMoniker
//
//  Synopsis:   Checks if pMk is a URL moniker
//
//  Arguments:  [pMk] -- the moniker to be checked
//
//  Returns:    true if moniker is URL moniker
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CUrlMon::IsUrlMoniker(IMoniker *pMk)
{
    PerfDbgLog(tagCUrlMon, this, "+CUrlMon::IsUrlMoniker");
    BOOL fRet = FALSE;

    if (pMk)
    {
        DWORD dwMnk = 0;
        pMk->IsSystemMoniker(&dwMnk);
        fRet = (dwMnk == MKSYS_URLMONIKER);
    }

    PerfDbgLog1(tagCUrlMon, this, "-CUrlMon::IsUrlMoniker (fRet:%d)", fRet);
    return fRet;
}


// ********** Helper Functions **********
// These functions are used by RelativePathTo

//+---------------------------------------------------------------------------
//
//  Function: CreateEmptyPathUrlMon
//
//  Synopsis: Create a UrlMon with empty path "", such that composing it onto
//            a base UrlMon via IMoniker::ComposeWith will yield a moniker
//            equal to the base UrlMon.
//            We don't call CreateUrlMoniker(""), to do this because that will
//            return a UrlMon with path "/".  Composing such a moniker onto
//            a base UrlMon out everything after the host name.
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:   8-16-96   ClarG  (Clarence Glasse)          Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CUrlMon * CreateEmptyPathUrlMon()
{
    PerfDbgLog(tagCUrlMon, NULL, "+CreateEmptyPathUrlMon");
    CUrlMon * pUMk = NULL;
    LPWSTR pwzUrlEmpty = NULL;

    // allocate empty string
    if ((pwzUrlEmpty = new WCHAR [1]) == NULL)
    {
        goto End;
    }
    pwzUrlEmpty[0] = 0;

    if ((pUMk = new CUrlMon(pwzUrlEmpty)) == NULL)
    {
        delete pwzUrlEmpty;
        goto End;
    }
    // pUMk has refcount of 1 now

End:
    PerfDbgLog1(tagCUrlMon, NULL, "-CreateEmptyPathUrlMon, pUMk:%lx", pUMk);
    return pUMk;
}


//+---------------------------------------------------------------------------
//
//  Function: HrCreateCUrlFromUrlMon
//
//  Synopsis: Given a Url moniker pmkUrl, create a CUrl object.  Assumes that
//            pmkUrl is indeed a Url moniker.
//
//  Arguments:  [pmkUrl] -- Url moniker
//              [fParseUrl] -- if TRUE, parse the URL.
//              [ppUrl] -- returns created CUrl obj, which caller must delete
//                         when finished with it.
//
//  Returns: NOERROR is successful.
//
//  History:   8-16-96   ClarG  (Clarence Glasse)          Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT  HrCreateCUrlFromUrlMon(LPMONIKER pmkUrl, BOOL fParseUrl, CUrl **ppUrl)
{
    PerfDbgLog(tagCUrlMon, NULL, "+HrCreateCUrlFromUrlMon");
    UrlMkAssert((pmkUrl));
    UrlMkAssert((ppUrl));
    HRESULT hr = NOERROR;
    LPBC pbc = NULL;
    LPWSTR pwzUrl = NULL;

    if ((*ppUrl = new CUrl) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto End;
    }

    if( !(*ppUrl)->CUrlInitAll() )
    {
        hr = E_OUTOFMEMORY;
        goto End;
    }

    if ((hr = CreateBindCtx(0, &pbc)) != NOERROR)
    {
        goto End;
    }

    if ((hr = pmkUrl->GetDisplayName(pbc, NULL, &pwzUrl)) != NOERROR)
    {
        goto End;
    }


    W2A(pwzUrl, (*ppUrl)->_pszBaseURL, MAX_URL_SIZE);

    if (fParseUrl)
    {
        if (!(*ppUrl)->ParseUrl())
        {
            hr = MK_E_SYNTAX;
            goto End;
        }
    }

End:
    if (pbc)
        pbc->Release();
    if (pwzUrl)
        CoTaskMemFree(pwzUrl);

    if ((hr != NOERROR) && (*ppUrl != NULL))
    {
        delete *ppUrl;
        *ppUrl = NULL;
    }

    PerfDbgLog1(tagCUrlMon, NULL, "-HrCreateCUrlFromUrlMon (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function: HrCreateCUrlFromUrlStr
//
//  Synopsis: Given a Url string pwzUrl, create a CUrl object.
//
//  Arguments:  [pwzUrl] -- Url string
//              [fParseUrl] -- if TRUE, parse the URL.
//              [ppUrl] -- returns created CUrl obj, which caller must delete
//                         when finished with it.
//
//  Returns: NOERROR is successful.
//
//  History:   8-16-96   ClarG  (Clarence Glasse)          Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT  HrCreateCUrlFromUrlStr(LPCWSTR pwzUrl, BOOL fParseUrl, CUrl **ppUrl)
{
    PerfDbgLog(tagCUrlMon, NULL, "+HrCreateCUrlFromUrlStr");
    UrlMkAssert((pwzUrl));
    UrlMkAssert((ppUrl));
    HRESULT hr = NOERROR;

    if ((*ppUrl = new CUrl) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto End;
    }

    if( !(*ppUrl)->CUrlInitAll() )
    {
        hr = E_OUTOFMEMORY;
        goto End;
    }

    W2A(pwzUrl, (*ppUrl)->_pszBaseURL, MAX_URL_SIZE);

    if (fParseUrl)
    {
        if (!(*ppUrl)->ParseUrl())
        {
            hr = MK_E_SYNTAX;
            goto End;
        }
    }

End:
    if ((hr != NOERROR) && (*ppUrl != NULL))
    {
        delete *ppUrl;
        *ppUrl = NULL;
    }

    PerfDbgLog1(tagCUrlMon, NULL, "-HrCreateCUrlFromUrlStr (hr:%lx)", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function: IsSeparator
//
//  Synopsis: Return TRUE if ch is a file path or url path separator char.
//
//  Arguments:  [ch] -- the char in question
//                         when finished with it.
//
//  Returns: TRUE if ch is a separator char
//
//  History:   8-20-96   ClarG  (Clarence Glasse)          Created
//
//  Notes: This is taken from OLE2 file moniker code
//
//----------------------------------------------------------------------------
inline BOOL IsSeparator( char ch )
{
    return (ch == '\\' || ch == '/' || ch == ':');
}


//+---------------------------------------------------------------------------
//
//  Function: CountSegments
//
//  Synopsis: Return the number of segments in file or url path pch.
//
//  Arguments:  [pch] -- the string in question.
//
//  Returns: number of segments in pch
//
//  History:   8-20-96   ClarG  (Clarence Glasse)          Created
//
//  Notes: This is taken from OLE2 file moniker code
//
//----------------------------------------------------------------------------
int CountSegments ( LPSTR pch )
{
    PerfDbgLog(tagCUrlMon, NULL, "+CountSegments");
    //  counts the number of pieces in a path, after the first colon, if
    //  there is one

    int n = 0;
    LPSTR pch1;
    pch1 = pch;
    while (*pch1 != '\0' && *pch1 != ':') IncLpch(pch1);
    if (*pch1 == ':') pch = ++pch1;
    while (*pch != '\0')
    {
        while (*pch && IsSeparator(*pch)) pch++;
        if (*pch) n++;
        while (*pch && (!IsSeparator(*pch))) IncLpch(pch);
    }

    PerfDbgLog1(tagCUrlMon, NULL, "-CountSegments (n:%ld)", n);
    return n;
}


//+---------------------------------------------------------------------------
//
//  Function: HrGetRelativePath
//
//  Synopsis: Compute and return relative path from url path
//            lpszBase to url path lpszOther.
//
//  Arguments:  [lpszBase] -- the base path without the scheme and host info
//              [lpszOther] -- the target path without the scheme and host
//              [dwProto] -- DLD_PROTOCOL_XXX value, indicating the scheme
//                           of the urls.
//              [lpszHost] -- the host name for the urls
//              [lpszRelPath] -- buffer of size MAX_URL_SIZE that returns the
//                               relative path.
//
//  Returns: NOERROR, MK_S_HIM, MK_E_NOTBINDABLE, or some other hresult.
//
//  History:   8-20-96   ClarG  (Clarence Glasse)          Created
//
//  Notes: This is based on OLE2 file moniker code
//
//----------------------------------------------------------------------------
HRESULT HrGetRelativePath(
    LPSTR lpszBase, LPSTR lpszOther, DWORD dwProto, LPSTR lpszHost,
    LPSTR lpszRelPath)
{
    PerfDbgLog(tagCUrlMon, NULL, "+HrGetRelativePath");
    HRESULT hr = NOERROR;
    LPSTR lpszRover;
    LPSTR lpszMarker;
    LPSTR lpch;
    LPSTR lpchStripBaseSav = NULL;
    char  ch1;
    char  ch2;
    char  chStripBaseSav;
    char  chNull = '\0';
    int i;
    int cAnti;

    lpszRelPath[0] = 0;

    // if neither lpszBase nor lpszRelPath look like absolute url or file
    // paths, return MK_E_NOTBINDALBE
    if (!lpszBase[0] || !lpszOther[0])
    {
        hr = MK_E_NOTBINDABLE;
        goto End;
    }

    if ((lpszBase[0] != '\\') && (lpszBase[0] != '/') && (lpszBase[1] != ':'))
    {
        hr = MK_E_NOTBINDABLE;
        goto End;
    }

    if ((lpszOther[0] != '\\') && (lpszOther[0] != '/') && (lpszOther[1] != ':'))
    {
        hr = MK_E_NOTBINDABLE;
        goto End;
    }

    if (lstrcmpi(lpszBase, lpszOther) == 0)
    {
        // if paths are equal, relative path is empty string
        lpszRelPath[0] = 0;
        hr = NOERROR;
        goto End;
    }

    // if base does not end in a separator, remove its last piece
    lpch = lpszBase + lstrlen(lpszBase);
    for( ; ((!(IsSeparator(*lpch))) && (lpch > lpszBase)); DecLpch(lpszBase, lpch) );
    if (IsSeparator(*lpch))
    {
        IncLpch(lpch);
        lpchStripBaseSav = lpch;
        chStripBaseSav = *lpch;
        *lpch = '\0';
    }

    lpszRover = lpszBase;
    lpszMarker = lpszRover;
    i = 0;
    lpszOther = lpszOther;

    while (*lpszRover != '\0')
    {
        while (*lpszRover && IsSeparator(*lpszRover)) lpszRover++;
        while (*lpszRover && !IsSeparator(*lpszRover)) IncLpch(lpszRover);
        //      the first part of the path is between m_szPath and
        //      lpszRover
        i = (int) (lpszRover - lpszBase);
        ch1 = *lpszRover;
        ch2 = *(lpszOther + i);
        *lpszRover = '\0';
        *(lpszOther + i) = '\0';

        if (lstrcmpi(lpszBase, lpszOther) == 0)
            lpszMarker = lpszRover;
        else
            lpszRover = &chNull;

        *(lpszBase + i) = ch1;
        *(lpszOther + i) = ch2;
    }

    //  common portion is from lpszBase to lpszMarker
    i = (int) (lpszMarker - lpszBase);
    if ((!lpszHost || !lpszHost[0]) && (i == 0))
    {
        lstrcpy(lpszRelPath, lpszOther);
        hr = MK_S_HIM;
        goto End;
    }

    lpszRover = lpszRelPath;
    while (IsSeparator(*(lpszOther+i))) i++;
    cAnti = CountSegments(lpszMarker);

    while (cAnti)
    {
        if (dwProto != DLD_PROTOCOL_FILE)
            lstrcpy(lpszRover, "../");
        else
            lstrcpy(lpszRover, "..\\");
        lpszRover += 3;
        cAnti--;
    }

    lstrcpy(lpszRover, lpszOther + i);

End:
    if (lpchStripBaseSav)
    {
        *lpchStripBaseSav = chStripBaseSav;
    }

    PerfDbgLog2(tagCUrlMon, NULL, "-HrGetRelativePath [%s], hr:%lx", lpszRelPath?lpszRelPath:"", hr);
    return hr;
}


#ifdef URLMON_RELPATHTO_PARSE_QUERY_PARAMS
//+---------------------------------------------------------------------------
//
//  Function: ParseUrlQuery
//
//  Synopsis: Parse the Query portion of a url
//
//  Arguments:  [pszURL] -- url to parse; the query portion is "removed" from
//                          pszURL when function returns.
//              [ppszQuery] -- returns pointer to query portion of URL
//
//  Returns: nothing
//
//  History:   8-20-96   ClarG  (Clarence Glasse)          Created
//
//  Notes: This is based on OLE2 file moniker code
//
//----------------------------------------------------------------------------
void ParseUrlQuery(LPSTR pszURL, LPSTR *ppszQuery)
{
    PerfDbgLog(tagCUrlMon, NULL, "+ParseUrlQuery");
    LPSTR pch = NULL;

    *ppszQuery = NULL;

    for (pch = pszURL; *pch; IncLpch(pch))
    {
        if (*pch == '?')
        {
            *pch = '\0';
            pch++;
            if (*pch)
                *ppszQuery = pch;
            break;
        }
    }

    PerfDbgLog(tagCUrlMon, NULL, "-ParseUrlQuery");
}


//+---------------------------------------------------------------------------
//
//  Function: ParseUrlParams
//
//  Synopsis: Parse the Params portion of a url
//
//  Arguments:  [pszURL] -- url to parse; the params portion is "removed" from
//                          pszURL when function returns.
//              [ppszParams] -- returns pointer to params portion of URL
//
//  Returns: nothing
//
//  History:   8-20-96   ClarG  (Clarence Glasse)          Created
//
//  Notes: This is based on OLE2 file moniker code
//
//----------------------------------------------------------------------------
void ParseUrlParams(LPSTR pszURL, LPSTR *ppszParams)
{
    PerfDbgLog(tagCUrlMon, NULL, "+ParseUrlParams");
    LPSTR pch = NULL;

    *ppszParams = NULL;

    for (pch = pszURL; *pch; IncLpch(pch))
    {
        if (*pch == ';')
        {
            *pch = '\0';
            pch++;
            if (*pch)
                *ppszParams = pch;
            break;
        }
    }

    PerfDbgLog(tagCUrlMon, NULL, "-ParseUrlParams");
}


//+---------------------------------------------------------------------------
//
//  Function: AddParamsAndQueryToRelPath
//
//  Synopsis: Given a computed relative URL path from a base to a target,
//            append the appropriate Params and Query info.
//
//  Arguments:  [szRelPath] -- computed relative path
//              [pszParamsBase] -- Params info of base URL
//              [pszParamsOther] -- Params info of target URL
//              [pszQueryBase] -- Query info of base URL
//              [pszQueryOther] -- Query info of target URL
//
//  Returns: nothing
//
//  History:   8-20-96   ClarG  (Clarence Glasse)          Created
//
//  Notes: This is based on OLE2 file moniker code
//
//----------------------------------------------------------------------------
void AddParamsAndQueryToRelPath(
    LPSTR szRelPath,
    LPSTR pszParamsBase, LPSTR pszParamsOther,
    LPSTR pszQueryBase, LPSTR pszQueryOther)
{
    PerfDbgLog(tagCUrlMon, NULL, "+AddParamsAndQueryToRelPath");

    if (szRelPath[0])
    {
        if (pszParamsOther)
        {
            lstrcat(szRelPath, ";");
            lstrcat(szRelPath, pszParamsOther);
        }
        if (pszQueryOther)
        {
            lstrcat(szRelPath, "?");
            lstrcat(szRelPath, pszQueryOther);
        }
    }
    else
    {
        if (pszParamsOther &&
            (!pszParamsBase || (lstrcmpi(pszParamsOther, pszParamsBase) !=  0)))
        {
            lstrcat(szRelPath, ";");
            lstrcat(szRelPath, pszParamsOther);

            if (pszQueryOther)
            {
                lstrcat(szRelPath, "?");
                lstrcat(szRelPath, pszQueryOther);
            }
        }
        else if (pszQueryOther &&
                 (!pszQueryBase || (lstrcmpi(pszQueryOther, pszQueryBase) !=  0)))
        {
            lstrcat(szRelPath, "?");
            lstrcat(szRelPath, pszQueryOther);
        }
    }

    PerfDbgLog(tagCUrlMon, NULL, "-AddParamsAndQueryToRelPath");
}
#endif // URLMON_RELPATHTO_PARSE_QUERY_PARAMS
