#include "stdafx.h"
#pragma hdrstop

//#include "comcat.h"
#define _SFVIEWP_H_ // we just want the GUIDs
//#include "sfviewp.h" // this dll should really be rolled into shell32
//#include <hliface.h>
//#include "shlguid.h"
//#include "shfvocx.h"

//
// BUGBUG: Remove this #include by defining _pauto as IWebBrowserApp, just like
//  Explorer.exe.
//
// NUKE - After ATL Convertion #include "hlframe.h"



#define IPSMSG(psz)             TraceMsg(TF_CUSTOM1, "shfv IPS::%s called", (psz))
#define IPSMSG2(psz, hres)      TraceMsg(TF_CUSTOM1, "shfv IPS::%s %x", (psz), hres)
#define IPSMSG3(psz, hres, x, y) TraceMsg(TF_CUSTOM1,"shfv IPS::%s %x %d (%d)", (psz), hres, x, y)
#define CCDMSG(psz, punk)       TraceMsg(TF_CUSTOM1, "shfv %s called punk=%x", (psz), punk)


static const OLEVERB c_averbsSV[] = {
        { 0, NULL, 0, 0 }
    };
static const OLEVERB c_averbsDesignSV[] = {
        { 0, NULL, 0, 0 }
    };


__inline BOOL SHIsEqualIID(REFGUID rguid1, REFGUID rguid2)
{
    return !memcmp(&rguid1, &rguid2, sizeof(GUID));
}

CShellFolderViewOC::CShellFolderViewOC() : _dwdeeCookie(0)
//    CShellOcx(punkOuter, c_averbsSV, c_averbsDesignSV)
{
    TraceMsg(TF_CUSTOM1, "ctor CShellFolderViewOC %x", this);

    // postpone initialization until InitNew is called.
    // this lets us query the container for ambient properties.
    // if we ever do any slow initialization stuff, we won't do it when not needed.
    // the cost is an if in front of every interface which references our state (robustness).
    _pdee = new CDShellFolderViewEvents(this, SAFECAST(this, IDispatch *));
}


CShellFolderViewOC::~CShellFolderViewOC()
{
    TraceMsg(TF_CUSTOM1, "dtor CShellFolderViewOC %x", this);

    // Release dispatch from shell folder view if any...
    ATOMICRELEASE(_pdispFolderView);
    if (_pdee)
        delete _pdee;
}


// ATL maintainence functions


// *** IPersistStreamInit ***
/****************** Not needed because of ATL
HRESULT CShellFolderViewOC::Load(IStream *pstm)
{
    IPSMSG(TEXT("Load"));
    // Load _size
    return NOERROR;
}


HRESULT CShellFolderViewOC::Save(IStream *pstm, BOOL fClearDirty)
{
    IPSMSG(TEXT("Save"));

    return NOERROR;
}
Not needed because of ATL ******************/

HRESULT CShellFolderViewOC::InitNew(void)
{
    IPSMSG(TEXT("InitNew"));
    if (!_fInit)
        _fInit = TRUE;
    else
        return E_UNEXPECTED;

    return IPersistStreamInitImpl<CShellFolderViewOC>::InitNew();
}

// *** IPersistPropertyBag ***
HRESULT CShellFolderViewOC::Load(IPropertyBag *pBag, IErrorLog *pErrorLog)
{
    IPSMSG(TEXT("Load PropertyBag"));

    // It is illegal to call ::Load or ::InitNew more than once
    if (_fInit)
    {
        TraceMsg(TF_CUSTOM1, "shv IPersistPropertyBag::Load called when ALREADY INITIALIZED!");
        ASSERT(FALSE);
        return(E_FAIL);
    }

    _fInit = TRUE;

    return IPersistPropertyBagImpl<CShellFolderViewOC>::Load(pBag, pErrorLog);
}


HRESULT CShellFolderViewOC::Save(IPropertyBag *pBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    IPSMSG(TEXT("Save PropertyBag"));

    return IPersistPropertyBagImpl<CShellFolderViewOC>::Save(pBag, fClearDirty, fSaveAllProperties);
}



// *** IFolderViewOC methods ***
// Properties and Methods we implement
HRESULT CShellFolderViewOC::SetFolderView(IDispatch *pDisp)
{
    HRESULT hres = S_OK;

    // first see if we already have a connection point we are holding on to.
    _ReleaseForwarder();

    IUnknown_Set((IUnknown **) &_pdispFolderView, (IUnknown *) pDisp);
    if (pDisp)
        hres = _SetupForwarder();

    return hres;
}

HRESULT CShellFolderViewOC::_SetupForwarder()
{
    if (!_pdee)
        return E_FAIL;

    return ConnectToConnectionPoint(SAFECAST(_pdee, IDispatch *), DIID_DShellFolderViewEvents, TRUE, _pdispFolderView, &_dwdeeCookie, NULL);
}

void CShellFolderViewOC::_ReleaseForwarder()
{
    ConnectToConnectionPoint(NULL, DIID_DShellFolderViewEvents, FALSE, _pdispFolderView, &_dwdeeCookie, NULL);
}


// ATL maintainence functions
LRESULT CShellFolderViewOC::_ReleaseForwarderMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
    bHandled = FALSE;
    _ReleaseForwarder();

    return 0;
}



/* IUnknown methods */
STDMETHODIMP CDShellFolderViewEvents::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if (SHIsEqualIID(riid, IID_IDispatch) || 
        SHIsEqualIID(riid, DIID_DShellFolderViewEvents) || 
        SHIsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (DShellFolderViewEvents *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG CDShellFolderViewEvents::AddRef()
{
    return _psfvOC->CComObjectRootEx<CComSingleThreadModel>::InternalAddRef();
}
ULONG CDShellFolderViewEvents::Release()
{
    return _psfvOC->CComObjectRootEx<CComSingleThreadModel>::InternalRelease();
}

STDMETHODIMP  CDShellFolderViewEvents::GetTypeInfoCount(UINT FAR* pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDShellFolderViewEvents::GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo FAR* FAR* pptinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDShellFolderViewEvents::GetIDsOfNames(REFIID riid,OLECHAR FAR* FAR* rgszNames,UINT cNames,
      LCID lcid, DISPID FAR* rgdispid)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDShellFolderViewEvents::Invoke(DISPID dispidMember,REFIID riid, LCID lcid, WORD wFlags,
          DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr)
{
    HRESULT hr = Invoke_OnConnectionPointerContainer(_punk, DIID_DShellFolderViewEvents, dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);

    return hr;
}

