//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       scroll.cxx
//
//  Contents:   Contains CDoc methods related to scrolling behavior.
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Scroll
//
//  Synopsis:   Scrolls the form.
//
//  Note:       A single scroll event is fired.
//
//--------------------------------------------------------------------------

#if 0
STDMETHODIMP
CDoc::Scroll(VARIANT varXAction, VARIANT varYAction)
{
    // BUGBUG (garybu) move method to 2dsite
    // BUGBUG (rodc) Nasty cast to the 2DSite (top).  Cleanup soon!

    HRESULT             hr = S_OK;
    VARIANT *           pvar;
    VARIANT             var;
    UINT                uCode;
    long                lPos;
    int                 i;
    BOOL                fScroll;
    long                dxl = 0;
    long                dyl = 0;
    fmScrollAction      xAction = fmScrollActionNoChange;
    fmScrollAction      yAction = fmScrollActionNoChange;

    for (i = 0; i < 2; i++)
    {
        lPos = 0;
        fScroll = TRUE;

        pvar = (i == 0 ? &varXAction : &varYAction);
        if (pvar->vt == VT_ERROR)
            continue;

        VariantInit(&var);
        hr = THR(VariantChangeTypeSpecial(&var, pvar, VT_I4));
        if (hr)
            goto Cleanup;

        if (i == 0)
        {
            xAction = (fmScrollAction) V_I4(&var);
        }
        else
        {
            yAction = (fmScrollAction) V_I4(&var);
        }

        switch(V_I4(&var))
        {
        case fmScrollActionLineUp:
            uCode = SB_LINEUP;
            break;

        case fmScrollActionLineDown:
            uCode = SB_LINEDOWN;
            break;

        case fmScrollActionPageUp:
            uCode = SB_PAGEUP;
            break;

        case fmScrollActionPageDown:
            uCode = SB_PAGEDOWN;
            break;

        case fmScrollActionBegin:
            uCode = SB_THUMBPOSITION;
            break;

        case fmScrollActionEnd:
            uCode = SB_THUMBPOSITION;
            lPos = LONG_MAX;
            break;

        case fmScrollActionNoChange:
            fScroll = FALSE;
            break;

        case fmScrollActionAbsoluteChange:
            uCode = SB_THUMBPOSITION;
            break;

        default:
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (fScroll)
        {
            DYNCAST(C2DSite, _RootSite._pElemClient)->OnScrollHelper(
                    i,
                    uCode,
                    lPos,
                    (i == 0 ? &dxl : &dyl));
        }
    }

    hr = THR(_RootSite.ScrollBy(dxl, dyl, xAction, yAction));

Cleanup:
    RRETURN(SetErrorInfo(hr));
    return S_OK;
}
#endif
