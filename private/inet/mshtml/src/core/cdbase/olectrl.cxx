//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       olectrl.cxx
//
//  Contents:   Ole Control Specific Interface implementations
//
//----------------------------------------------------------------------------


#include <headers.hxx>

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CControlBaseCtrl::GetControlInfo, IOleControl
//
//  Synopsis:   Returns a filled-in CONTROLINFO.
//
//  Arguments:  [pCI] -- CONTROLINFO to fill in
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::GetControlInfo(CONTROLINFO *pCI)
{
    memset(pCI, 0, sizeof(CONTROLINFO));
    pCI->cb = sizeof(CONTROLINFO);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::OnMnemonic, IOleControl
//
//  Synopsis:   Indicates one of our mnemonics has been pressed by the user
//              and we need to take the appropriate action.
//
//  Arguments:  [pMsg] -- Message which corresponds to a mnemonic.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::OnMnemonic(LPMSG pMsg)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::OnAmbientPropertyChange, IOleControl
//
//  Synopsis:   Indicates one or more ambient properties have changed so we
//              can update our state.
//
//  Arguments:  [dispid] -- Property which changed
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::OnAmbientPropertyChange(DISPID dispid)
{
    switch (dispid)
    {
    case DISPID_UNKNOWN:
    case DISPID_AMBIENT_USERMODE:
        _fUserMode = GetAmbientBool(DISPID_AMBIENT_USERMODE, TRUE);
        break;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::FreezeEvents, IOleControl
//
//  Synopsis:   Enables or disables the ability of the control to fire events.
//              Any control which cares about the status of the event
//              freeze count should overwrite this method.
//
//  Arguments:  [fFreeze] -- If TRUE, events are disabled.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CServer::FreezeEvents(BOOL fFreeze)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetDisplayString, IPerPropertyBrowsing
//
//----------------------------------------------------------------------------

HRESULT
CServer::GetDisplayString(DISPID dispid, BSTR * pbstr)
{
    if (pbstr)
        *pbstr = NULL;

    return S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::MapPropertyToPage, IPerPropertyBrowsing
//
//----------------------------------------------------------------------------

HRESULT
CServer::MapPropertyToPage(DISPID dispid, LPCLSID lpclsid)
{
    if (lpclsid)
        *lpclsid = CLSID_NULL;

    return PERPROP_E_NOPAGEAVAILABLE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetPredefinedStrings, IPerPropertyBrowsing
//
//----------------------------------------------------------------------------

HRESULT
CServer::GetPredefinedStrings(DISPID       dispid,
                              CALPOLESTR * pcaStringsOut,
                              CADWORD *    lpcaCookiesOut)
{
    if (pcaStringsOut)
    {
        pcaStringsOut->cElems = 0;
        pcaStringsOut->pElems = NULL;
    }

    if (lpcaCookiesOut)
    {
        lpcaCookiesOut->cElems = 0;
        lpcaCookiesOut->pElems = NULL;
    }

    // BUGBUG (ChrisF): according to the spec this should probably return S_FALSE
    // however, because of a bug in VB4 and VB5/VBA this causes VB not to look for
    // the string in the typelib either. 
    return E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CServer::GetPredefinedValue, IPerPropertyBrowsing
//
//----------------------------------------------------------------------------

HRESULT
CServer::GetPredefinedValue(DISPID dispid, DWORD dwCookie, VARIANT * pvarOut)
{
    if (pvarOut)
        V_VT(pvarOut) = VT_EMPTY;

    return S_FALSE;
}
