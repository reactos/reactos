// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  UPDOWN.CPP
//
//  This knows how to talk to COMCTL32's updown control.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "updown.h"


#define NOSTATUSBAR
#define NOTOOLBAR
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
#include <commctrl.h>




// --------------------------------------------------------------------------
//
//  CreateUpDownClient()
//
// --------------------------------------------------------------------------
HRESULT CreateUpDownClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvClient)
{
    CUpDown32 * pupdown;
    HRESULT     hr;

    InitPv(ppvClient);

    pupdown = new CUpDown32(hwnd, idChildCur);
    if (!pupdown)
        return(E_OUTOFMEMORY);

    hr = pupdown->QueryInterface(riid, ppvClient);
    if (!SUCCEEDED(hr))
        delete pupdown;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CUpDown32::CUpDown32()
//
// --------------------------------------------------------------------------
CUpDown32::CUpDown32(HWND hwnd, long idChildCur)
{
    Initialize(hwnd, idChildCur);

    m_cChildren = CCHILDREN_UPDOWN;

    if (!(GetWindowLong(m_hwnd, GWL_STYLE) & UDS_HORZ))
        m_fVertical = TRUE;
}



// --------------------------------------------------------------------------
//
//  CUpDown32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accName(varChild, pszName));

    //
    // Remember:
    // Spin buttons work opposite to the way that scrollbars do.  When you 
    // push the up arrow button in a vertical scrollbar, you are _decreasing_
    // the position of the vertical scrollbar, its value.  When you push
    // the up arrow button in a vertical spin button, you are _increasing_
    // its value.
    //
    return(HrCreateString(STR_SPIN_GREATER + varChild.lVal - 1, pszName));
}



// --------------------------------------------------------------------------
//
//  CUpDown32::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    long    lPos;

    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
        return(E_NOT_APPLICABLE);

    lPos = SendMessageINT(m_hwnd, UDM_GETPOS, 0, 0);

    return(VarBstrFromI4(lPos, 0, 0, pszValue));
}



// --------------------------------------------------------------------------
//
//  CUpDown32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
        pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
    else
        pvarRole->lVal = ROLE_SYSTEM_SPINBUTTON;
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CUpDown32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    RECT    rc;
    int     iCoord;
    int     nHalf;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    // The buttons split the client area in half.
    MyGetRect(m_hwnd, &rc, FALSE);
    MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rc, 2);

    iCoord = (m_fVertical ? 1 : 0);
    nHalf = (((LPINT)&rc)[iCoord] + ((LPINT)&rc)[iCoord+2]) / 2;

    //
    // We want the right side of the left button to be the halfway point.
    // We want the left side of the right button to be the halfway point.
    // We want the bottom side of the up button to be the halfway point.
    // We want the top side of the down button to be the halfway point.
    //
    ((LPINT)&rc)[iCoord + ((varChild.lVal == INDEX_UPDOWN_UPLEFT) ? 2 : 0)] =
        nHalf;

    *pxLeft = rc.left;
    *pyTop = rc.top;
    *pcxWidth = rc.right - rc.left;
    *pcyHeight = rc.bottom - rc.top;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CUpDown32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    HRESULT hr;
    POINT   pt;
    RECT    rc;
    int     iCoord;
    int     nHalf;

    //
    // If the point isn't in us at all, don't bother hit-testing for the
    // button item.
    //
    hr = CClient::accHitTest(x, y, pvarHit);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarHit->vt != VT_I4) || (pvarHit->lVal != 0))
        return(hr);

    pt.x = x;
    pt.y = y;
    ScreenToClient(m_hwnd, &pt);

    MyGetRect(m_hwnd, &rc, FALSE);

    iCoord = (m_fVertical ? 1 : 0);
    nHalf = (((LPINT)&rc)[iCoord] + ((LPINT)&rc)[iCoord+2]) / 2;

    if (((LPINT)&pt)[iCoord] < nHalf)
        pvarHit->lVal = INDEX_UPDOWN_UPLEFT;
    else
        pvarHit->lVal = INDEX_UPDOWN_DNRIGHT;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CUpDown32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::accNavigate(long dwNavDir, VARIANT varStart, VARIANT* pvarEnd)
{
    long    lEnd = 0;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir == NAVDIR_FIRSTCHILD)
        dwNavDir = NAVDIR_NEXT;
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        dwNavDir = NAVDIR_PREVIOUS;
        varStart.lVal = m_cChildren + 1;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
NextChild:
            lEnd = varStart.lVal+1;
            if (lEnd > m_cChildren)
                lEnd = 0;
            break;

        case NAVDIR_PREVIOUS:
PreviousChild:
            lEnd = varStart.lVal-1;
            break;

        case NAVDIR_UP:
            if (m_fVertical)
                goto PreviousChild;
            else
                lEnd = 0;
            break;

        case NAVDIR_DOWN:
            if (m_fVertical)
                goto NextChild;
            else
                lEnd = 0;
            break;

        case NAVDIR_LEFT:
            if (!m_fVertical)
                goto PreviousChild;
            else
                lEnd = 0;
            break;

        case NAVDIR_RIGHT:
            if (!m_fVertical)
                goto NextChild;
            else
                lEnd = 0;
            break;
    }

    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;

        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CUpDown32::put_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CUpDown32::put_accValue(VARIANT varChild, BSTR szValue)
{
    long    lPos;
    HRESULT hr;

    // 
    // BOGUS!  Do we set the pos directly, or set this in the buddy?
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
        return(E_NOT_APPLICABLE);

    lPos = 0;
    hr = VarI4FromStr(szValue, 0, 0, &lPos);
    if (!SUCCEEDED(hr))
        return(hr);

    SendMessage(m_hwnd, UDM_SETPOS, 0, lPos);

    return(S_OK);
}
