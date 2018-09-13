//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       rsctrl.cxx
//
//  Contents:   Implementation of Datadoc OLE Control site that works
//              without a control
//
//  Classes:    COleDataSite::CRSControl
//
//  Maintained by LaszloG
//
//----------------------------------------------------------------------------


#define _OLEDSITE_CXX_   1

#include "headers.hxx"
#include "dfrm.hxx"

//DeclareTag(tagOleDataSite,"src\\ddoc\\datadoc\\oledsite.cxx","Ole Data Site");
//DeclareTag(tagDataBind,"olecfrm.cxx","IDispatch ONLY");


//  dummy CServer class descriptor for the fake null control
CServer::CLASSDESC COleDataSite::s_sclassdescDummyControl =
{
    {                                            // _classdescBase
        &CLSID_CFakeControl,                     // _pclsid
        IDR_BASE_FORM,                           // _idrBase
        NULL,                                    // _apClsidPages
        0,                                       // _ccp
        NULL,                                    // _pcpi
        0,                                       // _cpropdesc
        NULL,                                    // _ppropdesc
        0,                                       // _dwFlags
        &IID_IControlElement,                           // _piidDispinterface
    },
    0,                                           // _dwMiscStatus
    0,                                           // _dwViewStatus
    0,                                           // _cOleVerbTable
    NULL,                                        // _pOleVerbTable
    NULL,                                        // _pfnDoVerb
    0,                                           // _cGetFmtTable
    NULL,                                        // _pGetFmtTable
    NULL,                                        // _pGetFuncs
    0,                                           // _cSetFmtTable
    NULL,                                        // _pSetFmtTable
    NULL,                                        // _pSetFuncs
    0,                                           // _ibItfPrimary
    DISPID_UNKNOWN,                              // _dispidRowset
    0,                                           // _wVFFlags
    DISPID_UNKNOWN,                              // _dispIDBind
    ~0UL,                                        // _uGetBindIndex
    ~0UL,                                        // _uPutBindIndex
    VT_EMPTY,                                    // _vtBindType
    ~0UL,                                        // _uGetValueIndex
    ~0UL,                                        // _uPutValueIndex
    VT_EMPTY,                                    // _vtValueType
    ~0UL,                                        // _uSetRowset
    SEF_NONE                                     // _sef
};



//+---------------------------------------------------------------------------
//
//      IPersistStreamInit Implementation (tear-off)
//
//----------------------------------------------------------------------------


BEGIN_TEAROFF_TABLE(COleDataSite::CRSControl, IPersistStreamInit)
    TEAROFF_METHOD(CRSControl::GetClassID, (LPCLSID lpClassID))
    TEAROFF_METHOD(CRSControl::IsDirty, (void))
    TEAROFF_METHOD(CRSControl::Load, (LPSTREAM pStm))
    TEAROFF_METHOD(CRSControl::Save, (LPSTREAM pStm, BOOL fClearDirty))
    TEAROFF_METHOD(CRSControl::GetSizeMax, (ULARGE_INTEGER FAR * pcbSize))
    TEAROFF_METHOD(CRSControl::InitNew, (void))
END_TEAROFF_TABLE()





//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CRSControl::AddRef, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

ULONG
COleDataSite::CRSControl::AddRef()
{
    return MyOleDataSite()->SubAddRef();
}


//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CRSControl::Release, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

ULONG
COleDataSite::CRSControl::Release()
{
    return MyOleDataSite()->SubRelease();
}


//+---------------------------------------------------------------------------
//
//  Member:     COleDataSite::CRSControl::QueryInterface, IUnknown
//
//  Synopsis:   Private IUnknown implementation.
//
//----------------------------------------------------------------------------

HRESULT
COleDataSite::CRSControl::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF(this, IPersistStreamInit, this)
        QI_INHERITS(this, IOleObject)
    }

    if (!*ppv)
    {
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    //*ppv = WATCHINTERFACE(iid, *ppv, "COleDataSite::CRSControl");

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}




//+---------------------------------------------------------------------------
//
// *** IPersist methods ***
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleDataSite::CRSControl::GetClassID (LPCLSID lpClassID)
{
    if ( ! lpClassID )
        return E_INVALIDARG;

    *lpClassID = CLSID_CFakeControl;
    return S_OK;
}





//+---------------------------------------------------------------------------
//
// *** IPersistStreamInit methods **
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleDataSite::CRSControl::IsDirty (void)
{
    return S_FALSE;
}



STDMETHODIMP
COleDataSite::CRSControl::Load (LPSTREAM pStm)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::Save (LPSTREAM pStm, BOOL fClearDirty)
{
    return S_OK;
}



//
//  The extents of the site is saved/loaded by the site
//
STDMETHODIMP
COleDataSite::CRSControl::GetSizeMax (ULARGE_INTEGER FAR * pcbSize)
{
    if ( ! pcbSize )
        return E_INVALIDARG;

    pcbSize->LowPart = 0;
    pcbSize->HighPart = 0;
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::InitNew (void)
{
    return S_OK;
}







//+---------------------------------------------------------------------------
//
//      IOleObject Implementation
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleDataSite::CRSControl::SetClientSite (LPOLECLIENTSITE pClientSite)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::GetClientSite (LPOLECLIENTSITE FAR* ppClientSite)
{
    if ( ! ppClientSite )
        return E_INVALIDARG;

    *ppClientSite = NULL;
    return E_FAIL;
}



STDMETHODIMP
COleDataSite::CRSControl::SetHostNames (LPCTSTR szContainerApp, LPCTSTR szContainerObj)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::Close (DWORD dwSaveOption)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::SetMoniker (DWORD dwWhichMoniker, LPMONIKER pmk)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::GetMoniker (
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER FAR* ppmk)
{
    if ( ! ppmk )
        return E_INVALIDARG;

    *ppmk = NULL;
    return E_NOTIMPL;
}



STDMETHODIMP
COleDataSite::CRSControl::InitFromData (
        LPDATAOBJECT pDataObject,
        BOOL fCreation,
        DWORD dwReserved)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::GetClipboardData (
        DWORD dwReserved,
        LPDATAOBJECT FAR* ppDataObject)
{
    if ( ! ppDataObject )
        return E_INVALIDARG;

    *ppDataObject = NULL;
    return E_NOTIMPL;
}



STDMETHODIMP
COleDataSite::CRSControl::DoVerb (
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    return E_FAIL;
}



STDMETHODIMP
COleDataSite::CRSControl::EnumVerbs (LPENUMOLEVERB FAR* ppenumOleVerb)
{
    if ( ! ppenumOleVerb )
        return E_INVALIDARG;

    *ppenumOleVerb = NULL;
    return E_NOTIMPL;
}



STDMETHODIMP
COleDataSite::CRSControl::Update ()
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::IsUpToDate ()
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::GetUserClassID (CLSID FAR* pClsid)
{
    if ( ! pClsid )
        return E_INVALIDARG;

    * pClsid = CLSID_CFakeControl;
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::GetUserType (DWORD dwFormOfType, LPTSTR FAR* plpstr)
{
    if (plpstr == NULL || dwFormOfType < 1 || dwFormOfType > 3)
    {
        RRETURN(E_INVALIDARG);
    }

    RRETURN(TaskAllocString(_T("RecordSelector"), plpstr));
}



STDMETHODIMP
COleDataSite::CRSControl::SetExtent (DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    if ( ! lpsizel )
    {
        return E_INVALIDARG;
    }
    else if ( dwDrawAspect == DVASPECT_CONTENT )
    {
        MyOleDataSite()->_rcl.SetSize(*lpsizel);
        return S_OK;
    }
    else
    {
        return E_NOTIMPL;
    }
}



STDMETHODIMP
COleDataSite::CRSControl::GetExtent (DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    if ( ! lpsizel )
    {
        return E_INVALIDARG;
    }
    else if ( dwDrawAspect == DVASPECT_CONTENT )
    {
        *lpsizel = MyOleDataSite()->_rcl.Size();
        return S_OK;
    }
    else
    {
        return E_NOTIMPL;
    }
}






STDMETHODIMP
COleDataSite::CRSControl::Advise (IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)
{
    return E_NOTIMPL;
}



STDMETHODIMP
COleDataSite::CRSControl::Unadvise (DWORD dwConnection)
{
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::EnumAdvise (LPENUMSTATDATA FAR* ppenumAdvise)
{
    if ( ! ppenumAdvise )
        return E_INVALIDARG;

    *ppenumAdvise = NULL;
    return E_NOTIMPL;
}



STDMETHODIMP
COleDataSite::CRSControl::GetMiscStatus (DWORD dwAspect, DWORD FAR* pdwStatus)
{
    *pdwStatus = 0;
    return S_OK;
}



STDMETHODIMP
COleDataSite::CRSControl::SetColorScheme (LPLOGPALETTE lpLogpal)
{
    return S_OK;
}



//
//
//  end of file
//
//----------------------------------------------------------------------------
