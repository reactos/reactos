// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TOOLTIPS.CPP
//
//  Knows how to talk to COMCTL32's tooltips.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "tooltips.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOHOTKEY
#define NOHEADER
#define NOLISTVIEW
#define NOTREEVIEW
#define NOTABCONTROL
#define NOANIMATE
#define NOTOOLBAR
#include <commctrl.h>



// --------------------------------------------------------------------------
//
//  CreateToolTipsClient()
//
// --------------------------------------------------------------------------
HRESULT CreateToolTipsClient(HWND hwnd, long idChildCur, REFIID riid, void **ppvToolTips)
{
    CToolTips32 *   ptooltips;
    HRESULT         hr;

    InitPv(ppvToolTips);

    ptooltips = new CToolTips32(hwnd, idChildCur);
    if (!ptooltips)
        return(E_OUTOFMEMORY);

    hr = ptooltips->QueryInterface(riid, ppvToolTips);
    if (!SUCCEEDED(hr))
        delete ptooltips;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CToolTips32::CToolTips32()
//
// --------------------------------------------------------------------------
CToolTips32::CToolTips32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);
}


// --------------------------------------------------------------------------
//
//  CToolTips32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CToolTips32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_TOOLTIP;

    return(S_OK);
}
