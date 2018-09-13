// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  SCROLL.CPP
//
//  Scroll bar class.
//
//  OUTSTANDING ISSUES:
//  Internationalize scrollbar placement for RtoL languages in window.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "scroll.h"



/////////////////////////////////////////////////////////////////////////////
//
//  SCROLLBAR (in a Window)
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
//
//  CreateScrollBarObject()
//
// --------------------------------------------------------------------------
HRESULT CreateScrollBarObject(HWND hwnd, long idObject, REFIID riid, void** ppvScroll)
{
    return(CreateScrollBarThing(hwnd, idObject, 0, riid, ppvScroll));
}



// --------------------------------------------------------------------------
//
//  CreateScrollBarThing()
//
// --------------------------------------------------------------------------
HRESULT CreateScrollBarThing(HWND hwnd, long idObject, long iItem, REFIID riid, void** ppvScroll)
{
    CScrollBar * pscroll;
    HRESULT     hr;

    InitPv(ppvScroll);

    pscroll = new CScrollBar();
    if (pscroll)
    {
        if (! pscroll->FInitialize(hwnd, idObject, iItem))
        {
            delete pscroll;
            return(E_FAIL);
        }
    }
    else
        return(E_OUTOFMEMORY);

    hr = pscroll->QueryInterface(riid, ppvScroll);
    if (!SUCCEEDED(hr))
        delete pscroll;

    return(hr);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::Clone(IEnumVARIANT** ppenum)
{
    return(CreateScrollBarThing(m_hwnd, (m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
        m_idChildCur, IID_IEnumVARIANT, (void**)ppenum));
}



// --------------------------------------------------------------------------
//
//  CScrollBar::FInitialize()
//
// --------------------------------------------------------------------------
BOOL CScrollBar::FInitialize(HWND hwndScrollBar, LONG idObject, LONG iChildCur)
{
    if (! IsWindow(hwndScrollBar))
        return(FALSE);

    m_hwnd = hwndScrollBar;

    m_cChildren = CCHILDREN_SCROLLBAR;
    m_idChildCur = iChildCur;

    m_fVertical = (idObject == OBJID_VSCROLL);

    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  GetScrollMask()
//
//  Gets present elements (may or may not be offscreen)
//
// --------------------------------------------------------------------------
void FixUpScrollBarInfo(LPSCROLLBARINFO lpsbi)
{
    if (lpsbi->rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_UNAVAILABLE)
    {
        lpsbi->rgstate[INDEX_SCROLLBAR_UPPAGE] |= STATE_SYSTEM_INVISIBLE;
        lpsbi->rgstate[INDEX_SCROLLBAR_THUMB] |= STATE_SYSTEM_INVISIBLE;
        lpsbi->rgstate[INDEX_SCROLLBAR_DOWNPAGE] |= STATE_SYSTEM_INVISIBLE;
    }
}



// --------------------------------------------------------------------------
//
//  CScrollBar::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_SCROLLBAR_NAME + varChild.lVal +
        (m_fVertical ? 0 : INDEX_SCROLLBAR_HORIZONTAL), pszName));
}



// --------------------------------------------------------------------------
//
//  CScrollBar::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    long    lPos;

    InitPv(pszValue);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
        return(E_NOT_APPLICABLE);

    //
    // The value is the position.
    //
    lPos = GetScrollPos(m_hwnd, (m_fVertical ? SB_VERT : SB_HORZ));

    int Min, Max;
    GetScrollRange( m_hwnd, (m_fVertical ? SB_VERT : SB_HORZ), & Min, & Max );

    // work out a percent value...
    if( Min != Max )
        lPos = ( ( lPos - Min ) * 100 ) / ( Max - Min );
    else
        lPos = 0; // Prevent div-by-0

    return(VarBstrFromI4(lPos, 0, 0, pszValue));
}




// --------------------------------------------------------------------------
//
//  CScrollBar::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::get_accDescription(VARIANT varChild, BSTR* pszDesc)
{
    InitPv(pszDesc);

    //
    // Validate the params
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_SCROLLBAR_DESCRIPTION + varChild.lVal +
        (m_fVertical ? 0 : INDEX_SCROLLBAR_HORIZONTAL), pszDesc));
}


// --------------------------------------------------------------------------
//
//  CScrollBar::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_SELF:
            pvarRole->lVal = ROLE_SYSTEM_SCROLLBAR;
            break;

        case INDEX_SCROLLBAR_UP:
        case INDEX_SCROLLBAR_DOWN:
        case INDEX_SCROLLBAR_UPPAGE:
        case INDEX_SCROLLBAR_DOWNPAGE:
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
            break;

        case INDEX_SCROLLBAR_THUMB:
            pvarRole->lVal = ROLE_SYSTEM_INDICATOR;
            break;

        default:
            AssertStr( "Invalid ChildID for child of scroll bar" );
    }

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    SCROLLBARINFO   sbi;

    InitPvar(pvarState);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    //
    // Get our information
    //
    if (! MyGetScrollBarInfo(m_hwnd, (m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
            &sbi)       ||
        (sbi.rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_INVISIBLE))
    {
        //
        // If scrollbar isn't there period, fail.
        //
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }

    //
    // If unavailable or offscreen, everything is.
    //
    FixUpScrollBarInfo(&sbi);

    pvarState->lVal |= sbi.rgstate[INDEX_SCROLLBAR_SELF];
    pvarState->lVal |= sbi.rgstate[varChild.lVal];

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::get_accDefaultAction(VARIANT varChild,
    BSTR * pszDefAction)
{
    InitPv(pszDefAction);

    //
    // Validate the params
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_UP:
        case INDEX_SCROLLBAR_UPPAGE:
        case INDEX_SCROLLBAR_DOWNPAGE:
        case INDEX_SCROLLBAR_DOWN:
            return(HrCreateString(STR_BUTTON_PUSH, pszDefAction));
    }

    return(E_NOT_APPLICABLE);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    SCROLLBARINFO   sbi;
    int             dxyButton;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate params
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! MyGetScrollBarInfo(m_hwnd, (m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
            &sbi)       ||
        (sbi.rgstate[INDEX_TITLEBAR_SELF] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)))
    {
        return(S_FALSE);
    }

    FixUpScrollBarInfo(&sbi);
    if (sbi.rgstate[varChild.lVal] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN))
        return(S_FALSE);

    if (m_fVertical)
        dxyButton = sbi.rcScrollBar.right - sbi.rcScrollBar.left;
    else
        dxyButton = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
            
    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_SELF:
            *pxLeft = sbi.rcScrollBar.left;
            *pyTop = sbi.rcScrollBar.top;
            *pcxWidth = sbi.rcScrollBar.right - sbi.rcScrollBar.left;
            *pcyHeight = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
            break;

        case INDEX_SCROLLBAR_UP:
        case INDEX_SCROLLBAR_DOWN:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;
                *pcyHeight = sbi.dxyLineButton;

                if (varChild.lVal == INDEX_SCROLLBAR_UP)
                    *pyTop = sbi.rcScrollBar.top;
                else
                    *pyTop = sbi.rcScrollBar.bottom - *pcyHeight;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;
                *pcxWidth = sbi.dxyLineButton;

                if (varChild.lVal == INDEX_SCROLLBAR_UP)
                    *pxLeft = sbi.rcScrollBar.left;
                else
                    *pxLeft = sbi.rcScrollBar.right - *pcxWidth;
            }
            break;

        case INDEX_SCROLLBAR_UPPAGE:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;

                *pyTop = sbi.rcScrollBar.top + sbi.dxyLineButton;
                *pcyHeight = sbi.xyThumbTop - sbi.dxyLineButton;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;

                *pxLeft = sbi.rcScrollBar.left + sbi.dxyLineButton;
                *pcxWidth = sbi.xyThumbTop - sbi.dxyLineButton;
            }
            break;

        case INDEX_SCROLLBAR_DOWNPAGE:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;

                *pyTop = sbi.rcScrollBar.top + sbi.xyThumbBottom;
                *pcyHeight = (sbi.rcScrollBar.bottom - sbi.rcScrollBar.top) -
                    sbi.xyThumbBottom - sbi.dxyLineButton;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;

                *pxLeft = sbi.rcScrollBar.left + sbi.xyThumbBottom;
                *pcxWidth = (sbi.rcScrollBar.right - sbi.rcScrollBar.left) -
                    sbi.xyThumbBottom - sbi.dxyLineButton;
            }
            break;

        case INDEX_SCROLLBAR_THUMB:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;

                *pyTop = sbi.rcScrollBar.top + sbi.xyThumbTop;
                *pcyHeight = sbi.xyThumbBottom - sbi.xyThumbTop;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;

                *pxLeft = sbi.rcScrollBar.left + sbi.xyThumbTop;
                *pcxWidth = sbi.xyThumbBottom - sbi.xyThumbTop;
            }
            break;

        default:
            AssertStr( "Invalid ChildID for child of scroll bar" );
    }

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT * pvarEnd)
{
    long    lEndUp = 0;
    SCROLLBARINFO sbi;

    InitPvar(pvarEnd);

    //
    // Validate params
    //
    if (! ValidateChild(&varStart)   ||
        ! ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (! MyGetScrollBarInfo(m_hwnd, (m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
        &sbi))
    {
        return(S_FALSE);
    }

    if (dwNavDir == NAVDIR_FIRSTCHILD)
    {
        dwNavDir = NAVDIR_NEXT;
    }
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        dwNavDir = NAVDIR_PREVIOUS;
        varStart.lVal = m_cChildren + 1;
    }
    else if (varStart.lVal == INDEX_SCROLLBAR_SELF)
        return(GetParentToNavigate((m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
            m_hwnd, OBJID_WINDOW, dwNavDir, pvarEnd));

    FixUpScrollBarInfo(&sbi);

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
FindNext:
            lEndUp = varStart.lVal;

            while (++lEndUp <= INDEX_SCROLLBAR_MAC)
            {
                if (!(sbi.rgstate[lEndUp] & STATE_SYSTEM_INVISIBLE))
                    break;
            }

            if (lEndUp > INDEX_SCROLLBAR_MAC)
                lEndUp = 0;
            break;

        case NAVDIR_PREVIOUS:
FindPrevious:
            lEndUp = varStart.lVal;

            while (--lEndUp >= INDEX_SCROLLBAR_MIC)
            {
                if (!(sbi.rgstate[lEndUp] & STATE_SYSTEM_INVISIBLE))
                    break;
            }

            if (lEndUp < INDEX_SCROLLBAR_MIC)
                lEndUp = 0;
            break;

        case NAVDIR_UP:
            lEndUp = 0;
            if (m_fVertical)
                goto FindPrevious;
            break;

        case NAVDIR_LEFT:
            lEndUp = 0;
            if (!m_fVertical)
                goto FindPrevious;
            break;

        case NAVDIR_DOWN:
            lEndUp = 0;
            if (m_fVertical)
                goto FindNext;
            break;

        case NAVDIR_RIGHT:
            lEndUp = 0;
            if (!m_fVertical)
                goto FindNext;
            break;

        default:
            AssertStr( "Invalid NavDir" );
    }

    if (lEndUp != INDEX_SCROLLBAR_SELF)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEndUp;
        return(S_OK);
    }
    else
        return(S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::accHitTest(long xLeft, long yTop, VARIANT * pvarChild)
{
    POINT pt;
    SCROLLBARINFO sbi;
    int   xyPtAxis;
    int   xyScrollEnd;
    long  lHit;

    InitPvar(pvarChild);

    if (! MyGetScrollBarInfo(m_hwnd, (m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
          &sbi)   ||
        (sbi.rgstate[INDEX_SCROLLBAR_SELF] & (STATE_SYSTEM_OFFSCREEN | STATE_SYSTEM_INVISIBLE)))
    {
        return(S_FALSE);
    }

    pt.x = xLeft;
    pt.y = yTop;
    if (! PtInRect(&sbi.rcScrollBar, pt))
        return(S_FALSE);

    FixUpScrollBarInfo(&sbi);

    //
    // Convert to scrollbar coords.
    //
    if (m_fVertical)
    {
        xyPtAxis = yTop - sbi.rcScrollBar.top;
        xyScrollEnd = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
    }
    else
    {
        xyPtAxis = xLeft - sbi.rcScrollBar.left;
        xyScrollEnd = sbi.rcScrollBar.right - sbi.rcScrollBar.left;
    }

    lHit = INDEX_SCROLLBAR_SELF;

    if (xyPtAxis < sbi.dxyLineButton)
    {
        Assert(!(sbi.rgstate[INDEX_SCROLLBAR_UP] & STATE_SYSTEM_INVISIBLE));
        lHit = INDEX_SCROLLBAR_UP;
    }
    else if (xyPtAxis >= xyScrollEnd - sbi.dxyLineButton)
    {
        Assert(!(sbi.rgstate[INDEX_SCROLLBAR_DOWN] & STATE_SYSTEM_INVISIBLE));
        lHit = INDEX_SCROLLBAR_DOWN;
    }
    else if (!(sbi.rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_UNAVAILABLE))
    {
        if (xyPtAxis < sbi.xyThumbTop)
        {
            Assert(!(sbi.rgstate[INDEX_SCROLLBAR_UPPAGE] & STATE_SYSTEM_INVISIBLE));
            lHit = INDEX_SCROLLBAR_UPPAGE;
        }
        else if (xyPtAxis >= sbi.xyThumbBottom)
        {
            Assert(!(sbi.rgstate[INDEX_SCROLLBAR_DOWNPAGE] & STATE_SYSTEM_INVISIBLE));
            lHit = INDEX_SCROLLBAR_DOWNPAGE;
        }
        else
        {
            Assert(!(sbi.rgstate[INDEX_SCROLLBAR_THUMB] & STATE_SYSTEM_INVISIBLE));
            lHit = INDEX_SCROLLBAR_THUMB;
        }
    }

    pvarChild->vt = VT_I4;
    pvarChild->lVal = lHit;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CScrollBar::accDoDefaultAction()
//
//  Only works if the element is visible and available!
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::accDoDefaultAction(VARIANT varChild)
{
    WPARAM  wpAction;
    SCROLLBARINFO sbi;

    //
    // Validate params
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Is child available and present?
    //
    if (!MyGetScrollBarInfo(m_hwnd, (m_fVertical ? OBJID_VSCROLL : OBJID_HSCROLL),
        &sbi)   ||
        (sbi.rgstate[INDEX_SCROLLBAR_SELF] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_UNAVAILABLE)))
    {
        return(S_FALSE);
    }

    FixUpScrollBarInfo(&sbi);

    if (sbi.rgstate[varChild.lVal] & STATE_SYSTEM_UNAVAILABLE)
        return(S_FALSE);

    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_UP:
            wpAction = SB_LINEUP;
            break;

        case INDEX_SCROLLBAR_UPPAGE:
            wpAction = SB_PAGEUP;
            break;

        case INDEX_SCROLLBAR_DOWNPAGE:
            wpAction = SB_PAGEDOWN;
            break;

        case INDEX_SCROLLBAR_DOWN:
            wpAction = SB_LINEDOWN;
            break;

        default:
            return(E_NOT_APPLICABLE);
    }

    PostMessage(m_hwnd, (m_fVertical ? WM_VSCROLL : WM_HSCROLL),
        wpAction, (LPARAM)m_hwnd);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CScrollBar::put_accValue()
//
//  CALLER frees the string
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollBar::put_accValue(VARIANT varChild, BSTR szValue)
{
    long    lPos;
    HRESULT hr;

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
        return(E_NOT_APPLICABLE);

    hr = VarI4FromStr(szValue, 0, 0, &lPos);
    if (!SUCCEEDED(hr))
        return(hr);

    // Verify that we've got a valid percent value
    if( lPos < 0 || lPos > 100 )
        return E_INVALIDARG;

    int Min, Max;
    GetScrollRange( m_hwnd, SB_CTL, & Min, & Max );

    // work out value from percentage...
    lPos = Min + ( ( Max - Min ) * lPos ) / 100;

    SetScrollPos(m_hwnd, (m_fVertical ? SB_VERT : SB_HORZ), lPos, TRUE);

    return(S_OK);
}



/////////////////////////////////////////////////////////////////////////////
//
//  GRIP
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
//
//  CreateSizeGripObject()
//
//  EXTERNAL
//
// --------------------------------------------------------------------------
HRESULT CreateSizeGripObject(HWND hwnd, long idObject, REFIID riid, void** ppvGrip)
{
    return(CreateSizeGripThing(hwnd, idObject, riid, ppvGrip));
}


// --------------------------------------------------------------------------
//
//  CreateSizeGripThing()
//
//  INTERNAL
//
// --------------------------------------------------------------------------
HRESULT CreateSizeGripThing(HWND hwnd, long idObject, REFIID riid, void** ppvGrip)
{
    CSizeGrip * psizegrip;
    HRESULT     hr;

    UNUSED(idObject);

    InitPv(ppvGrip);

    psizegrip = new CSizeGrip();
    if (psizegrip)
    {
        if (! psizegrip->FInitialize(hwnd))
        {
            delete psizegrip;
            return(E_FAIL);
        }
    }
    else
        return(E_OUTOFMEMORY);

    hr = psizegrip->QueryInterface(riid, ppvGrip);
    if (!SUCCEEDED(hr))
        delete psizegrip;

    return(hr);
}




// --------------------------------------------------------------------------
//
//  CSizeGrip::FInitialize()
//
// --------------------------------------------------------------------------
BOOL CSizeGrip::FInitialize(HWND hwnd)
{
    m_hwnd = hwnd;

    return(IsWindow(hwnd));
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::get_accName(VARIANT varChild, BSTR * pszName)
{
    InitPv(pszName);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_SCROLLBAR_NAME+INDEX_SCROLLBAR_GRIP, pszName));
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::get_accDescription(VARIANT varChild, BSTR * pszDesc)
{
    InitPv(pszDesc);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_SCROLLBAR_DESCRIPTION+INDEX_SCROLLBAR_GRIP,
        pszDesc));
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::get_accRole(VARIANT varChild, VARIANT * pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_GRIP;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::get_accState(VARIANT varChild, VARIANT * pvarState)
{
    WINDOWINFO  wi;

    InitPvar(pvarState);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    //
    // We are only visible if both scrollbars are present.
    //
    if (! MyGetWindowInfo(m_hwnd, &wi)      ||
        !(wi.dwStyle & WS_VSCROLL)          ||
        !(wi.dwStyle & WS_HSCROLL))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    WINDOWINFO  wi;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (MyGetWindowInfo(m_hwnd, &wi)    &&
        (wi.dwStyle & WS_VSCROLL)       &&
        (wi.dwStyle & WS_HSCROLL))
    {
        *pxLeft = wi.rcClient.right;
        *pyTop = wi.rcClient.bottom;
        *pcxWidth = GetSystemMetrics(SM_CXVSCROLL);
        *pcyHeight = GetSystemMetrics(SM_CYHSCROLL);
    }
    else
        return(S_FALSE);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::accHitTest(long xLeft, long yTop, VARIANT * pvarChild)
{
    WINDOWINFO wi;

    InitPvar(pvarChild);

    if (MyGetWindowInfo(m_hwnd, &wi)  &&
        (wi.dwStyle & WS_VSCROLL)     &&
        (wi.dwStyle & WS_HSCROLL))
    {
        if ((xLeft >= wi.rcClient.right) &&
            (xLeft < wi.rcClient.right + GetSystemMetrics(SM_CXVSCROLL)) &&
            (yTop >= wi.rcClient.bottom) &&
            (yTop < wi.rcClient.bottom + GetSystemMetrics(SM_CYHSCROLL)))
        {
            pvarChild->vt = VT_I4;
            pvarChild->lVal = CHILDID_SELF;
            return(S_OK);
        }
    }

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::accNavigate(long dwNavFlags, VARIANT varStart,
    VARIANT * pvarEnd)
{
    InitPvar(pvarEnd);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varStart)   ||
        ! ValidateNavDir(dwNavFlags, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavFlags >= NAVDIR_FIRSTCHILD)
        return(S_FALSE);

    //
    // Navigation among peers only
    //
    return(GetParentToNavigate(OBJID_SIZEGRIP, m_hwnd, OBJID_WINDOW,
        dwNavFlags, pvarEnd));
}



// --------------------------------------------------------------------------
//
//  CSizeGrip::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CSizeGrip::Clone(IEnumVARIANT** ppenum)
{
    return(CreateSizeGripThing(m_hwnd, OBJID_SIZEGRIP, IID_IEnumVARIANT, (void**)ppenum));
}




/////////////////////////////////////////////////////////////////////////////
//
//  SCROLL CONTROL (Can be bar or grip)
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
//
//  CreateScrollBarClient()
//
//  Called from CClient creation
//
// --------------------------------------------------------------------------
HRESULT CreateScrollBarClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvScroll)
{
    CScrollCtl * pscroll;
    HRESULT hr;

    InitPv(ppvScroll);

    pscroll = new CScrollCtl(hwnd, idChildCur);
    if (!pscroll)
        return(E_OUTOFMEMORY);

    hr = pscroll->QueryInterface(riid, ppvScroll);
    if (!SUCCEEDED(hr))
        delete pscroll;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::CScrollCtl
//
// --------------------------------------------------------------------------
CScrollCtl::CScrollCtl(HWND hwnd, long idChildCur)
{
    long    lStyle;

    Initialize(hwnd, idChildCur);

    lStyle = GetWindowLong(hwnd, GWL_STYLE);
    if (lStyle & (SBS_SIZEBOX| SBS_SIZEGRIP))
    {
        m_fGrip = TRUE;
        m_cChildren = 0;
    }
    else
    {
        m_fUseLabel = TRUE;
        m_cChildren = CCHILDREN_SCROLLBAR;

        if (lStyle & SBS_VERT)
            m_fVertical = TRUE;
    }
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accName(varChild, pszName));

    Assert(!m_fGrip);

    return(HrCreateString(STR_SCROLLBAR_NAME + varChild.lVal +
        (m_fVertical ? 0 : INDEX_SCROLLBAR_HORIZONTAL), pszName));
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::get_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    long    lPos;

    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal || m_fGrip)
        return(E_NOT_APPLICABLE);

    lPos = GetScrollPos(m_hwnd, SB_CTL);
    int Min, Max;
    GetScrollRange( m_hwnd, SB_CTL, & Min, & Max );

    // work out a percent value...
    if( Min != Max )
        lPos = ( ( lPos - Min ) * 100 ) / ( Max - Min );
    else
        lPos = 0; // Prevent div-by-0

    return(VarBstrFromI4(lPos, 0, 0, pszValue));
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::get_accDescription(VARIANT varChild, BSTR* pszDesc)
{
    InitPv(pszDesc);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accDescription(varChild, pszDesc));

    Assert(!m_fGrip);

    return(HrCreateString(STR_SCROLLBAR_DESCRIPTION + varChild.lVal +
        (m_fVertical ? 0 : INDEX_SCROLLBAR_HORIZONTAL), pszDesc));
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    switch (varChild.lVal)
    {
        case 0:
            if (m_fGrip)
                pvarRole->lVal = ROLE_SYSTEM_GRIP;
            else
                pvarRole->lVal = ROLE_SYSTEM_SCROLLBAR;
            break;

        case INDEX_SCROLLBAR_UP:
        case INDEX_SCROLLBAR_DOWN:
        case INDEX_SCROLLBAR_UPPAGE:
        case INDEX_SCROLLBAR_DOWNPAGE:
            pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;
            break;

        case INDEX_SCROLLBAR_THUMB:
            pvarRole->lVal = ROLE_SYSTEM_INDICATOR;
            break;

        default:
            AssertStr( "Invalid ChildID for child of scroll bar" );
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    SCROLLBARINFO   sbi;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    Assert(!m_fGrip);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (!MyGetScrollBarInfo(m_hwnd, OBJID_CLIENT, &sbi))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }

    FixUpScrollBarInfo(&sbi);

    pvarState->lVal |= sbi.rgstate[INDEX_SCROLLBAR_SELF];
    pvarState->lVal |= sbi.rgstate[varChild.lVal];

    if ((varChild.lVal == INDEX_SCROLLBAR_THUMB) && (MyGetFocus() == m_hwnd))
        pvarState->lVal |= STATE_SYSTEM_FOCUSED;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::get_accDefaultAction(VARIANT varChild, BSTR* pszDefA)
{
    InitPv(pszDefA);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_UP:
        case INDEX_SCROLLBAR_UPPAGE:
        case INDEX_SCROLLBAR_DOWNPAGE:
        case INDEX_SCROLLBAR_DOWN:
            return(HrCreateString(STR_BUTTON_PUSH, pszDefA));
    }

    return(E_NOT_APPLICABLE);
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    SCROLLBARINFO   sbi;
    int             dxyButton;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    Assert(!m_fGrip);

    if (!MyGetScrollBarInfo(m_hwnd, OBJID_CLIENT, &sbi))
        return(S_FALSE);

    FixUpScrollBarInfo(&sbi);
    if (sbi.rgstate[varChild.lVal] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN))
        return(S_FALSE);

    if (m_fVertical)
        dxyButton = sbi.rcScrollBar.right - sbi.rcScrollBar.left;
    else
        dxyButton = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
            
    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_UP:
        case INDEX_SCROLLBAR_DOWN:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;
                *pcyHeight = sbi.dxyLineButton;

                if (varChild.lVal == INDEX_SCROLLBAR_UP)
                    *pyTop = sbi.rcScrollBar.top;
                else
                    *pyTop = sbi.rcScrollBar.bottom - *pcyHeight;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;
                *pcxWidth = sbi.dxyLineButton;

                if (varChild.lVal == INDEX_SCROLLBAR_UP)
                    *pxLeft = sbi.rcScrollBar.left;
                else
                    *pxLeft = sbi.rcScrollBar.right - *pcxWidth;
            }
            break;

        case INDEX_SCROLLBAR_UPPAGE:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;

                *pyTop = sbi.rcScrollBar.top + sbi.dxyLineButton;
                *pcyHeight = sbi.xyThumbTop - sbi.dxyLineButton;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;

                *pxLeft = sbi.rcScrollBar.left + sbi.dxyLineButton;
                *pcxWidth = sbi.xyThumbTop - sbi.dxyLineButton;
            }
            break;

        case INDEX_SCROLLBAR_DOWNPAGE:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;

                *pyTop = sbi.rcScrollBar.top + sbi.xyThumbBottom;
                *pcyHeight = (sbi.rcScrollBar.bottom - sbi.rcScrollBar.top) -
                    sbi.xyThumbBottom - sbi.dxyLineButton;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;

                *pxLeft = sbi.rcScrollBar.left + sbi.xyThumbBottom;
                *pcxWidth = (sbi.rcScrollBar.right - sbi.rcScrollBar.left) -
                    sbi.xyThumbBottom - sbi.dxyLineButton;
            }
            break;

        case INDEX_SCROLLBAR_THUMB:
            if (m_fVertical)
            {
                *pxLeft = sbi.rcScrollBar.left;
                *pcxWidth = dxyButton;

                *pyTop = sbi.rcScrollBar.top + sbi.xyThumbTop;
                *pcyHeight = sbi.xyThumbBottom - sbi.xyThumbTop;
            }
            else
            {
                *pyTop = sbi.rcScrollBar.top;
                *pcyHeight = dxyButton;

                *pxLeft = sbi.rcScrollBar.left + sbi.xyThumbTop;
                *pcxWidth = sbi.xyThumbBottom - sbi.xyThumbTop;
            }
            break;

        default:
            AssertStr( "Invalid ChildID for child of scroll bar" );
    }

    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  CScrollCtl::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::accNavigate(long dwNavDir, VARIANT varStart, VARIANT* pvarEnd)
{
    long    lEndUp = 0;
    SCROLLBARINFO   sbi;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (!varStart.lVal)
    {
        if (dwNavDir < NAVDIR_FIRSTCHILD)
            return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

        if (!m_fGrip)
            return(S_FALSE);

        if (dwNavDir == NAVDIR_FIRSTCHILD)
            dwNavDir = NAVDIR_NEXT;
        else
        {
            dwNavDir = NAVDIR_PREVIOUS;
            varStart.lVal = m_cChildren + 1;
        }
    }

    Assert(!m_fGrip);

    if (!MyGetScrollBarInfo(m_hwnd, OBJID_CLIENT, &sbi))
        return(S_FALSE);

    FixUpScrollBarInfo(&sbi);

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
FindNext:
            lEndUp = varStart.lVal;

            while (++lEndUp <= INDEX_SCROLLBAR_MAC)
            {
                if (!(sbi.rgstate[lEndUp] & STATE_SYSTEM_INVISIBLE))
                    break;
            }

            if (lEndUp > INDEX_SCROLLBAR_MAC)
                lEndUp = 0;
            break;

        case NAVDIR_PREVIOUS:
FindPrevious:
            lEndUp = varStart.lVal;

            while (--lEndUp >= INDEX_SCROLLBAR_MIC)
            {
                if (!(sbi.rgstate[lEndUp] & STATE_SYSTEM_INVISIBLE))
                    break;
            }

            if (lEndUp < INDEX_SCROLLBAR_MIC)
                lEndUp = 0;
            break;

        case NAVDIR_UP:
            lEndUp = 0;
            if (m_fVertical)
                goto FindPrevious;
            break;

        case NAVDIR_LEFT:
            lEndUp = 0;
            if (!m_fVertical)
                goto FindPrevious;
            break;

        case NAVDIR_DOWN:
            lEndUp = 0;
            if (m_fVertical)
                goto FindNext;
            break;

        case NAVDIR_RIGHT:
            lEndUp = 0;
            if (!m_fVertical)
                goto FindNext;
            break;

        default:
            AssertStr( "Invalid NavDir" );
    }

    if (lEndUp != INDEX_SCROLLBAR_SELF)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEndUp;
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::accHitTest(long xLeft, long yTop, VARIANT* pvarHit)
{
    HRESULT hr;
    SCROLLBARINFO sbi;
    int     xyPtAxis;
    int     xyScrollEnd;

    //
    // Is this in our client area at all?
    //
    hr = CClient::accHitTest(xLeft, yTop, pvarHit);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarHit->vt != VT_I4) || (pvarHit->lVal != 0) || m_fGrip)
        return(hr);

    //
    // We only get here if this is a scrollbar control (not a grip)
    //
    if (!MyGetScrollBarInfo(m_hwnd, OBJID_CLIENT, &sbi))
        return(S_OK);

    FixUpScrollBarInfo(&sbi);

    //
    // Convert to scrollbar coords.
    //
    //
    // Convert to scrollbar coords.
    //
    if (m_fVertical)
    {
        xyPtAxis = yTop - sbi.rcScrollBar.top;
        xyScrollEnd = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
    }
    else
    {
        xyPtAxis = xLeft - sbi.rcScrollBar.left;
        xyScrollEnd = sbi.rcScrollBar.right - sbi.rcScrollBar.left;
    }

    if (xyPtAxis < sbi.dxyLineButton)
    {
        Assert(!(sbi.rgstate[INDEX_SCROLLBAR_UP] & STATE_SYSTEM_INVISIBLE));
        pvarHit->lVal = INDEX_SCROLLBAR_UP;
    }
    else if (xyPtAxis >= xyScrollEnd - sbi.dxyLineButton)
    {
        Assert(!(sbi.rgstate[INDEX_SCROLLBAR_DOWN] & STATE_SYSTEM_INVISIBLE));
        pvarHit->lVal = INDEX_SCROLLBAR_DOWN;
    }
    else if (!(sbi.rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_UNAVAILABLE))
    {
        if (xyPtAxis < sbi.xyThumbTop)
        {
            Assert(!(sbi.rgstate[INDEX_SCROLLBAR_UPPAGE] & STATE_SYSTEM_INVISIBLE));
            pvarHit->lVal = INDEX_SCROLLBAR_UPPAGE;
        }
        else if (xyPtAxis >= sbi.xyThumbBottom)
        {
            Assert(!(sbi.rgstate[INDEX_SCROLLBAR_DOWNPAGE] & STATE_SYSTEM_INVISIBLE));
            pvarHit->lVal = INDEX_SCROLLBAR_DOWNPAGE;
        }
        else
        {
            Assert(!(sbi.rgstate[INDEX_SCROLLBAR_THUMB] & STATE_SYSTEM_INVISIBLE));
            pvarHit->lVal = INDEX_SCROLLBAR_THUMB;
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CScrollCtl::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::accDoDefaultAction(VARIANT varChild)
{
    WPARAM          wpAction = 0;
    SCROLLBARINFO   sbi;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal || (varChild.lVal == INDEX_SCROLLBAR_THUMB))
        return(E_NOT_APPLICABLE);

    Assert(!m_fGrip);

    if (!MyGetScrollBarInfo(m_hwnd, OBJID_CLIENT, &sbi) ||
        (sbi.rgstate[INDEX_SCROLLBAR_SELF] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_UNAVAILABLE)))
    {
        return(S_FALSE);
    }

    FixUpScrollBarInfo(&sbi);

    if (sbi.rgstate[varChild.lVal] & STATE_SYSTEM_UNAVAILABLE)
        return(S_FALSE);

    switch (varChild.lVal)
    {
        case INDEX_SCROLLBAR_UP:
            wpAction = SB_LINEUP;
            break;

        case INDEX_SCROLLBAR_UPPAGE:
            wpAction = SB_PAGEUP;
            break;

        case INDEX_SCROLLBAR_DOWNPAGE:
            wpAction = SB_PAGEDOWN;
            break;

        case INDEX_SCROLLBAR_DOWN:
            wpAction = SB_LINEDOWN;
            break;

        default:
            AssertStr( "Invalid ChildID for child of scroll bar" );
    }                     

    SendMessage(GetParent(m_hwnd), (m_fVertical ? WM_VSCROLL : WM_HSCROLL),
        wpAction, (LPARAM)m_hwnd);

    return(S_OK);

}



// --------------------------------------------------------------------------
//
//  CScrollCtl::put_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CScrollCtl::put_accValue(VARIANT varChild, BSTR szValue)
{
    long    lPos;
    HRESULT hr;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal || m_fGrip)
        return(E_NOT_APPLICABLE);

    hr = VarI4FromStr(szValue, 0, 0, &lPos);
    if (!SUCCEEDED(hr))
        return(hr);

    // Verify that we've got a valid percent value
    if( lPos < 0 || lPos > 100 )
        return E_INVALIDARG;

    int Min, Max;
    GetScrollRange( m_hwnd, SB_CTL, & Min, & Max );

    // work out value from percentage...
    lPos = Min + ( ( Max - Min ) * lPos ) / 100;

    SetScrollPos(m_hwnd, SB_CTL, lPos, TRUE);

    return(S_OK);
}
