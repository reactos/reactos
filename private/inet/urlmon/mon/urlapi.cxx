//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       UrlApi.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-25-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <mon.h>
#include <shlwapip.h>
#include "urlapi.hxx"
#include "httpneg.hxx"
#include "mpxbsc.hxx"
#ifndef unix
#include "..\trans\transact.hxx"
#include "..\trans\bindctx.hxx"
#include "..\trans\urlmk.hxx"
#else
#include "../trans/transact.hxx"
#include "../trans/bindctx.hxx"
#include "../trans/urlmk.hxx"
#endif /* unix */

PerfDbgTag(tagUrlApi, "Urlmon", "Log UrlMon API", DEB_ASYNCAPIS);

// API defined in trans\oinet.cxx
BOOL IsOInetProtocol(IBindCtx*, LPCWSTR);

//+---------------------------------------------------------------------------
//
//  Function:   CreateURLMoniker
//
//  Synopsis:   Create a new empty URL Moniker object
//
//  Arguments:  [pMkCtx] -- the context moniker
//              [szUrl] --  url string
//              [ppmk] --   new moniker
//
//  Returns:
//
//  History:    12-13-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateURLMoniker(LPMONIKER pMkCtx, LPCWSTR szUrl, LPMONIKER FAR * ppMk)
{
    VDATEPTROUT(ppMk,LPMONIKER);
    VDATEPTRIN(szUrl,WCHAR);
    PerfDbgLog2(tagUrlApi, NULL, "+CreateURLMoniker (szUrl%ws, pMkCtx:%lx)",szUrl?szUrl:L"<NULL PATH>", pMkCtx);
    HRESULT hr = NOERROR;
    LPWSTR  szUrlLocal = NULL;
    WCHAR   wzUrlStr[MAX_URL_SIZE + 1];
    CUrlMon * pUMk = NULL;

    hr = ConstructURL(NULL, pMkCtx, NULL, (LPWSTR)szUrl, wzUrlStr,
            sizeof(wzUrlStr), CU_CANONICALIZE);

    if (hr != NOERROR)
    {
        goto CreateExit;
    }

    szUrlLocal = new WCHAR [wcslen(wzUrlStr) + 1];
    if (szUrlLocal)
    {
        wcscpy(szUrlLocal, wzUrlStr);

        if ((pUMk = new CUrlMon(szUrlLocal)) == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        // CUrlmon has refcount of 1 now
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

CreateExit:

    *ppMk = pUMk;

    PerfDbgLog2(tagUrlApi, NULL, "-CreateURLMoniker(%ws, Mnk:%lx)",wzUrlStr?wzUrlStr:L"<NULL PATH>",pUMk);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   MkParseDisplayNameEx
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [pszName] --
//              [pchEaten] --
//              [ppmk] --
//
//  Returns:
//
//  History:    12-13-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI MkParseDisplayNameEx(LPBC pbc, LPCWSTR szDispName, ULONG *pchEaten, LPMONIKER *ppmk)
{
    VDATEPTROUT(ppmk, LPMONIKER);
    VDATEPTROUT(pchEaten, ULONG);
    VDATEIFACE(pbc);
    VDATEPTRIN(szDispName, WCHAR);
    HRESULT hr = NOERROR;
    WCHAR   wzUrlStr[MAX_URL_SIZE + 1];
    PerfDbgLog1(tagUrlApi, NULL, "+MkParseDisplayNameEx(%ws)",szDispName);


    // No need to canonicalize the URL here.  It will be done later by
    // CreateURLMoniker call below.

    hr = ConstructURL(pbc, NULL, NULL, (LPWSTR)szDispName, wzUrlStr,
            sizeof(wzUrlStr), CU_NO_CANONICALIZE);

    // for unknown protocol (not registered)
    // instead of returning a Moniker which will fail on the Bind
    // we should call the system's MkParseDisplayName()
    if( hr == NOERROR )
    {
        // this is an internal API defined at trans\oinet.cxx
        if(!IsOInetProtocol(pbc, wzUrlStr))
        {
            // for Office backward compatibility...
            if( !StrCmpNIW(wzUrlStr, L"telnet", (sizeof("telnet") - 1) ) )
            {
                hr = NOERROR;
            }
            else
            {
                hr = INET_E_UNKNOWN_PROTOCOL;
            }
        }
    }

    if (hr == NOERROR)
    {
        IMoniker *pmk = NULL;
        // create a URL Moniker and call ParseDisplayName
        hr = CreateURLMoniker(NULL, wzUrlStr, &pmk);
        if (hr == NOERROR)
        {
            *pchEaten = wcslen(szDispName);
            *ppmk = pmk;
        }
        else
        {
            *pchEaten = 0;
            *ppmk = NULL;
        }
    }
    else
    {
        // call the standard OLE parser
        hr = MkParseDisplayName(pbc, szDispName, pchEaten, ppmk);
    }

    PerfDbgLog1(tagUrlApi, NULL, "-MkParseDisplayNameEx(%ws)",szDispName);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAsyncBindCtx
//
//  Synopsis:
//
//  Arguments:  [reserved] --
//              [pBSCb] --
//              [ppBC] --
//
//  Returns:
//
//  History:    10-25-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEnum, IBindCtx **ppBC)
{
    PerfDbgLog1(tagUrlApi, NULL, "+CreateAsyncBindCtx(%lx)",pBSCb);
    HRESULT hr = NOERROR;
    IUnknown *pUnk;


    if (pBSCb == NULL || ppBC == NULL)
    {
        hr = E_INVALIDARG;
        goto End;
    }

    hr = CreateBindCtx(reserved, ppBC);
    if (hr == NOERROR)
    {
        BIND_OPTS BindOpts;
        BindOpts.cbStruct = sizeof(BIND_OPTS);
        BindOpts.grfFlags = BIND_MAYBOTHERUSER;
        BindOpts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
        BindOpts.dwTickCountDeadline = 0L;

        ((IBindCtx *)*ppBC)->SetBindOptions(&BindOpts);

        // Register the IBindStatusCallback in the bind context.
        if (pBSCb != NULL)
        {
            CBSCHolder *pCBSCHolder;

            hr = GetBSCHolder(*ppBC, &pCBSCHolder);

            if (hr == NOERROR)
            {
                //hr = pCBSCHolder->AddNode(pBSCb, BSCO_ALLONIBSC);
                hr = pCBSCHolder->SetMainNode(pBSCb, 0);
                pCBSCHolder->Release();
            }
        }
        if ((hr == NOERROR) && (pEnum != NULL))
        {
            hr = RegisterFormatEnumerator(*ppBC, pEnum, 0);
        }
    }

End:

    PerfDbgLog1(tagUrlApi, NULL, "-CreateAsyncBindCtx(%lx)",pBSCb);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateAsyncBindCtxEx
//
//  Synopsis:
//
//  Arguments:  [pbc] --
//              [dwOptions] --
//              [pBSCb] --
//              [pEnum] --
//              [ppBC] --
//              [reserved] --
//
//  Returns:
//
//  History:    10-09-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateAsyncBindCtxEx(IBindCtx *pbc, DWORD dwOptions, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEnum, IBindCtx **ppBC, DWORD reserved)
{
    PerfDbgLog1(tagUrlApi, NULL, "+CreateAsyncBindCtxEx(%lx)",pBSCb);
    HRESULT hr = NOERROR;
    IUnknown *pUnk;
    CBindCtx *pCBCtx = NULL;


    if (ppBC == NULL)
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        hr = CBindCtx::Create(&pCBCtx, pbc);
        if (hr == NOERROR)
        {

            *ppBC = pCBCtx;
            BIND_OPTS BindOpts;
            BindOpts.cbStruct = sizeof(BIND_OPTS);
            BindOpts.grfFlags = BIND_MAYBOTHERUSER;
            BindOpts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
            BindOpts.dwTickCountDeadline = 0L;

            ((IBindCtx *)*ppBC)->SetBindOptions(&BindOpts);

            // Register the IBindStatusCallback in the bind context.
            if (pBSCb != NULL)
            {
                CBSCHolder *pCBSCHolder;

                hr = GetBSCHolder(*ppBC, &pCBSCHolder);

                if (hr == NOERROR)
                {
                    //hr = pCBSCHolder->AddNode(pBSCb, BSCO_ALLONIBSC);
                    hr = pCBSCHolder->SetMainNode(pBSCb, 0);
                    pCBSCHolder->Release();
                }
            }
            if ((hr == NOERROR) && (pEnum != NULL))
            {
                hr = RegisterFormatEnumerator(*ppBC, pEnum, 0);
            }
        }
    }

    PerfDbgLog1(tagUrlApi, NULL, "-CreateAsyncBindCtxEx(%lx)",pBSCb);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsAsyncMoniker
//
//  Synopsis:
//
//  Arguments:  [pmk] --
//
//  Returns:
//
//  History:    2-13-96   JohannP (Johann Posch)   Created
//              3-05-96   JoePe - Changed to use QI for IID_IAsyncMoniker
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI IsAsyncMoniker(IMoniker* pmk)
{
    PerfDbgLog1(tagUrlApi, NULL, "+IsAsyncMoniker(%lx)", pmk);
    HRESULT hr = NOERROR;

    if (pmk)
    {
        IUnknown *punk;
        hr = pmk->QueryInterface(IID_IAsyncMoniker, (void**)&punk);
        if (hr == S_OK)
        {
            punk->Release();
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog2(tagUrlApi, NULL, "-IsAsyncMoniker(%lx, hr:%lx)",pmk, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterBindStatusCallback
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [pBSCb] --
//              [reserved] --
//
//  Returns:
//
//  History:    12-13-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI RegisterBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb,IBindStatusCallback **ppBSCBPrev, DWORD reserved)
{
    HRESULT hr;
    PerfDbgLog2(tagUrlApi, NULL, "+RegisterBindStatusCallback(pBC:%lx, pBSCb:%lx)",pBC,pBSCb);

    if (ppBSCBPrev)
    {
        *ppBSCBPrev = NULL;
    }

    // Register the IBindStatusCallback in the bind context.
    if (pBSCb != NULL && pBC != NULL)
    {
        CBSCHolder *pCBSCHolder;
        IBindStatusCallback *pBSCBUnused = NULL;

        // Note: get the previous register IBSC - this
        //       might be actual a marshaled object
        //
        if (ppBSCBPrev)
        {
            // ask for the IBSC and NOT the holder since the holder does NOT get marshaled
            hr = GetObjectParam(pBC, REG_BSCB_HOLDER, IID_IBindStatusCallback, (IUnknown **)ppBSCBPrev);
            PerfDbgLog1(tagUrlApi, NULL, "=== RegisterBindStatusCallback (pBSCBPrev:%lx)",*ppBSCBPrev);
        }

        hr = GetBSCHolder(pBC, &pCBSCHolder);
        if (hr == NOERROR)
        {
            hr = pCBSCHolder->SetMainNode(pBSCb, &pBSCBUnused);
            pCBSCHolder->Release();
        }

        if (pBSCBUnused)
        {
            if (ppBSCBPrev && *ppBSCBPrev)
            {
                (*ppBSCBPrev)->Release();
            }

            if (ppBSCBPrev)
            {
                *ppBSCBPrev = pBSCBUnused;
            }
            else
            {
                pBSCBUnused->Release();
            }
        }
    }
    else
    {
        UrlMkAssert((pBSCb != NULL && pBC != NULL && "Invalid argument passed in RegisterBindStatusCallback"));
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagUrlApi, NULL, "-RegisterBindStatusCallback(hr:%lx)",hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RevokeBindStatusCallback
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [pBSCb] --
//              [reserved] --
//
//  Returns:
//
//  History:    12-13-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI RevokeBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb)
{
    HRESULT hr;
    PerfDbgLog2(tagUrlApi, NULL, "+RevokeBindStatusCallback(pBC:%lx, pBSCb:%lx)",pBC,pBSCb);
    CBSCHolder *pCBSCHolder;

    if (pBSCb != NULL && pBC != NULL)
    {
        hr = GetBSCHolder(pBC, &pCBSCHolder);
        if (hr == NOERROR)
        {
            hr = pCBSCHolder->RemoveNode(pBSCb);
            if (hr == S_FALSE)
            {
                // remove the holder from this bind context
                // the holder will be deleted since by the
                // last release
                PerfDbgLog2(tagUrlApi, NULL, "===  RevokeBindStatusCallback Revoke Holder Start (pBndCtx:%lx, -> %lx)",pBC, pBSCb);
                hr = pBC->RevokeObjectParam(REG_BSCB_HOLDER);
                PerfDbgLog2(tagUrlApi, NULL, "===  RevokeBindStatusCallback Revoke Holder Done (pBndCtx:%lx, -> %lx)",pBC, pBSCb);
            }
            else
            {
                hr = NOERROR;
            }
            pCBSCHolder->Release();
        }
    }
    else
    {
        UrlMkAssert((pBSCb != NULL && pBC != NULL && "Invalid argument passed in RevokeBindStatusCallback"));
        hr = E_INVALIDARG;
    }

    PerfDbgLog2(tagUrlApi, NULL, "-RevokeBindStatusCallback(%lx, hr:%lx)",pBSCb, hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetClassURL
//
//  Synopsis:
//
//  Arguments:  [szURL] --
//              [pClsID] --
//
//  Returns:
//
//
//  Notes:      BUGBUG: do we have to implement this api? Is it really needed?
//
//----------------------------------------------------------------------------
STDAPI GetClassURL(LPCWSTR szURL, CLSID *pClsID)
{
    HRESULT hr = E_NOTIMPL;
    PerfDbgLog(tagUrlApi, NULL, "+GetClassURL");

    PerfDbgLog(tagUrlApi, NULL, "-GetClassURL");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterMediaTypesW
//
//  Synopsis:   registers a media types for the current apartment
//
//  Arguments:  [ctypes] --
//              [rgszTypes] --
//              [rgcfTypes] --
//
//  Returns:
//
//  History:    1-20-96   JohannP (Johann Posch)   Created
//
//  Notes:      Media types are registered on apartment level
//
//----------------------------------------------------------------------------
HRESULT RegisterMediaTypesW(UINT ctypes, const LPCWSTR* rgszTypes, CLIPFORMAT* rgcfTypes)
{
    HRESULT hr = E_NOTIMPL;
    PerfDbgLog(tagUrlApi, NULL, "+RegisterMediaTypesW");
    CMediaTypeHolder *pCMHolder;
    CLock lck(g_mxsMedia);

#ifdef UNUSED
    pCMHolder = GetMediaTypeHolder();

    if (pCMHolder)
    {
        hr = pCMHolder->Register(ctypes, rgszTypes, rgcfTypes);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
#endif //UNUSED

    PerfDbgLog(tagUrlApi, NULL, "-RegisterMediaTypesW");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterMediaTypes
//
//  Synopsis:   registers a media types for the current apartment
//
//  Arguments:  [ctypes] --
//              [rgszTypes] --
//              [rgcfTypes] --
//
//  Returns:
//
//  History:    1-20-96   JohannP (Johann Posch)   Created
//
//  Notes:      Media types are registered on apartment level
//
//----------------------------------------------------------------------------
HRESULT RegisterMediaTypes(UINT ctypes, const LPCSTR* rgszTypes, CLIPFORMAT* rgcfTypes)
{
    HRESULT hr;
    PerfDbgLog(tagUrlApi, NULL, "+RegisterMediaTypes");
    CMediaTypeHolder *pCMHolder;
    CLock lck(g_mxsMedia);

    if (ctypes > 0)
    {
        pCMHolder = GetMediaTypeHolder();

        if (pCMHolder)
        {
            hr = pCMHolder->Register(ctypes, rgszTypes, rgcfTypes);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }


    PerfDbgLog(tagUrlApi, NULL, "-RegisterMediaTypes");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterMediaTypeClass
//
//  Synopsis:
//
//  Arguments:  [UINT] --
//              [ctypes] --
//              [rgszTypes] --
//              [rgclsID] --
//              [reserved] --
//
//  Returns:
//
//  History:    3-26-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI RegisterMediaTypeClass(LPBC pBC,UINT ctypes,  const LPCSTR* rgszTypes, CLSID *rgclsID, DWORD reserved)
{
    HRESULT hr = E_FAIL;
    PerfDbgLog(tagUrlApi, NULL, "+RegisterMediaTypeClass");
    IMediaHolder *pIMHolder = NULL;

    if (ctypes > 0)
    {
        hr = GetObjectParam(pBC, REG_MEDIA_HOLDER, IID_IMediaHolder, (IUnknown**)&pIMHolder);

        if (pIMHolder == NULL)
        {
            pIMHolder = new CMediaTypeHolder();
            if (pIMHolder)
            {
                hr = pBC->RegisterObjectParam(REG_MEDIA_HOLDER, pIMHolder);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (hr == NOERROR)
    {
        UrlMkAssert((pIMHolder));
        hr = pIMHolder->RegisterClassMapping(ctypes,(const char **) rgszTypes, rgclsID, 0);
    }

    if (pIMHolder)
    {
        pIMHolder->Release();
    }

    PerfDbgLog(tagUrlApi, NULL, "-RegisterMediaTypeClass");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindMediaTypeClass
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [pszType] --
//              [pclsID] --
//              [reserved] --
//
//  Returns:
//
//  History:    3-26-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI FindMediaTypeClass(LPBC pBC, LPCSTR pszType, CLSID *pclsID, DWORD reserved)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagUrlApi, NULL, "+FindMediaTypeClass");
    IMediaHolder *pIMHolder;
    CLIPFORMAT cfTypes = CF_NULL;

    TransAssert((pclsID));
    *pclsID = CLSID_NULL;

    cfTypes = (CLIPFORMAT) RegisterClipboardFormat(pszType);
    if (cfTypes != CF_NULL)
    {
        hr = GetObjectParam(pBC, REG_MEDIA_HOLDER, IID_IMediaHolder, (IUnknown**)&pIMHolder);

        if (pIMHolder)
        {
            hr = pIMHolder->FindClassMapping(pszType, pclsID, 0);
            pIMHolder->Release();
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog1(tagUrlApi, NULL, "-FindMediaTypeClass (hr:%lx)", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateFormatEnumerator
//
//  Synopsis:
//
//  Arguments:  [cfmtetc] --
//              [rgfmtetc] --
//              [ppenumfmtetc] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CreateFormatEnumerator( UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc)
{
    HRESULT hr = NOERROR;
    PerfDbgLog(tagUrlApi, NULL, "+CreateFormatEnumerator");

    if (ppenumfmtetc != NULL)
    {
        CEnumFmtEtc *pCEnum;
        pCEnum = CEnumFmtEtc::Create(cfmtetc, rgfmtetc);
        if (pCEnum)
        {
            *ppenumfmtetc = (IEnumFORMATETC *)pCEnum;
        }
        else
        {
            *ppenumfmtetc = NULL;
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    PerfDbgLog(tagUrlApi, NULL, "-CreateFormatEnumerator");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   BindAsyncMoniker
//
//  Synopsis:
//
//  Arguments:  [pmk] --
//              [grfOpt] --
//              [iidResult] --
//              [ppvResult] --
//
//  Returns:
//
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI BindAsyncMoniker (LPMONIKER pmk, DWORD grfOpt, IBindStatusCallback *pIBSCb, REFIID iidResult, LPVOID FAR* ppvResult)
{
    VDATEPTROUT(ppvResult,LPVOID);
    *ppvResult = NULL;
    VDATEIFACE(pmk);
    VDATEIID(iidResult);

    LPBC pbc = NULL;
    HRESULT hr = E_INVALIDARG;
    PerfDbgLog1(tagUrlApi, NULL, "+BindAsyncMoniker(%lx)",pmk);

    if (pmk)
    {
        hr = CreateAsyncBindCtx(0, pIBSCb, NULL, &pbc);
        if (hr == NOERROR)
        {
            hr = pmk->BindToObject(pbc, NULL, iidResult, ppvResult);
            pbc->Release();

        }
    }

    PerfDbgLog1(tagUrlApi, NULL, "-BindAsyncMoniker(%lx)",pmk);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegisterFormatEnumerator
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [pEFetc] --
//              [reserved] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved)
{
    HRESULT hr = E_INVALIDARG;
    PerfDbgLog1(tagUrlApi, NULL, "+RegisterFormatEnumerator(%lx)",pBC);

    if (pBC)
    {
        hr = pBC->RegisterObjectParam(REG_ENUMFORMATETC, pEFetc);
    }

    PerfDbgLog1(tagUrlApi, NULL, "-RegisterFormatEnumerator(%lx)",pBC);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RevokeFormatEnumerator
//
//  Synopsis:
//
//  Arguments:  [pBC] --
//              [pEFetc] --
//
//  Returns:
//
//  History:    1-19-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI RevokeFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc)
{
    HRESULT hr = E_INVALIDARG;
    PerfDbgLog1(tagUrlApi, NULL, "+RevokeFormatEnumerator(%lx)",pBC);

    if (pBC)
    {
        hr = pBC->RevokeObjectParam(REG_ENUMFORMATETC);
    }

    PerfDbgLog1(tagUrlApi, NULL, "-RevokeFormatEnumerator(%lx)",pBC);
    return hr;
}
