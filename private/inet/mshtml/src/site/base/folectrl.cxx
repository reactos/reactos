//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       olectrl.cxx
//
//  Contents:   Implementation of IOleControl methods
//
//  Classes:    CDoc (partial)
//
//  History:    05-Feb-94     LyleC    Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_IDISPIDS_H_
#define X_IDISPIDS_H_
#include "idispids.h"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

void
CDoc::GetLoadFlag(DISPID dispidProp)
{

    BOOL fVal = FALSE;
    DWORD dwCurrFlag = (dispidProp == DISPID_AMBIENT_SILENT) ? DLCTL_SILENT : DLCTL_OFFLINEIFNOTCONNECTED;
    fVal = GetAmbientBool(dispidProp, FALSE);
    if(fVal)
        _dwLoadf |= dwCurrFlag;
    else
        _dwLoadf &= (~dwCurrFlag);

    return;
}
//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnAmbientPropertyChange, public
//
//  Synopsis:   Captures ambient property changes and takes appropriate action.
//
//  Arguments:  [dispidProp] -- Property which changed.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::OnAmbientPropertyChange(DISPID dispidProp)
{
    BOOL            fAll = (dispidProp == DISPID_UNKNOWN);
    BOOL            fMode;
    CNotification   nf;

    // Update dochostui flags - helps us read DOCHOSTUIFLAG_FLAT_SCROILLBAR
    // correctly when we switch to/from full screen mode
    if (fAll && _pHostUIHandler)
    {
        DOCHOSTUIINFO   docHostUIInfo;
        CBodyElement *  pBody;
        DWORD           dwFlagsHostInfoOld = _dwFlagsHostInfo;

        // Initialize
        memset(&docHostUIInfo, 0, sizeof(DOCHOSTUIINFO));
        docHostUIInfo.cbSize = sizeof(DOCHOSTUIINFO);
        if (OK(_pHostUIHandler->GetHostInfo(&docHostUIInfo)))
        {
            HRESULT hr;
                
            if (docHostUIInfo.pchHostNS)
            {
                hr = THR(_cstrHostNS.Set(docHostUIInfo.pchHostNS));
                if (hr)
                    RRETURN(hr);
            }

            if (docHostUIInfo.pchHostCss)
            {
                hr = THR(_cstrHostCss.Set(docHostUIInfo.pchHostCss));
                if (hr)
                    RRETURN(hr);

                hr = THR(EnsureHostStyleSheets());
                if (hr)
                    RRETURN(hr);
            }

            _dwFlagsHostInfo = docHostUIInfo.dwFlags;

             // What we use for a default block tag.
             if (_dwFlagsHostInfo & DOCHOSTUIFLAG_DIV_BLOCKDEFAULT)
                 SetDefaultBlockTag(ETAG_DIV);
             else
                 SetDefaultBlockTag(ETAG_P);

            CoTaskMemFree(docHostUIInfo.pchHostCss);
            CoTaskMemFree(docHostUIInfo.pchHostNS);
        }

        IGNORE_HR(GetBodyElement(&pBody));
        if (    pBody
            &&  dwFlagsHostInfoOld != _dwFlagsHostInfo
            &&  GetView()->IsActive())
        {
            pBody->ResizeElement();
        }
    }

    // Workaround for bugs 4.0 60383, 4.01SP1 #?, 5.0 5781: (also see CDoc::SetClientSite)
    // if hosted in a frame, grab the DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE flag from the top doc
    // so that nested frames don't block waiting for in-place activation when the top doc doesn't.
    if (_pDocParent)
    {
        _dwFlagsHostInfo = (_dwFlagsHostInfo & ~DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE) |
                           (GetRootDoc()->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE);
    }

    if (fAll)
    {
        // Update the scrollbar info
        IGNORE_HR(OnFrameOptionScrollChange());
    }

    // Check to see if DesignMode should be set. If the ambient is
    // not supported by the client site then set DesignMode.

    if ((fAll || dispidProp == DISPID_AMBIENT_USERMODE) &&
            _fInheritDesignMode)
    {
        if (!_fFrameSet)
        {
            fMode = !GetAmbientBool(DISPID_AMBIENT_USERMODE, !_fDesignMode);
        }
        else
        {
            // Can't go into edit (design) mode if our top-level site is a
            // frameset.
            fMode = FALSE;
        }

        if (fMode != (BOOL) _fDesignMode)
        {
            IGNORE_HR(UpdateDesignMode(fMode));
        }

    }

    if (fAll ||
        dispidProp == DISPID_AMBIENT_DLCONTROL ||
        dispidProp == DISPID_AMBIENT_SILENT ||
        dispidProp == DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)
    {
        CVariant var;

        if (!GetAmbientVariant(DISPID_AMBIENT_DLCONTROL, &var) &&
            V_VT(&var) == VT_I4)
        {
            _dwLoadf = (DWORD) V_I4(&var);
            _fGotAmbientDlcontrol = TRUE;
        }
        else
        {
            _fGotAmbientDlcontrol = FALSE;
            if (fAll)
            {
                GetLoadFlag(DISPID_AMBIENT_SILENT);
                GetLoadFlag(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
            }
            else
            {
                Assert(dispidProp == DISPID_AMBIENT_SILENT ||
                       dispidProp == DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
                GetLoadFlag(dispidProp);
            }
        }
    }

    if (fAll || dispidProp == DISPID_AMBIENT_USERAGENT)
    {
        FormsFreeString(_bstrUserAgent);
        GetAmbientBstr(DISPID_AMBIENT_USERAGENT, &_bstrUserAgent);
    }

    if (fAll || dispidProp == DISPID_AMBIENT_PALETTE)
    {
        HPALETTE hpalAmbient = GetAmbientPalette();

        if (hpalAmbient != _hpalAmbient)
        {
            _hpalAmbient = hpalAmbient;
            _fHtAmbientPalette = IsHalftonePalette(hpalAmbient);
            
            Invalidate();
            if (_hpalDocument)
            {
                DeleteObject(_hpalDocument);
                _hpalDocument = 0;
                _fHtDocumentPalette = FALSE;
            }
        }
    }

    //
    // Forward to all the sites.
    //

    if (GetPrimaryElementClient())
    {
        nf.AmbientPropChange(PrimaryRoot(), (void *)(DWORD_PTR)dispidProp);
        BroadcastNotify(&nf);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnControlInfoChanged, public
//
//  Synopsis:   To be called when our control info has changed.  This normally
//              happens when a site gets an OnControlInfoChanged call from
//              its control.  Calls OnControlInfoChanged on our site.
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------
void
CDoc::OnControlInfoChanged(DWORD_PTR dwContext)
{
    IOleControlSite * pCtrlSite;

    if (_pClientSite &&
        OK(_pClientSite->QueryInterface(IID_IOleControlSite, (void **) &pCtrlSite)))
    {
        IGNORE_HR(pCtrlSite->OnControlInfoChanged());
        pCtrlSite->Release();
    }

    _fOnControlInfoChangedPosted = FALSE;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetControlInfo, public
//
//  Synopsis:   Returns a filled-in CONTROLINFO.
//
//  Arguments:  [pCI] -- CONTROLINFO to fill in
//
//  Returns:    HRESULT
//
//  Notes:      Note that the hAccel we hand out is not valid past our own
//              lifetime, because we call DestroyAcceleratorTable in our
//              destructor.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::GetControlInfo(CONTROLINFO *pCI)
{
#ifdef WIN16
    MessageBox(NULL, "CDoc::GetControlInfo:: need CreateAcceleratorTable.", "BUGWIN16", MB_OK);
    return E_FAIL;
#else
    ACCEL       aaccel[2];

    if (!pCI)
        RRETURN(E_POINTER);

    // BUGBUG -- is this desired behavior?
    if (pCI->cb != sizeof(CONTROLINFO))
        RRETURN(E_INVALIDARG);

    pCI->cAccel = 0;

    aaccel[0].fVirt = FVIRTKEY | FALT | FCONTROL | FSHIFT;
    aaccel[0].key   = 0;    // any key
    aaccel[0].cmd   = 0;    // Not used.

    aaccel[1].fVirt = FALT | FCONTROL | FSHIFT;
    aaccel[1].key   = 0;    // any key
    aaccel[1].cmd   = 0;    // Not used.

    pCI->hAccel = CreateAcceleratorTable(aaccel, 2);
    if (pCI->hAccel == NULL)
        return E_OUTOFMEMORY;

    pCI->cAccel = 2;

    //
    // We don't eat return or escape because we always delegate those keys
    // to our parent before we take action on them.
    //
    pCI->dwFlags = 0;

    //  CONSIDER don't we eat return or escape if the active control in
    //    the form eats it?


    return S_OK;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::OnMnemonic, public
//
//  Synopsis:   Indicates one of our mnemonics has been pressed by the user
//              and we need to take the appropriate action.
//
//  Arguments:  [pMsg] -- Message which corresponds to a mnemonic.
//
//  Returns:    S_OK
//
//  History:    02-Feb-94     LyleC    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::OnMnemonic(LPMSG lpmsg)
{
    HRESULT hr;
    CMessage Message(lpmsg);

    hr = TransitionTo(OS_UIACTIVE);
    if (!hr)
    {
        hr = PumpMessage(&Message, PrimaryRoot()->GetFirstBranch());
    }

    RRETURN1(hr, S_FALSE);
}



//+------------------------------------------------------------------------
//
//  Member:     CDoc::FreezeEvents
//
//  Synopsis:   Broadcast the freeze events notification to all of our
//              controls
//
//  Arguments:  [fFreeze]   TRUE if events are being frozen by the
//                          development environment, FALSE if not.
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDoc::FreezeEvents(BOOL fFreeze)
{
    //
    // Assert that the freeze count is not going below zero.
    //
    //AssertSz(fFreeze || _cFreeze, "Too many FreezeEvents(FALSE) calls received by the document");

    _cFreeze = fFreeze ? _cFreeze + 1 : _cFreeze - 1;



    //
    // Notify sites if there was a change in freeze status
    //

    if (    _fHasOleSite
        &&  GetPrimaryElementClient()
        &&  (!_cFreeze || (fFreeze && (_cFreeze == 1))))
    {
        CNotification   nf;
        nf.FreezeEvents(PrimaryRoot(), (void *)(DWORD_PTR)fFreeze);
        BroadcastNotify(&nf);
    }

    return S_OK;
}
