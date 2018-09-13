//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       scrpctrl.cxx
//
//  History:    19-Jan-1998     sramani     Created
//
//  Contents:   CScriptControl implementation
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_SCRPTLET_HXX_
#define X_SCRPTLET_HXX_
#include "scrptlet.hxx"
#endif

#ifndef X_SCRPCTRL_HXX_
#define X_SCRPCTRL_HXX_
#include "scrpctrl.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_SCRSBOBJ_HXX_
#define X_SCRSBOBJ_HXX_
#include "scrsbobj.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_EVENTOBJ_H_
#define X_EVENTOBJ_H_
#include "eventobj.h"
#endif

MtDefine(CScriptControl, Scriptlet, "CScriptControl")

const CBase::CLASSDESC CScriptControl::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IWBScriptControl,          // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

HRESULT
CScriptControl::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
    default:
        if (IsEqualIID(iid, IID_IWBScriptControl))
            *ppv = (IWBScriptControl *)this;
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IWBScriptControl

STDMETHODIMP
CScriptControl::raiseEvent(BSTR name, VARIANT eventData)
{
    BYTE byParamTypes[3] = {VT_BSTR, VT_VARIANT, 0};
    return _pScriptlet->FireEvent(1, DISPID_UNKNOWN, byParamTypes, name, eventData);
}

struct
{
    LPOLESTR    pstrEventName;
    DISPID      dispidEvent;
} mppstreventinfo [] =
{
    { _T("click"),      DISPID_HTMLDOCUMENTEVENTS_ONCLICK },
    { _T("dblclick"),   DISPID_HTMLDOCUMENTEVENTS_ONDBLCLICK },
    { _T("keydown"),    DISPID_HTMLDOCUMENTEVENTS_ONKEYDOWN },
    { _T("keypress"),   DISPID_HTMLDOCUMENTEVENTS_ONKEYPRESS },
    { _T("keyup"),      DISPID_HTMLDOCUMENTEVENTS_ONKEYUP },
    { _T("mousedown"),  DISPID_HTMLDOCUMENTEVENTS_ONMOUSEDOWN },
    { _T("mousemove"),  DISPID_HTMLDOCUMENTEVENTS_ONMOUSEMOVE },
    { _T("mouseup"),    DISPID_HTMLDOCUMENTEVENTS_ONMOUSEUP }
};                   

STDMETHODIMP
CScriptControl::bubbleEvent()
{
    int              i;
    DISPID           dispidEvent;
    BSTR             bstrType;
    IHTMLWindow2    *pHW = NULL;
    IHTMLEventObj   *pEO = NULL;

    // Look at the Window object to see whether it has an event posted.

    if (_pScriptlet->_pDoc->get_parentWindow(&pHW))
        goto Cleanup;
    if (pHW->get_event(&pEO))
        goto Cleanup;
    if (!pEO)
        goto Cleanup;
 
    // They did. Find the flavour of the event and do the appropriate thing:

    if (pEO->get_type(&bstrType))
        goto Cleanup;

    dispidEvent = DISPID_UNKNOWN;
    for (i = 0; i < ARRAY_SIZE(mppstreventinfo); ++i)
    {
        if (!_tcscmp(mppstreventinfo[i].pstrEventName, bstrType))
        {
            dispidEvent = mppstreventinfo[i].dispidEvent;
            break;
        }
    }
    if (dispidEvent == DISPID_UNKNOWN)
        goto Cleanup;

    _pScriptlet->FireEvent(dispidEvent, DISPID_UNKNOWN, (BYTE *)VTS_NONE);

Cleanup:
    ReleaseInterface(pHW);
    ReleaseInterface(pEO);
    return S_OK;
}

STDMETHODIMP
CScriptControl::setContextMenu(VARIANT var)
{
    return _pScriptlet->_ScriptletSubObjects.SetContextMenu(var);
}


STDMETHODIMP
CScriptControl::get_selectableContent(VARIANT_BOOL * vbSelectable)
{
    *vbSelectable = _pScriptlet->_vbSelectable;
    return S_OK;
}

STDMETHODIMP
CScriptControl::put_selectableContent(VARIANT_BOOL vbSelectable)
{
    _pScriptlet->_vbSelectable = vbSelectable ? VARIANT_TRUE : VARIANT_FALSE;
    return _pScriptlet->_pDoc->OnAmbientPropertyChange(DISPID_UNKNOWN);
}

STDMETHODIMP
CScriptControl::get_frozen(VARIANT_BOOL * pvbFrozen)
{
    HRESULT hr = S_OK;
    if (!pvbFrozen)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pvbFrozen = _pScriptlet->_cFreezes > 0 ? VARIANT_TRUE : VARIANT_FALSE;

Cleanup:
    return hr;
}

STDMETHODIMP
CScriptControl::get_scrollbar(VARIANT_BOOL * pvbShow)
{
    return _pScriptlet->get_Scrollbar(pvbShow);
}

STDMETHODIMP
CScriptControl::put_scrollbar(VARIANT_BOOL vbShow)
{
    return _pScriptlet->put_Scrollbar(vbShow);
}

STDMETHODIMP
CScriptControl::get_version(BSTR * pbstr)
{
    return (FormsAllocString(_T("5.0 Win32"), pbstr));
}

STDMETHODIMP
CScriptControl::get_visibility(VARIANT_BOOL *pvbVisibility)
{
    if (!pvbVisibility)
        return E_POINTER;
    
    *pvbVisibility = _pScriptlet->_fIsVisible ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}


STDMETHODIMP
CScriptControl::get_onvisibilitychange(VARIANT *pvar)
{
    if (!pvar)
        return E_POINTER;
        
    return VariantCopy(pvar, &_pScriptlet->_varOnVisChange);
}


STDMETHODIMP
CScriptControl::put_onvisibilitychange(VARIANT var)
{
    return VariantCopy(&_pScriptlet->_varOnVisChange, &var);
}
