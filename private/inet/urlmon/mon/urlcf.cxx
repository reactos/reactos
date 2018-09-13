//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       urlcf.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <mon.h>
#include <eapp.h>
#include "urlcf.hxx"
#ifndef unix
#include "..\eapp\protbase.hxx"
#include "..\trans\urlmk.hxx"
#include "..\trans\bindctx.hxx"
#include "..\trans\oinet.hxx"
#include "..\download\cdl.h"
#else
#include "../eapp/protbase.hxx"
#include "../trans/urlmk.hxx"
#include "../trans/bindctx.hxx"
#include "../trans/oinet.hxx"
#include "../download/cdl.h"
#endif /* unix */

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::Create
//
//  Synopsis:
//
//  Arguments:  [clsid] --
//              [ppCF] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CUrlClsFact::Create(REFCLSID clsid, CUrlClsFact **ppCF)
{
    UrlMkDebugOut((DEB_URLMON, "NULL _IN CUrlClsFact::Create\n"));
    HRESULT hr = NOERROR;
    CUrlClsFact * pCF =  NULL;

    DWORD dwId = IsKnownOInetProtocolClass( (CLSID*)&clsid );

    if (   (dwId != DLD_PROTOCOL_NONE)
        || (clsid == CLSID_StdURLMoniker)
        || (clsid == CLSID_UrlMkBindCtx)
        || (clsid == CLSID_StdURLProtocol)
        || (clsid == CLSID_SoftDistExt)
        || (clsid == CLSID_DeCompMimeFilter)
        || (clsid == CLSID_StdEncodingFilterFac)
        || (clsid == CLSID_ClassInstallFilter)
        || (clsid == CLSID_CdlProtocol)
        || (clsid == CLSID_InternetSecurityManager)
        || (clsid == CLSID_InternetZoneManager)
       )
    {
        pCF = (CUrlClsFact *) new CUrlClsFact(clsid, dwId);
    }

    if (pCF == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppCF = pCF;
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::Create (hr:%lx\n", pCF,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::CUrlClsFact
//
//  Synopsis:   constructor
//
//  Arguments:  [clsid] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:      we need to keep a refcount on the dll if for each object given to
//              outside, including ClassFactories.
//              The corresponding DllRelease is in the destructor
//
//----------------------------------------------------------------------------
CUrlClsFact::CUrlClsFact(REFCLSID clsid, DWORD dwId) : _CRefs(), _CLocks(0)
{
    _ClsID =  clsid;
    _dwId = dwId;

    DllAddRef();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::~CUrlClsFact
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CUrlClsFact::~CUrlClsFact()
{
    DllRelease();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::CreateInstance
//
//  Synopsis:   creates an instance of an Explode Object
//
//  Arguments:  [pUnkOuter] -- controlling unknown (must be NULL)
//              [riid] --      id of desired interface
//              [ppv] --       pointer to receive the interface
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:      S_OK - success
//              CLASS_E_NOAGGREATION - the caller tried to aggregate
//              CLASS_E_CLASSNOTAVAILABLE - couldn't initialize the class
//              E_OUTOFMEMORY - not enough memory to instantiate class
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppv)
{
    //UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::CreateInstance\n", this));
    HRESULT hr = NOERROR;
    // Class factory init time, the pointer to the creation function of
    // the object is given.  Use that to create the object

    //DumpIID(riid);
    //DumpIID(_rClsID);

    if (riid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *)this;
        AddRef();
    }
    else if (_dwId != DLD_PROTOCOL_NONE)
    {
        hr = CreateKnownProtocolInstance(_dwId, _ClsID, pUnkOuter, riid, (IUnknown **)ppv);
    }
    else if (_ClsID == CLSID_StdURLMoniker)
    {
        CUrlMon * pMnk = NULL;
        LPWSTR szUrl = NULL;

        if ((pMnk = new CUrlMon(szUrl)) != NULL)
        {
            hr = pMnk->QueryInterface(riid, ppv);
            pMnk->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }
    else if (_ClsID == CLSID_UrlMkBindCtx)
    {
        CBindCtx *pCBCtx = NULL;

        hr = CBindCtx::Create(&pCBCtx);

        if (hr == NOERROR)
        {
            TransAssert((pCBCtx));
            hr = pCBCtx->QueryInterface(riid, ppv);
            pCBCtx->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }
    else if (_ClsID == CLSID_StdURLProtocol)
    {
        // BUGBUG: add protocol here

    }
    else if (_ClsID == CLSID_InternetSecurityManager)
    {
        hr = InternetCreateSecurityManager(pUnkOuter, riid, ppv, 0);
    }
    else if (_ClsID == CLSID_InternetZoneManager)
    {
        hr = InternetCreateZoneManager(pUnkOuter, riid, ppv, 0);
    }
    else if (_ClsID == CLSID_SoftDistExt)
    {
        CSoftDist * pSoftDist = NULL;

        if ((pSoftDist = new CSoftDist()) != NULL)
        {
            hr = pSoftDist->QueryInterface(riid, ppv);
            pSoftDist->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }
    else if( _ClsID == CLSID_DeCompMimeFilter)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        if( pUnkOuter)
            hr = CLASS_E_NOAGGREGATION;
        else
        {
            CMimeFt*    pMft = NULL;
            hr = CMimeFt::Create(&pMft);
            if( (hr == NOERROR) && pMft )
            {
                hr = pMft->QueryInterface(riid, ppv);
                pMft->Release();
            }
        }
    }
    else if( _ClsID == CLSID_StdEncodingFilterFac)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        if( pUnkOuter)
            hr = CLASS_E_NOAGGREGATION;
        else
        {
            *ppv = NULL;
            *ppv = new CEncodingFilterFactory;
            if( *ppv == NULL )
                hr = E_OUTOFMEMORY;
            else
                hr = ((IEncodingFilterFactory*)(*ppv))->AddRef();                
        }
    
    }
    else if( _ClsID == CLSID_ClassInstallFilter)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        if (pUnkOuter)
            hr = CLASS_E_NOAGGREGATION;
        else
        {
            CClassInstallFilter *pCIF = NULL;
            pCIF = new CClassInstallFilter();
            *ppv = (LPVOID)(IOInetProtocol *)pCIF;
            if (!*ppv)
                hr = E_OUTOFMEMORY;
            else
                hr = S_OK;
        }
    }
    else if (_ClsID == CLSID_CdlProtocol)
    {
        hr = CreateAPP(_ClsID, pUnkOuter, riid, (IUnknown **)ppv);
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::CreateInstance (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::LockServer
//
//  Synopsis:   locks the server, preventing it from being unloaded
//
//  Arguments:  [fLock] -- TRUE to lock, FALSE to unlock
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::LockServer(BOOL fLock)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::LockServer\n", this));
    HRESULT hr = NOERROR;
    if (fLock)
    {
        if (++_CLocks == 1)
        {
            DllAddRef();
        }
    }
    else
    {
        UrlMkAssert((_CLocks > 0));
        if (_CLocks > 0)
        {
            if (--_CLocks == 0)
            {
                DllRelease();
            }
        }
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::LockServer (hr:%lx)\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUrlClsFact::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CUrlClsFact::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    HRESULT hr = NOERROR;
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::QueryInterface\n", this));

    if (   riid == IID_IUnknown
        || riid == IID_IClassFactory)
    {
        *ppvObj = this;
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

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlClsFact::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlClsFact::AddRef(void)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CUrlClsFact::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CUrlClsFact::Release(void)
{
    UrlMkDebugOut((DEB_URLMON, "%p _IN CUrlClsFact::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    UrlMkDebugOut((DEB_URLMON, "%p OUT CUrlClsFact::Release (cRefs:%ld)\n", this,lRet));
    return lRet;
}

#if DBG==1
HRESULT DumpIID(REFIID riid)
{

    HRESULT hr;
    LPOLESTR pszStr = NULL;
    hr = StringFromCLSID(riid, &pszStr);
    UrlMkDebugOut((DEB_BINDING, "API >>> DumpIID (riid:%ws) \n", pszStr));

    if (pszStr)
    {
        delete pszStr;
    }
    return hr;
}
#endif


#ifdef FOR_SHDOCVW
//+---------------------------------------------------------------------------
//
//  Function:   GetObjectOffFileMoniker
//
//  Synopsis:   tries to create on object using a file moniker
//              this is a work around for OLE
//
//  Arguments:  [pMnk] --
//              [REFIID] --
//              [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    3-10-96   JohannP (Johann Posch)   Created
//
//  Notes:      See comments inside function
//
//----------------------------------------------------------------------------
HRESULT GetObjectOffFileMoniker(IMoniker *pMnk, IBindCtx *pbc,REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    DWORD dwMnk;
    LPWSTR      wzFilename = NULL;
    CLSID       * pClsID = NULL;
    IUnknown    * pUnk = NULL;
    IPersistFile *pPersistFile = NULL;
    BIND_OPTS     bindopts;

    bindopts.cbStruct = sizeof(BIND_OPTS);
    hr = pbc->GetBindOptions(&bindopts);


    pMnk->IsSystemMoniker(&dwMnk);

    if (dwMnk != MKSYS_FILEMONIKER)
    {
        goto Done;
    }

    //get path and filename
    hr = pMnk->GetDisplayName(NULL, NULL, &wzFilename);
    if (wzFilename == NULL)
    {
        goto Done;
    }

    // try to find class
    hr = GetClassFile(wzFilename, pClsID);
    if (hr != NOERROR)
    {
        goto Done;
    }

    // create object
    hr = CoCreateInstance(*pClsID, NULL, CLSCTX_INPROC_SERVER |CLSCTX_LOCAL_SERVER,
                            riid, (void**)&pUnk);

    if (hr != NOERROR)
    {
        goto Done;
    }

    // ask for the IPersistFile interface
    hr = pUnk->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
    if (hr != NOERROR)
    {
        goto Done;
    }
    // load the object
    hr = pPersistFile->Load(wzFilename, bindopts.grfMode);

Done:
    if (hr != NOERROR)
    {
        if (pUnk)
        {
            pUnk->Release();
        }
    }
    else
    {
        *ppv = (void *)pUnk;
    }

    if (pPersistFile)
    {
        pPersistFile->Release();
    }
    if (pClsID)
    {
        delete pClsID;
    }
    if (wzFilename)
    {
        delete wzFilename;
    }

    return hr;
}
#endif //FOR_SHDOCVW



