//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       scriptlet.cxx
//
//  History:    19-Jan-1998     sramani     Created
//
//  Contents:   CScriptlet implementation
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_SCRPTLET_HXX_
#define X_SCRPTLET_HXX_
#include "scrptlet.hxx"
#endif

#ifndef X_SCRPCTRL_HXX_
#define X_SCRPCTRL_HXX_
#include "scrpctrl.hxx"
#endif

#ifndef X_SCRSBOBJ_HXX_
#define X_SCRSBOBJ_HXX_
#include "scrsbobj.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EVENTOBJ_H_
#define X_EVENTOBJ_H_
#include "eventobj.h"
#endif

extern class CBaseLockCF g_cfDoc;

// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);

#define _cxx_
#include "scrptlet.hdl"

MtDefine(Scriptlet, Mem, "Scriptlet")
MtDefine(CScriptlet, Scriptlet, "CScriptlet")
MtDefine(CScriptlet_aryDR_pv, CScriptlet, "CScriptlet::_aryDR::_pv")
MtDefine(CSortedAtomTable, Scriptlet, "CSortedAtomTable")
MtDefine(CSortedAtomTable_pv, CSortedAtomTable, "CSortedAtomTable::_pv")
MtDefine(CSortedAtomTable_aryIndex_pv, CSortedAtomTable, "CSortedAtomTable::_aryIndex::_pv")

BEGIN_TEAROFF_TABLE(CScriptlet, IOleObject)
    TEAROFF_METHOD(CScriptlet, SetClientSite, setclientsite, (LPOLECLIENTSITE pClientSite))
    TEAROFF_METHOD(CScriptlet, GetClientSite, getclientsite, (LPOLECLIENTSITE FAR* ppClientSite))
    TEAROFF_METHOD(CScriptlet, SetHostNames, sethostnames, (LPCTSTR szContainerApp, LPCTSTR szContainerObj))
    TEAROFF_METHOD(CScriptlet, Close, close, (DWORD dwSaveOption))
    TEAROFF_METHOD(CScriptlet, SetMoniker, setmoniker, (DWORD dwWhichMoniker, LPMONIKER pmk))
    TEAROFF_METHOD(CScriptlet, GetMoniker, getmoniker, (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk))
    TEAROFF_METHOD(CScriptlet, InitFromData, initfromdata, (LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved))
    TEAROFF_METHOD(CScriptlet, GetClipboardData, getclipboarddata, (DWORD dwReserved, LPDATAOBJECT * ppDataObject))
    TEAROFF_METHOD(CScriptlet, DoVerb, doverb, (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex, HWND hwndParent, LPCOLERECT lprcPosRect))
    TEAROFF_METHOD(CScriptlet, EnumVerbs, enumverbs, (LPENUMOLEVERB FAR* ppenumOleVerb))
    TEAROFF_METHOD(CScriptlet, Update, update, ())
    TEAROFF_METHOD(CScriptlet, IsUpToDate, isuptodate, ())
    TEAROFF_METHOD(CScriptlet, GetUserClassID, getuserclassid, (CLSID FAR* pClsid))
    TEAROFF_METHOD(CScriptlet, GetUserType, getusertype, (DWORD dwFormOfType, LPTSTR FAR* pszUserType))
    TEAROFF_METHOD(CScriptlet, SetExtent, setextent, (DWORD dwDrawAspect, LPSIZEL lpsizel))
    TEAROFF_METHOD(CScriptlet, GetExtent, getextent, (DWORD dwDrawAspect, LPSIZEL lpsizel))
    TEAROFF_METHOD(CScriptlet, Advise, advise, (IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection))
    TEAROFF_METHOD(CScriptlet, Unadvise, unadvise, (DWORD dwConnection))
    TEAROFF_METHOD(CScriptlet, EnumAdvise, enumadvise, (LPENUMSTATDATA FAR* ppenumAdvise))
    TEAROFF_METHOD(CScriptlet, GetMiscStatus, getmiscstatus, (DWORD dwAspect, DWORD FAR* pdwStatus))
    TEAROFF_METHOD(CScriptlet, SetColorScheme, setcolorscheme, (LPLOGPALETTE lpLogpal))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CScriptlet, IOleControl)
    TEAROFF_METHOD(CScriptlet, GetControlInfo, getcontrolinfo, (CONTROLINFO * pCI))
    TEAROFF_METHOD(CScriptlet, OnMnemonic, onmnemonic, (LPMSG pMsg))
    TEAROFF_METHOD(CScriptlet, OnAmbientPropertyChange, onambientpropertychange, (DISPID dispid))
    TEAROFF_METHOD(CScriptlet, FreezeEvents, freezeevents, (BOOL fFreeze))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CScriptlet, IOleInPlaceObject)
    TEAROFF_METHOD(CScriptlet, GetWindow, getwindow, (HWND*))
    TEAROFF_METHOD(CScriptlet, ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode))
    TEAROFF_METHOD(CScriptlet, InPlaceDeactivate, inplacedeactivate, ())
    TEAROFF_METHOD(CScriptlet, UIDeactivate, uideactivate, ())
    TEAROFF_METHOD(CScriptlet, SetObjectRects, setobjectrects, (LPCRECT prcPos, LPCRECT prcClip))
    TEAROFF_METHOD(CScriptlet, ReactivateAndUndo, reactivateandundo, ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CScriptlet, IPersistPropertyBag)
    TEAROFF_METHOD(CScriptlet, GetClassID, getclassid, (LPCLSID lpClassID))
    TEAROFF_METHOD(CScriptlet, InitNew, initnew, ())
    TEAROFF_METHOD(CScriptlet, Load, LOAD, (LPPROPERTYBAG pBag, LPERRORLOG pErrLog))
    TEAROFF_METHOD(CScriptlet, Save, SAVE, (LPPROPERTYBAG pBag, BOOL fClearDirty, BOOL fSaveAllProperties))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CScriptlet, IPersistStreamInit)
    TEAROFF_METHOD(CScriptlet, GetClassID, getclassid, (LPCLSID lpClassID))
    TEAROFF_METHOD(CScriptlet, IsDirty, isdirty, ())
    TEAROFF_METHOD(CScriptlet, Load, LOAD, (LPSTREAM pStm))
    TEAROFF_METHOD(CScriptlet, Save, SAVE, (LPSTREAM pStm, BOOL fClearDirty))
    TEAROFF_METHOD(CScriptlet, GetSizeMax, GETSIZEMAX, (ULARGE_INTEGER FAR * pcbSize))
    TEAROFF_METHOD(CScriptlet, InitNew, initnew, ())
END_TEAROFF_TABLE()

/////////////////////////////////////////////////////////////////////////////
// CScriptlet

const CONNECTION_POINT_INFO CScriptlet::s_acpi[] =
{
    CPI_ENTRY(IID_IPropertyNotifySink, DISPID_A_PROPNOTIFYSINK)
    CPI_ENTRY(DIID_DWebBridgeEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)
    CPI_ENTRY_NULL
};

const CBase::CLASSDESC CScriptlet::s_classdesc =
{
    &CLSID_Scriptlet,               // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IWebBridge,                // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+------------------------------------------------------------------------
//
//  Member:     CreateScriptlet
//
//  Synopsis:   Creates a new scriptlet doc instance.
//
//  Arguments:  pUnkOuter   Outer unknown
//
//-------------------------------------------------------------------------

CBase * STDMETHODCALLTYPE
CreateScriptlet(IUnknown *pUnkOuter)
{
    CBase *pBase;
    pBase = new CScriptlet(pUnkOuter);
    return(pBase);
}

CScriptlet::CScriptlet(IUnknown *pUnkOuter)
    : _aryDR(Mt(CScriptlet_aryDR_pv))
{
    _pUnkOuter  = pUnkOuter ? pUnkOuter : PunkInner();
    VariantInit(&_varOnVisChange);
    _dispidCur = DISPID_VECTOR_BASE;
}

CScriptlet::~CScriptlet()
{
    _cstrUrl.Free();
    VariantClear(&_varOnVisChange);
    _aryDR.DeleteAll();
    _aryDispid.Free();
}

void CScriptlet::Passivate()
{
    ReleaseInterface(_pScriptCtrl);
    ReleaseInterface(_pDescription);
// BUGBUG: ***TLL*** Tempory disable for cached HTMLElement
//    ReleaseInterface(_pHTMLElement);

    super::Passivate();

    // (sramani) Trident will rel its propnotify sink when its attr array is destroyed in the destructor thus
    // subrel'ing us, causing the scriptlet to be destroyed.
    // This will also rel the doc host handler associated with this scriptlet!
    ReleaseInterface(_pTrident); 
}

HRESULT
CScriptlet::Init()
{
    HRESULT hr;

    // Create And Aggregate Trident
    hr = THR(g_cfDoc.CreateInstance(_pUnkOuter, IID_IUnknown, (void**)&_pTrident));
    if (hr)
        goto Cleanup;

    hr = _pTrident->QueryInterface(CLSID_HTMLDocument, (void **)&_pDoc);
    if (hr)
        goto Cleanup;

    _pDoc->_fDelegateWindowOM = TRUE;
    
    hr = E_OUTOFMEMORY;

    _pScriptCtrl = new CScriptControl(this);
    if (!_pScriptCtrl)
        goto Cleanup;

    // (sramani) The doc holds a ref on to the doc host handler at SetDocHostUI time. This causes a subaddref on us,
    // the corresponding subrelease happening when the doc is destroyed. 
    _pDoc->SetUIHandler((IDocHostUIHandler *)&_ScriptletSubObjects);
    
    hr = ConnectSink(_pTrident, IID_IPropertyNotifySink, (IPropertyNotifySink *)&_ScriptletSubObjects, NULL);
    if (hr)
        goto Cleanup;
    // (sramani) Note:Trident now holds a subAddref on the scriptlet. This will be released when Trident destroys its attr
    // array on the passivate call of the scriptlet.

Cleanup:
    if (hr)
    {
        _pDoc->SetUIHandler(NULL);
        ReleaseInterface(_pScriptCtrl);
        ReleaseInterface(_pTrident);
    }
    return hr;
}

// IOleObject

HRESULT CScriptlet::SetClientSite(IOleClientSite *pClientSite)
{ 
    // (sramani) No need to addref this as the COleSite's clientsite will always outlive the 
    // scriptlet that it contains. This is so because the outer doc's olesite will hold on to the scriptlet.
    _pOCS = pClientSite;
    return _pDoc->SetClientSite(pClientSite);
}

HRESULT CScriptlet::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite* pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    BOOL fExtentSet = FALSE;

    // (sramani) Need to SetExtent as some container's like VB don't do so b4 inplace activating us.
    if (!_fExtentSet && (iVerb == OLEIVERB_INPLACEACTIVATE))
    {
        SIZEL sizel;
        GetExtent(DVASPECT_CONTENT, &sizel);
        fExtentSet = TRUE;
    }

    HRESULT hr = _pDoc->DoVerb(iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
    
    if (iVerb == OLEIVERB_INPLACEACTIVATE)
    {
        SIZEL sizeHiMetric;

        // BUGBUG(sramani): This will break VB as this is the only time this will be
        // called with an lprcPosRect, but we don't care, at least you will have the 
        // default size.
        if (!fExtentSet)
        {
            _sizePixExtent.cx = lprcPosRect->right - lprcPosRect->left;
            _sizePixExtent.cy = lprcPosRect->bottom - lprcPosRect->top;
            _fValidCx = _fValidCy = TRUE;
        }

        sizeHiMetric.cx = HimetricFromHPix(_sizePixExtent.cx);
        sizeHiMetric.cy = HimetricFromVPix(_sizePixExtent.cy);
        hr = SetExtent(DVASPECT_CONTENT, &sizeHiMetric);
        OnVisibilityChange();
    }
    return hr;
}

HRESULT CScriptlet::GetUserClassID(CLSID *pClsid)
{
    *pClsid = *BaseDesc()->_pclsid;
    return S_OK;
}

HRESULT CScriptlet::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    return OleRegGetUserType(*BaseDesc()->_pclsid, dwFormOfType, pszUserType);
}

HRESULT CScriptlet::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    HRESULT     hr;
    IHTMLStyle *pStyle = NULL;

    _sizePixExtent.cx = HPixFromHimetric(psizel->cx);
    _sizePixExtent.cy = VPixFromHimetric(psizel->cy);
    _fValidCx = _fValidCy = TRUE;
    _fExtentSet = TRUE;

    hr = _pDoc->SetExtent(dwDrawAspect, psizel);
    if (hr)
	    goto Cleanup;

    hr = GetStyleProperty(&pStyle);
    if (hr)
        goto Cleanup;

    if (pStyle)
    {
        pStyle->put_pixelWidth(_sizePixExtent.cx);
        pStyle->put_pixelHeight(_sizePixExtent.cy);
    }

Cleanup:
    ReleaseInterface(pStyle);
    return hr;
}

HRESULT CScriptlet::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    if (!_fValidCx)
    {
        _sizePixExtent.cx = WEBBRIDGE_DEFAULT_WIDTH;
        _fValidCx = TRUE;
    }

    if (!_fValidCy)
    {
        _sizePixExtent.cy = WEBBRIDGE_DEFAULT_HEIGHT;
        _fValidCy = TRUE;
    }

    psizel->cx = HimetricFromHPix(_sizePixExtent.cx);
    psizel->cy = HimetricFromVPix(_sizePixExtent.cy);

    // (sramani) Need to SetExtent as some container's like VB don't do so b4 inplace activating us.
    if (!_fExtentSet)
    {
        _fExtentSet = TRUE;
        _pDoc->SetExtent(DVASPECT_CONTENT, psizel);
    }

    return S_OK;
}

// IOleControl

HRESULT CScriptlet::FreezeEvents(BOOL fFreeze)
{
    if (fFreeze)
        ++_cFreezes;
    else
        --_cFreezes;

    Assert(_cFreezes >= 0);
    if (!_cFreezes && _fDelayOnReadyStateFiring)
    {
        FireEvent(DISPID_READYSTATECHANGE, DISPID_UNKNOWN, (BYTE *)VTS_NONE);
        _fDelayOnReadyStateFiring = FALSE;
    }

    return _pDoc->FreezeEvents(fFreeze);
}

// IOleInPlaceObject

HRESULT CScriptlet::InPlaceDeactivate()
{
    HRESULT hr;
    hr = _pDoc->InPlaceDeactivate();
    OnVisibilityChange();
    return hr;
}

HRESULT CScriptlet::SetObjectRects(LPCOLERECT prcPos, LPCOLERECT prcClip)
{
    HRESULT hr;
    hr = _pDoc->SetObjectRects(prcPos, prcClip);
    OnVisibilityChange();
    return hr;
}

// Scriptlet helpers

HRESULT CScriptlet::FireEvent(DISPID dispidEvent, DISPID dispidProp, BYTE * pbTypes, ...)
{
    va_list valParms;
    HRESULT hr;
    IDispatch *pEventObj = NULL;

    va_start(valParms, pbTypes);

    if (!_pDoc->EnsureOmWindow())
    {
        // Get the eventObject.
        IGNORE_HR(_pDoc->_pOmWindow->get_event((IHTMLEventObj **)&pEventObj));
    }
    
    hr = THR(FireEventV(dispidEvent, dispidProp, pEventObj, NULL, pbTypes, valParms));

    ReleaseInterface(pEventObj);
    
    va_end(valParms);

    RRETURN(hr);
}

HRESULT
CScriptlet::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, _pUnkOuter)
        QI_TEAROFF(this, IWebBridge, _pUnkOuter)
        QI_TEAROFF(this, IOleObject, _pUnkOuter)
        QI_TEAROFF(this, IOleControl, _pUnkOuter)
        QI_TEAROFF(this, IOleInPlaceObject, _pUnkOuter)
        QI_TEAROFF(this, IPersistStreamInit, _pUnkOuter)
        QI_TEAROFF(this, IPersistPropertyBag, _pUnkOuter)
        QI_TEAROFF((CBase *)this, IProvideMultipleClassInfo, _pUnkOuter)
        QI_TEAROFF2((CBase *)this, IProvideClassInfo, IProvideMultipleClassInfo, _pUnkOuter)
        QI_TEAROFF2((CBase *)this, IProvideClassInfo2, IProvideMultipleClassInfo, _pUnkOuter)
        QI_CASE(IConnectionPointContainer)
            if (iid == IID_IConnectionPointContainer)
            {
                *((IConnectionPointContainer **)ppv) =
                        new CConnectionPointContainer(this, NULL);

                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);
            }
            break;

        // (sramani): For perf reasons, just the Data1 part is compared, assuming that neither CDoc
        // nor CServer returns the QI for an interface with the same Data1 part but different than
        // any of the ones in the QI_CASE list below.
        QI_CASE(IPersistMoniker)
        QI_CASE(IViewObject)
        QI_CASE(IViewObjectEx)
        QI_CASE(IViewObject2)
        QI_CASE(IOleInPlaceActiveObject)
        QI_CASE(IOleInPlaceObjectWindowless)
        QI_CASE(IOleWindow)
        QI_CASE(IQuickActivate)
        QI_CASE(IOleCommandTarget)
        QI_CASE(IObjectSafety)
        QI_CASE(IOleContainer)
        QI_CASE(IDataObject)
        QI_CASE(IHTMLDocument)
#if (SPVER >= 0x1)
        QI_CASE(IMarkupServices)
#endif
            // (sramani): This is equivalent to calling _pTrident->QI, but faster!
            RRETURN(_pDoc->PrivateQueryInterface(iid, ppv));

    default:
        return E_NOINTERFACE;
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

// IDispatchEx over-rides

STDMETHODIMP 
CScriptlet::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT                 hr;
    long                    lIndex;
    DispidRecord            dr;
    BSTR                    bstrCopy = NULL;
    CStr                    cstrName;

    // Check to see if the Names are implemented directly by the Scriptlet. If so, intercept them
    // immediately.

    hr = CBase::GetInternalDispID(bstrName, pid, grfdex);
    if (!hr)
        goto Cleanup;

    // Assume failure.
    *pid = DISPID_UNKNOWN;
    hr = DISP_E_UNKNOWNNAME;

    if (_pDoc->_readyState < READYSTATE_COMPLETE || !bstrName)
        goto Cleanup;

    if (!StrCmpNIC(_T("get_"), bstrName, 4) ||
        !StrCmpNIC(_T("put_"), bstrName, 4) ||
        !StrCmpNIC(_T("event_"), bstrName, 6))    // BUGBUG (sramani) : why shud this last check be there?
        goto Cleanup;

    // See if we've found this particular name before. 
    if (_aryDispid.Find(bstrName, pid, !!(grfdex & fdexNameCaseSensitive)))
    {
        *pid += DISPID_VECTOR_BASE;
        hr = S_OK;
        goto Cleanup;
    }

    // If these are NULL, there is no public_description object,
    // so we need to look for public_ prefixes.
    if (!_pDescription)
    {
        hr = _pDoc->EnsureOmWindow();
        if (hr)
            goto Cleanup;

        Assert(_pDoc->_pOmWindow);

        hr = cstrName.Set(_T("public_get_"));
        if (hr)
            goto Cleanup;
        hr = cstrName.Append(bstrName);
        if (hr)
            goto Cleanup;
        hr = cstrName.AllocBSTR(&bstrCopy);
        if (hr)
            goto Cleanup;

        hr = _pDoc->_pOmWindow->GetDispID(bstrCopy, grfdex & ~fdexNameEnsure, &dr.dispid_get);
        if (hr)
            dr.dispid_get = DISPID_UNKNOWN;

        bstrCopy[7] = _T('p');
        bstrCopy[8] = _T('u');
        hr = _pDoc->_pOmWindow->GetDispID(bstrCopy, grfdex & ~fdexNameEnsure, &dr.dispid_put);
        if (hr)
            dr.dispid_put = DISPID_UNKNOWN;

        // If neither a get or put method exists for this name, look for a bare name

        if (dr.dispid_get == DISPID_UNKNOWN &&
            dr.dispid_put == DISPID_UNKNOWN)
        {
            hr = cstrName.Set(_T("public_"));
            if (hr)
                goto Cleanup;
            hr = cstrName.Append(bstrName);
            if (hr)
                goto Cleanup;
            FormsFreeString(bstrCopy);
            hr = cstrName.AllocBSTR(&bstrCopy);
            if (hr)
                goto Cleanup;

            hr = _pDoc->_pOmWindow->GetDispID(bstrCopy, grfdex & ~fdexNameEnsure, &dr.dispidBare);
            if (hr)
                goto Cleanup;
        }
        else
            dr.dispidBare = DISPID_UNKNOWN;
    }
    else
    {
        hr = cstrName.Set(_T("get_"));
        if (hr)
            goto Cleanup;
        hr = cstrName.Append(bstrName);
        if (hr)
            goto Cleanup;
        hr = cstrName.AllocBSTR(&bstrCopy);
        if (hr)
            goto Cleanup;

        Assert(_pDescription);
        hr = _pDescription->GetIDsOfNames(IID_NULL, &bstrCopy, 1, LOCALE_USER_DEFAULT, &dr.dispid_get);
        if (hr)
            dr.dispid_get = DISPID_UNKNOWN;

        bstrCopy[0] = _T('p');
        bstrCopy[1] = _T('u');

        hr = _pDescription->GetIDsOfNames(IID_NULL, &bstrCopy, 1, LOCALE_USER_DEFAULT, &dr.dispid_put);
        if (hr)
            dr.dispid_put = DISPID_UNKNOWN;

        // If neither a get or put method exists for this name, look for a bare name

        if (dr.dispid_get == DISPID_UNKNOWN &&
            dr.dispid_put == DISPID_UNKNOWN)
        {
            hr = _pDescription->GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, &dr.dispidBare);
            if (hr)
                goto Cleanup;
        }
        else
            dr.dispidBare = DISPID_UNKNOWN;
    }

    // Otherwise, we got a keeper! 
    hr = _aryDR.AppendIndirect(&dr);
    if (hr)
        goto Cleanup;

    hr = _aryDispid.Insert(bstrName, *pid, &lIndex);
    if (hr)
    {
        _aryDR.Delete(_aryDR.Size() - 1);
        goto Cleanup;
    }

    Assert(lIndex == _aryDR.Size() - 1);

    _dispidCur = lIndex + DISPID_VECTOR_BASE + 1;
    Assert(_dispidCur < 0x7FFFFFF);
    *pid = lIndex + DISPID_VECTOR_BASE;

Cleanup:
    cstrName.Free();
    FormsFreeString(bstrCopy);
    return hr;
}

STDMETHODIMP
CScriptlet::InvokeEx(DISPID dispid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, IServiceProvider *pSrvProvider)
{
    HRESULT         hr;
    DispidRecord *  pDR;
    DISPPARAMS      dispparams;

    // (sramani) CScriptlet's own DISPIDs have first priority.

    if ((0 <= dispid && dispid < DISPID_VECTOR_BASE) ||
        dispid == DISPID_ABOUTBOX ||
        dispid == DISPID_IHTMLWINDOW2_EVENT)
    {
        return CBase::InvokeEx(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider);
    }

    // Garybu, Ramani, "Scriptlet should pass the ... dispids through to Trident document object."

    if (PassThruDISPID(dispid))
        return _pDoc->InvokeEx(dispid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider);

    if (dispid < 0)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if (dispid < _dispidCur)
    {
        pDR = _aryDR + (dispid - DISPID_VECTOR_BASE);

        // Check how they want to call this thing.

        if (wFlags & DISPATCH_PROPERTYGET)
        {
            // try function property get 
            Assert((wFlags & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF)) == 0);
            dispid = pDR->dispid_get;
            if (dispid == DISPID_UNKNOWN)
                dispid = pDR->dispidBare;               // could be member property get
            else
                wFlags = DISPATCH_METHOD;               // calling a get_XXX method.
        }
        else if (wFlags & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF))
        {
            // try function property put 
            dispid = pDR->dispid_put;
            if (dispid == DISPID_UNKNOWN)
                dispid = pDR->dispidBare;               // could be member property put
            else
            {
                wFlags = DISPATCH_METHOD;               // calling a put_XXX method.
                // (sramani) Nuke the named parameters!. This is reqd. as the original call
                // came in as a propput and the script engine passes a named param for it. but
                // the actual invoke is really a method call. The script engine will fail the
                // invoke if we dont do this as it will be unable to find a named param for
                // public_put_foo, as it thinks it really belongs to foo (which is how it was
                // called from script)
                dispparams = *pdispparams;
                dispparams.cNamedArgs = 0;
                pdispparams = &dispparams;
            }
        }
        else
        {
            // try method call
            dispid = pDR->dispidBare;
        }

        if (dispid == DISPID_UNKNOWN)
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }
         
        if (_pDescription)
        {
            hr = _pDescription->Invoke(dispid,  IID_NULL,  lcid,  wFlags,  pdispparams,  pvarResult,  pexcepinfo,  NULL);
        }
        else
        {
            Assert(_pDoc->_pOmWindow);
            hr = _pDoc->_pOmWindow->InvokeEx(dispid,  lcid,  wFlags,  pdispparams,  pvarResult,  pexcepinfo,  pSrvProvider);
        }
    }
    else
        hr = DISP_E_MEMBERNOTFOUND;

Cleanup:
    return hr;
}

BOOL
CScriptlet::PassThruDISPID(DISPID dispid)
{
    switch (dispid)
    {
        case    DISPID_SECURITYDOMAIN:
        case    DISPID_SECURITYCTX:
        case    DISPID_READYSTATE:
            return TRUE;

        default:
            return FALSE;
    }
}

BOOL
CScriptlet::InDesignMode()
{
    HRESULT    hr;
    VARIANT    var;
    IDispatch *pDispOCS = NULL;

    if (!_pOCS)
        return FALSE;

    hr = _pOCS->QueryInterface(IID_IDispatch, (void **)&pDispOCS);
    if (hr)
        return FALSE;

    hr = Property_get(pDispOCS, DISPID_AMBIENT_USERMODE, &var);

    ReleaseInterface(pDispOCS);
    return (hr == S_OK && var.vt == VT_BOOL && var.boolVal == VARIANT_FALSE);
}


// IWebBridge properties and methods //////////////////////////////////

STDMETHODIMP CScriptlet::get_URL(BSTR * pVal)
{
    return _pDoc->_cstrUrl.AllocBSTR(pVal);
}

STDMETHODIMP CScriptlet::put_URL(BSTR newVal)
{
    HRESULT hr;

    // We only expose this as a write property at design time.
    if (!InDesignMode() || _fHardWiredURL)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    _fRequiresSave = 1;
    hr = LoadScriptletURL(newVal);

Cleanup:
    return hr;
}

STDMETHODIMP
CScriptlet::get_Scrollbar(VARIANT_BOOL *pfShow)
{
    *pfShow = _vbScrollbar;
    return S_OK;
}

STDMETHODIMP
CScriptlet::put_Scrollbar(VARIANT_BOOL fShow)
{
    _vbScrollbar = fShow ? VARIANT_TRUE : VARIANT_FALSE;
    return _pDoc->OnAmbientPropertyChange(DISPID_UNKNOWN);
}

STDMETHODIMP
CScriptlet::get_embed(VARIANT_BOOL * pfEmbedded)
{
    // 'embed' is a design-time only property.
    if (!InDesignMode())
        return DISP_E_MEMBERNOTFOUND;

    *pfEmbedded = _vbEmbedded;
    return S_OK;
}

STDMETHODIMP
CScriptlet::put_embed(VARIANT_BOOL fEmbed)
{
    if (!InDesignMode())
        return DISP_E_MEMBERNOTFOUND;

    _vbEmbedded = fEmbed ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

STDMETHODIMP
CScriptlet::get_readyState(long * pReadyState)
{
    *pReadyState = _pDoc->_readyState;
    return S_OK;
}

// BUGBUG (sramani) : shud we consider returning IHTMLEventObj *?
STDMETHODIMP
CScriptlet::get_event(IDispatch **ppEvent)
{
    HRESULT          hr;
    IHTMLWindow2    *pHW = NULL;
    IHTMLEventObj   *pEO = NULL;

    *ppEvent = NULL;

    hr = _pDoc->get_parentWindow(&pHW);
    if (hr)
        goto Cleanup;

    hr = pHW->get_event(&pEO);
    if (hr)
        goto Cleanup;

    if (pEO)
    {
        *ppEvent = pEO;
        (*ppEvent)->AddRef();
    }

Cleanup:
    ReleaseInterface(pHW);
    ReleaseInterface(pEO);
    return hr;
}

STDMETHODIMP
CScriptlet::AboutBox()
{
    HRESULT hr;
    HWND    hwnd;
    RECT    rcPos;
    RECT    rcClip;
    OLEINPLACEFRAMEINFO oipfi;
    IOleInPlaceSite *pOIPS = NULL;
    IOleInPlaceFrame *pFrame = NULL;
    IOleInPlaceUIWindow *pDoc = NULL;

    // We only expose this as a method at design time.
    if (!InDesignMode())
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = _pOCS->QueryInterface(IID_IOleInPlaceSite, (void **) &pOIPS);
    if (hr)
        goto Cleanup;

    hr = pOIPS->GetWindowContext(&pFrame, &pDoc, &rcPos, &rcClip, &oipfi);
    if (hr)
        goto Cleanup;

    hr = pFrame->GetWindow(&hwnd);
    if (hr)
        goto Cleanup;
    
    pFrame->EnableModeless(FALSE);
    MessageBoxA(hwnd, "Copyright (C) 1997 Microsoft\r\nhttp://www.microsoft.com", "Scriptlet Component", MB_OK);
    pFrame->EnableModeless(TRUE);

Cleanup:
    ReleaseInterface(pOIPS);
    ReleaseInterface(pFrame);
    ReleaseInterface(pDoc);
    return hr;
}

HRESULT CScriptlet::LoadScriptletURL(TCHAR *pchUrl)
{
    CStr cstrSpecialURL;

    CStr cstrBlank;
    if (_fHardWiredURL && !pchUrl)
        pchUrl = _cstrUrl;
        
    if (!pchUrl || !_pOCS)
        return E_POINTER;

    if (!*pchUrl || WrapSpecialUrl(pchUrl, &cstrSpecialURL, _pDoc->_cstrUrl, FALSE, FALSE) != S_OK)
    {
        cstrBlank.Set(_T("about:blank"));
        pchUrl = cstrBlank;
    }

    HRESULT hr;

    IMoniker           *pMoniker = NULL;
    IServiceProvider   *pSP = NULL;
    IBindHost          *pBH = NULL;

    // Try getting the IBindHost service first.

    hr = _pOCS->QueryInterface(IID_IServiceProvider, (void **)&pSP);
    if (!hr)
    {
        hr = pSP->QueryService(SID_IBindHost, IID_IBindHost, (void **)&pBH);
        if (!hr)
        {
            hr = pBH->CreateMoniker(pchUrl, NULL, &pMoniker, 0);
        }
    }

    // If we failed to create the moniker that way, make  an absolute moniker.

    if (!pMoniker)
    {
        hr = CreateURLMoniker(NULL, pchUrl, &pMoniker);
    }

    if (!hr)
    {
        hr = _pDoc->Load(FALSE, pMoniker, NULL, 0);
    }

    ReleaseInterface(pMoniker);
    ReleaseInterface(pBH);
    ReleaseInterface(pSP);
    return hr;
}

HRESULT CScriptlet::GetStyleProperty(IHTMLStyle **ppHTMLStyle)
{
    HRESULT                 hr;
    VARIANT                 varArg1;
    VARIANT                 varArg2;
    IHTMLElementCollection *pHEC = NULL;
    IHTMLElementCollection *pHEC2 = NULL;
    IDispatch              *pDispHtmlElement = NULL;
    IDispatch              *pDispCollection = NULL;

    *ppHTMLStyle = NULL;
    
/* BUGBUG: ***TLL*** Temporary to avoid assert when tree blown away _pHTMLElement is still held onto
    if (!_pHTMLElement)
*/
    {
        hr = _pDoc->get_all(&pHEC);
        if (hr)
            goto Cleanup;

        varArg1.vt = VT_BSTR;
        hr = FormsAllocString(_T("HTML"), &varArg1.bstrVal);
        if (hr)
            goto Cleanup;

        hr = pHEC->tags(varArg1, &pDispCollection);
        if (hr)
            goto Cleanup;

        hr = pDispCollection->QueryInterface(IID_IHTMLElementCollection, (void **)&pHEC2);
        if (hr)
            goto Cleanup;

        VariantClear(&varArg1);
        varArg1.vt = VT_I4;
        varArg1.lVal = 0;
        varArg2.vt = VT_ERROR;
        hr = pHEC2->item(varArg1, varArg2, &pDispHtmlElement);
        if (hr)
            goto Cleanup;

        if (pDispHtmlElement)
        {
            hr = pDispHtmlElement->QueryInterface(IID_IHTMLElement, (void **)&_pHTMLElement);
            if (hr)
                goto Cleanup;
        }
        else
            goto Cleanup;
    }

    Assert(_pHTMLElement);
    hr = _pHTMLElement->get_style(ppHTMLStyle);

// BUGBUG: ***TLL*** Below line is temporary to disable cache of HTMLElement
ClearInterface(&_pHTMLElement);

Cleanup:
    ReleaseInterface(pHEC);
    ReleaseInterface(pHEC2);
    ReleaseInterface(pDispHtmlElement);
    ReleaseInterface(pDispCollection);
    return hr;
}

HRESULT CScriptlet::Resize()
{
    HRESULT                 hr = E_FAIL;
    RECT                    rcPos;
    RECT                    rcClip;
    OLEINPLACEFRAMEINFO     oipfi;
    IOleInPlaceSite        *pOIPS = NULL;
    IOleInPlaceFrame       *pOIPF = NULL;
    IOleInPlaceUIWindow    *pOIPUIW = NULL;

    if (!_pOCS)
        goto Cleanup;

    if (_pDoc->State() >= OS_INPLACE)
    {
        hr = _pOCS->QueryInterface(IID_IOleInPlaceSite, (void **)&pOIPS);
        if (hr)
            goto Cleanup;

        hr = pOIPS->GetWindowContext(&pOIPF, &pOIPUIW, &rcPos, &rcClip, &oipfi);
        if (hr)
            goto Cleanup;

        rcPos.right = rcPos.left + _sizePixExtent.cx;
        rcPos.bottom = rcPos.top + _sizePixExtent.cy;

        hr = pOIPS->OnPosRectChange(&rcPos);
    }
    else
        hr = _pOCS->RequestNewObjectLayout();

Cleanup:
    ReleaseInterface(pOIPS);
    ReleaseInterface(pOIPF);
    ReleaseInterface(pOIPUIW);
    return hr;
}

void CScriptlet::OnReadyStateChange()
{
    HRESULT hr;
    BSTR bstrName = NULL;
    IHTMLStyle *pStyle = NULL;

    if (_pDoc->_readyState == READYSTATE_COMPLETE)
    {
        // Discover if script name space contains the "public_description" object.
        DISPID dispidED;
        VARIANT v;

        if (THR(FormsAllocString(EXTERNAL_DESCRIPTION, &bstrName)))
            goto Cleanup;

        if (THR(_pDoc->EnsureOmWindow()))
            goto Cleanup;

        Assert(_pDoc->_pOmWindow);

        hr = THR(_pDoc->_pOmWindow->GetDispID(bstrName, fdexFromGetIdsOfNames, &dispidED));
        if (!hr)
        {
            Assert(dispidED != DISPID_UNKNOWN);
            DISPPARAMS dispparams = { NULL, NULL, 0, 0 };

            hr = _pDoc->_pOmWindow->InvokeEx(dispidED, LOCALE_USER_DEFAULT,
                              DISPATCH_METHOD|DISPATCH_PROPERTYGET, &dispparams, &v, NULL, NULL);
            if (!hr && v.vt == VT_DISPATCH)
            {
                _pDescription = v.pdispVal;
            }
        }
    }
    else if (_pDoc->_readyState == READYSTATE_INTERACTIVE)
    {
        // Need to recache the HTML element if doc gets blown away;
        ClearInterface(&_pHTMLElement);
        if (_fValidCx || _fValidCy)
        {
            hr = THR(GetStyleProperty(&pStyle));
            if (hr)
                goto Cleanup;
            if (!pStyle)
                goto Cleanup;
            if (_fValidCx)
                pStyle->put_pixelWidth(_sizePixExtent.cx);
            if (_fValidCy)
                pStyle->put_pixelHeight(_sizePixExtent.cy);
        }
    }

Cleanup:
    ReleaseInterface(pStyle);
    FormsFreeString(bstrName);

    // Fire the event on the ole site (<OBJECT>) and whoever else might be listening, if the
    // container is ready to handle events
    if (!_cFreezes)
        FireEvent(DISPID_READYSTATECHANGE, DISPID_UNKNOWN, (BYTE *)VTS_NONE);
    else
        _fDelayOnReadyStateFiring = TRUE;

    return;
}

void CScriptlet::SetWidth(DISPID dispid)
{
    long cpixels;
    IHTMLStyle *pStyle = NULL;

    if (GetStyleProperty(&pStyle))
        goto Cleanup;
    if (!pStyle)
        goto Cleanup;
    if (!pStyle->get_pixelWidth(&cpixels))
    {
        if (!_fValidCx || cpixels != _sizePixExtent.cx)
        {
            // HACK to avoid infinite scale factor error, if current size is 0
            if (_fValidCx && !_sizePixExtent.cx)
            {
                SIZEL sizel;
                if (!_sizePixExtent.cy)
                    _sizePixExtent.cy = 1;
                sizel.cx = HimetricFromHPix(cpixels);
                sizel.cy = HimetricFromVPix(_sizePixExtent.cy);
                _pDoc->SetExtent(DVASPECT_CONTENT, &sizel);
            }

            _sizePixExtent.cx = cpixels;
            _fValidCx = TRUE;
            Resize();
        }

        FireOnChanged(dispid);
    }

Cleanup:
    ReleaseInterface(pStyle);
    return;
}

void CScriptlet::SetHeight(DISPID dispid)
{
    long cpixels;
    IHTMLStyle *pStyle = NULL;
    
    if (GetStyleProperty(&pStyle))
        goto Cleanup;
    if (!pStyle)
        goto Cleanup;
    if (!pStyle->get_pixelHeight(&cpixels))
    {
        if (!_fValidCy || cpixels != _sizePixExtent.cy)
        {
            // HACK to avoid infinite scale factor error, if current size is 0
            if (_fValidCy && !_sizePixExtent.cy)
            {
                SIZEL sizel;
                if (!_sizePixExtent.cx)
                    _sizePixExtent.cx = 1;
                sizel.cx = HimetricFromHPix(_sizePixExtent.cx);
                sizel.cy = HimetricFromVPix(cpixels);
                _pDoc->SetExtent(DVASPECT_CONTENT, &sizel);
            }

            _sizePixExtent.cy = cpixels;
            _fValidCy = TRUE;
            Resize();
        }

        FireOnChanged(dispid);
    }

Cleanup:
    ReleaseInterface(pStyle);
    return;
}

void CScriptlet::OnVisibilityChange()
{
    RECT                    rcPos;
    RECT                    rcClip;
    OLEINPLACEFRAMEINFO     oipfi;
    IOleInPlaceSite        *pOIPS = NULL;
    IOleInPlaceFrame       *pOIPF = NULL;
    IOleInPlaceUIWindow    *pOIPUIW = NULL;
    IDispatchEx            *pDispEx = NULL;
    BOOL                    fNewVisible = _fIsVisible;
    HRESULT                 hr;
    
    if (_pDoc->State() < OS_INPLACE)
    {
        fNewVisible = FALSE;
        // BUGBUG: (anandra) Not quite correct.  Will do the right thing
        // in design mode in trident/vb because it's not inplace-active
        // but not quite correct because it might be visible. IE4 bug 59988.
    }
    else if (_pOCS)
    {
        hr = _pOCS->QueryInterface(
                IID_IOleInPlaceSite, 
                (void **) &pOIPS);
        if (hr)
            goto Cleanup;
            
        hr = pOIPS->GetWindowContext(
                &pOIPF,
                &pOIPUIW,
                &rcPos,
                &rcClip,
                &oipfi);
        if (hr)
            goto Cleanup;

        fNewVisible = IntersectRect(&rcPos, &rcPos, &rcClip) != 0;
    }

Cleanup:
    if ((unsigned)fNewVisible != _fIsVisible)
    {
        _fIsVisible = (unsigned)fNewVisible;
        
        if (V_VT(&_varOnVisChange) == VT_DISPATCH)
        {
            DISPPARAMS  dp = { 0 };

            // BUGBUG (sramani): consider calling DispatchInvokeWithThis ?
            if (SUCCEEDED(V_DISPATCH(&_varOnVisChange)->QueryInterface(
                    IID_IDispatchEx, (void **)&pDispEx)))
            {
                hr = pDispEx->InvokeEx(
                        DISPID_VALUE,
                        LOCALE_USER_DEFAULT,
                        DISPATCH_METHOD,
                        &dp,
                        NULL,
                        NULL,
                        NULL);
            }
            else
            {
                hr = V_DISPATCH(&_varOnVisChange)->Invoke(
                    DISPID_VALUE, 
                    IID_NULL,  
                    LOCALE_USER_DEFAULT,
                    DISPATCH_METHOD,  
                    &dp,  
                    NULL,  
                    NULL,  
                    NULL);
            }
        }
    }

    ReleaseInterface(pOIPS);
    ReleaseInterface(pOIPF);
    ReleaseInterface(pOIPUIW);
    ReleaseInterface(pDispEx);
}

HRESULT
Property_get(IDispatch * pDisp, DISPID dispid, VARIANT * pvar)
{
    DISPPARAMS dispparams = { NULL, NULL, 0, 0 };

    return pDisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD|DISPATCH_PROPERTYGET,
                         &dispparams, pvar, NULL, NULL);
}

// IPersistPropertyBag

STDMETHODIMP CScriptlet::GetClassID(CLSID *pClassID)
{
    *pClassID = *BaseDesc()->_pclsid;
    return S_OK;
}

STDMETHODIMP CScriptlet::InitNew()
{
    HRESULT hr; 

    if (_fHardWiredURL)
    {
        hr = LoadScriptletURL();
    }
    else
    {
        hr = _pDoc->InitNew();
    }
    
    return hr;
}

STDMETHODIMP CScriptlet::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    HRESULT hr;
    VARIANT var;
    TCHAR *pchUrl = NULL;
    
    VariantInit(&var);
    hr = THR(pPropBag->Read(_T("Scrollbar"), &var, pErrorLog));
    if (!hr && !VariantChangeType(&var, &var, 0, VT_BOOL))
        _vbScrollbar = var.boolVal;
    VariantClear(&var);

    if (!_fHardWiredURL)
    {
        hr = THR(pPropBag->Read(_T("URL"), &var, pErrorLog));
        if (hr == E_INVALIDARG)
        {
            // No URL property was saved in the property bag. We force the update
            // of the scrollbar and go home.

            hr = THR(_pDoc->InitNew());
            if (hr)
                goto Cleanup;

            _pDoc->OnAmbientPropertyChange(DISPID_UNKNOWN);
            hr = S_OK;
            goto Cleanup;
        }

        if (hr)
            goto Cleanup;

        hr = THR(VariantChangeType(&var, &var, 0, VT_BSTR));
        if (hr)
            goto Cleanup;

        pchUrl = var.bstrVal;
    }

    // Force Trident to update its UI
    _pDoc->OnAmbientPropertyChange(DISPID_UNKNOWN);

    hr = THR(LoadScriptletURL(pchUrl));

Cleanup:
    VariantClear(&var);
    RRETURN(hr);  
}

STDMETHODIMP CScriptlet::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr;
    VARIANT var;

    var.vt = VT_BOOL;
    var.boolVal = _vbScrollbar;
    hr = pPropBag->Write(_T("Scrollbar"), &var);
    if (hr)
        goto Cleanup;

    if (!_fHardWiredURL)
    {
        var.vt = VT_BSTR;
        hr = THR(_pDoc->_cstrUrl.AllocBSTR(&var.bstrVal));
        if (hr)
            goto Cleanup;

        hr = THR(pPropBag->Write(_T("URL"), &var));
        if (hr)
            goto Cleanup;
    }

    if (fClearDirty)
        _fRequiresSave = 0;

Cleanup:
    VariantClear(&var);
    RRETURN(hr); 
}

// IPersistStreamInit

STDMETHODIMP CScriptlet::IsDirty(void)
{
    if (_fRequiresSave)
        return S_OK;
    else
        return _pDoc->IsDirty();
}

STDMETHODIMP CScriptlet::Load(LPSTREAM pStm)
{
    HRESULT hr;
    CStr cstrUrl;

    hr = THR(pStm->Read(&_vbScrollbar, sizeof(_vbScrollbar), NULL));
    if (hr)
        goto Cleanup;

    if (!_fHardWiredURL)
    {
        hr = THR(cstrUrl.Load(pStm));
        if (hr)
            goto Cleanup;
    }

    _pDoc->OnAmbientPropertyChange(DISPID_UNKNOWN);
    hr = THR(LoadScriptletURL(cstrUrl));

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CScriptlet::Save(LPSTREAM pStm, BOOL fClearDirty)
{
    HRESULT hr;

    if (fClearDirty)
        _fRequiresSave = 0;

    hr = THR(pStm->Write(&_vbScrollbar, sizeof(_vbScrollbar), NULL));
    if (hr)
        goto Cleanup;

    if (!_fHardWiredURL)
        hr = _pDoc->_cstrUrl.Save(pStm);

Cleanup:
    RRETURN(hr);
}

BOOL CSortedAtomTable::Find(LPCTSTR pch, LONG *plIndex, BOOL fCaseSensitive)
{
    int r;
    long iLow  = 0;
    long iMid  = 0;
    long iHigh = Size() - 1;
    STRINGCOMPAREFN pfnCompareString = fCaseSensitive ? StrCmpC : StrCmpIC;
    long lIndex;
    TCHAR *pchCur;

    // Binary search for atom name
    while (iHigh >= iLow)
    {
        iMid = (iHigh + iLow) >> 1;
        lIndex = (long)_aryIndex[iMid];
        pchCur = (LPTSTR)(*(CStr *)Deref(sizeof(CStr), lIndex));
        r = pfnCompareString(pch, pchCur);
        if (r < 0)
        {
            iHigh = iMid - 1;
        }
        else if (r > 0)
        {
            iLow = iMid + 1;
        }
        else
        {
            *plIndex = lIndex;
            return TRUE;
        }
    }

    *plIndex = iHigh + 1;
    return FALSE;
}

HRESULT
CSortedAtomTable::Insert(LPCTSTR pch, LONG lInsertAt, LONG *plIndex)
{
    HRESULT hr = S_OK;
    WORD    wIndex;
    long    lIndex;
    CStr    cstrCopy;
    CStr   *pcstr;

    // Not found, so add atom(name) to end of array, so that the lIndex given out
    // is always the same (this could be a dispid that needs to be the same.
    hr = THR(AppendIndirect(&cstrCopy));
    if (hr)
        goto Cleanup;

    lIndex = Size() - 1;
    pcstr = (CStr *)Deref(sizeof(CStr), lIndex);
    hr = THR(pcstr->Set(pch));
    if (hr)
    {
        Delete(lIndex);
        goto Cleanup;
    }

    wIndex = (WORD)lIndex;

    // Insert the index of the new atom so that the sort order is maintained.
    hr = _aryIndex.InsertIndirect(lInsertAt, &wIndex);
    if (hr)
    {
        Delete(lIndex);
        goto Cleanup;
    }

    if (plIndex)
        *plIndex = lIndex;

Cleanup:
    RRETURN(hr);
}

void
CSortedAtomTable::Free()
{
    CStr *pcstr;
    long  i;
    
    for (i = 0; i < Size(); i++)
    {
        pcstr = (CStr *)Deref(sizeof(CStr), i);
        pcstr->Free();
    }

    _aryIndex.DeleteAll();
    DeleteAll();
}
