// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TITLEBAR.CPP
//
//  Title bar class.
//
//  NOTE:
//  Every time we make a drawing, hittesting change to USER for titlebars,
//  update this file also!  I.E. when you
//      (1) Add a titlebar element like a new button
//      (2) Change the spacing like margins
//      (3) Add a new type of titlebar beyond normal/small
//      (4) Shuffle the layout
//
//  ISSUES:
//      (1) Need "button down" info from USER and hence a shared <oleuser.h>
//      (2) Need "hovered" info from USER
//      (3) For FE, we need a SC_IME system command.  The TrackIMEButton()
//          code does the command in line, unlike all other titlebar buttons.
//          This makes it not programmable.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "window.h"
#include "client.h"
#include "titlebar.h"


// --------------------------------------------------------------------------
//
//  CreateTitleBarObject()
//
// --------------------------------------------------------------------------
HRESULT CreateTitleBarObject(HWND hwnd, long idObject, REFIID riid, void** ppvTitle)
{
    UNUSED(idObject);

    return(CreateTitleBarThing(hwnd, 0, riid, ppvTitle));
}


// --------------------------------------------------------------------------
//
//  CreateTitleBarThing()
//
// --------------------------------------------------------------------------
HRESULT CreateTitleBarThing(HWND hwnd, long iChildCur, REFIID riid, void** ppvTitle)
{
    CTitleBar * ptitlebar;
    HRESULT hr;

    InitPv(ppvTitle);

    ptitlebar = new CTitleBar();
    if (ptitlebar)
    {
        if (! ptitlebar->FInitialize(hwnd, iChildCur))
        {
            delete ptitlebar;
            return(E_FAIL);
        }
    }
    else
        return(E_OUTOFMEMORY);

    hr = ptitlebar->QueryInterface(riid, ppvTitle);
    if (!SUCCEEDED(hr))
        delete ptitlebar;

    return(hr);
}




// --------------------------------------------------------------------------
//
//  GetRealChild()
//
// --------------------------------------------------------------------------
long GetRealChild(DWORD dwStyle, LONG lChild)
{
    switch (lChild)
    {
        case INDEX_TITLEBAR_MINBUTTON:
            if (dwStyle & WS_MINIMIZE)
                lChild = INDEX_TITLEBAR_RESTOREBUTTON;
            break;

        case INDEX_TITLEBAR_MAXBUTTON:
            if (dwStyle & WS_MAXIMIZE)
                lChild = INDEX_TITLEBAR_RESTOREBUTTON;
            break;
    }

    return(lChild);
}



// --------------------------------------------------------------------------
//
//  CTitleBar::FInitialize
//
// --------------------------------------------------------------------------
BOOL CTitleBar::FInitialize(HWND hwndTitleBar, LONG iChildCur)
{
    if (! IsWindow(hwndTitleBar))
        return(FALSE);

    m_hwnd = hwndTitleBar;
    m_cChildren = CCHILDREN_TITLEBAR;
    m_idChildCur = iChildCur;

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  CTitleBar::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::Clone(IEnumVARIANT ** ppenum)
{
    return(CreateTitleBarThing(m_hwnd, m_idChildCur, IID_IEnumVARIANT, (void**)ppenum));
}



// --------------------------------------------------------------------------
//
//  CTitleBar::get_accName()
//
//  Returns the proper noun name of the object.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::get_accName(VARIANT varChild, BSTR * pszName)
{
    long    index;
    LONG    dwStyle;

    InitPv(pszName);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    // The titlebar doesn't have a name itself
    if (!varChild.lVal)
        return(S_FALSE);

    //
    // Figure out what string to _really_ load (depends on window state)
    //
    dwStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    index = GetRealChild(dwStyle, varChild.lVal);

    return(HrCreateString(STR_TITLEBAR_NAME+index, pszName));
}



// --------------------------------------------------------------------------
//
//  CTitleBar::get_accValue()
//
//  The value of the titlebar itself is the text inside.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Only the titlebar has a value, the child buttons don't.
    //
    if (varChild.lVal)
        //CWO, 1/16/97, Changed to S_FALSE from E_NOT_APPLICABLE
        return(S_FALSE);

    return(HrGetWindowName(m_hwnd, FALSE, pszValue));
}




// --------------------------------------------------------------------------
//
//  CTitleBar::get_accDescription()
//
//  Returns a full sentence describing the object.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::get_accDescription(VARIANT varChild, BSTR * pszDesc)
{
    long    index;
    LONG    dwStyle;

    InitPv(pszDesc);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // Figure out what string to _really_ load, depends on state.
    //
    dwStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    index = GetRealChild(dwStyle, varChild.lVal);

    return(HrCreateString(STR_TITLEBAR_DESCRIPTION+index, pszDesc));
}



// --------------------------------------------------------------------------
//
//  CTitleBar::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::get_accRole(VARIANT varChild, VARIANT * pvarRole)
{
    InitPvar(pvarRole);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (varChild.lVal == INDEX_TITLEBAR_SELF)
        pvarRole->lVal = ROLE_SYSTEM_TITLEBAR;
    else
        pvarRole->lVal = ROLE_SYSTEM_PUSHBUTTON;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTitleBar::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    TITLEBARINFO    ti;

    InitPvar(pvarState);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (! MyGetTitleBarInfo(m_hwnd, &ti) ||
        (ti.rgstate[INDEX_TITLEBAR_SELF] & STATE_SYSTEM_INVISIBLE))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_OK);
    }

    pvarState->lVal |= ti.rgstate[INDEX_TITLEBAR_SELF];
    pvarState->lVal |= ti.rgstate[varChild.lVal];

	// only the title bar itself is focusable.
	if (varChild.lVal != INDEX_TITLEBAR_SELF)
		pvarState->lVal &= ~STATE_SYSTEM_FOCUSABLE;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTitleBar::get_accDefaultAction()
//
//  NOTE:  only the buttons have default actions.  The default action of
//  the system menu is ambiguous, since it is unknown until the window
//  enters menu mode.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::get_accDefaultAction(VARIANT varChild, BSTR*
    pszDefAction)
{
    long index;
    LONG dwStyle;

    InitPv(pszDefAction);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(E_NOT_APPLICABLE);

    //
    // Get the string
    //
    dwStyle = GetWindowLong(m_hwnd, GWL_STYLE);
    index = GetRealChild(dwStyle, varChild.lVal);

    //
    // BOGUS!  The IME button doesn't have a def action either since
    // we can't change the keyboard layout indirectly via WM_SYSCOMMAND.
    // The IME code does the work in line.  We need to make an SC_.
    //
    if (index <= INDEX_TITLEBAR_IMEBUTTON)
        return(E_NOT_APPLICABLE);

    return(HrCreateString(STR_BUTTON_PUSH, pszDefAction));
}


// --------------------------------------------------------------------------
//
//  CTitleBar::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::accSelect(long flagsSel, VARIANT varChild)
{
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! ValidateSelFlags(flagsSel))
        return(E_INVALIDARG);

	if (flagsSel & SELFLAG_TAKEFOCUS)
	{
		if (varChild.lVal == CHILDID_SELF)
		{
			if (MyGetAncestor(m_hwnd, GA_PARENT) == GetDesktopWindow())
				SetForegroundWindow(m_hwnd);
			else
				SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			return (S_OK);
		}
	}

    return(E_NOT_APPLICABLE);
}

// --------------------------------------------------------------------------
//
//  CTitleBar::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT * pvarEnd)
{
    TITLEBARINFO    ti;
    long        lEndUp;

    InitPvar(pvarEnd);

    if (! ValidateChild(&varStart) ||
        ! ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (! MyGetTitleBarInfo(m_hwnd, &ti))
        return(S_FALSE);

    if (dwNavDir == NAVDIR_FIRSTCHILD)
        dwNavDir = NAVDIR_NEXT;
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        dwNavDir = NAVDIR_PREVIOUS;
        varStart.lVal = m_cChildren + 1;
    }
    else if (varStart.lVal == INDEX_TITLEBAR_SELF)
        return(GetParentToNavigate(OBJID_TITLEBAR, m_hwnd,
            OBJID_WINDOW, dwNavDir, pvarEnd));

    //
    // NOTE:  It is up to the caller to make sure the item navigation
    // is starting from is visible.
    //
    switch (dwNavDir)
    {
        case NAVDIR_LEFT:
        case NAVDIR_PREVIOUS:
            // 
            // Is there anything to the left of us?
            //
            lEndUp = varStart.lVal;
            while (--lEndUp >= INDEX_TITLEBAR_MIC)
            {
                if (!(ti.rgstate[lEndUp] & STATE_SYSTEM_INVISIBLE))
                    break;
            }

            if (lEndUp < INDEX_TITLEBAR_MIC)
                lEndUp = 0;
            break;

        case NAVDIR_RIGHT:
        case NAVDIR_NEXT:
            //
            // Is there anything to the right of us?
            //
            lEndUp = varStart.lVal;
            while (++lEndUp <= INDEX_TITLEBAR_MAC)
            {
                if (!(ti.rgstate[lEndUp] & STATE_SYSTEM_INVISIBLE))
                    break;
            }

            if (lEndUp > INDEX_TITLEBAR_MAC)
                lEndUp = 0;
            break;

        default:
            lEndUp = 0;
            break;
    }

    if (lEndUp)
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
//  CTitleBar::accLocation()
//
//  Gets the screen rect of a particular element.  If the item isn't
//  actually present, this will fail.
//
//  NOTE:  It is up to the caller to make sure that the container (titlebar)
//  is visible before calling accLocation on a child.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    int     cyBorder;
    int     cxyButton;
    TITLEBARINFO ti;
    int     index;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (! MyGetTitleBarInfo(m_hwnd, &ti))
        return(S_FALSE);

    //
    // If this object isn't around, fail.
    //
    if ((ti.rgstate[INDEX_TITLEBAR_SELF] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN)) ||
        (ti.rgstate[varChild.lVal] & STATE_SYSTEM_INVISIBLE))
    {
        return(S_FALSE);
    }

    cyBorder = GetSystemMetrics(SM_CYBORDER);
    cxyButton = ti.rcTitleBar.bottom - ti.rcTitleBar.top - cyBorder;

    if (varChild.lVal == INDEX_TITLEBAR_SELF)
    {
        *pxLeft     = ti.rcTitleBar.left;
        *pyTop      = ti.rcTitleBar.top;
        *pcxWidth   = ti.rcTitleBar.right - ti.rcTitleBar.left;
        *pcyHeight  = ti.rcTitleBar.bottom - ti.rcTitleBar.top;
    }
    else
    {
        *pyTop      = ti.rcTitleBar.top;
        *pcxWidth   = cxyButton;
        *pcyHeight  = cxyButton;

        // Where is the left side of the button?  Our INDEX_s are
        // conveniently defined in left-to-right order.  Start at the
        // end and work backwards to the system menu.  Subtract cxyButton
        // when a child is present.
        *pxLeft     = ti.rcTitleBar.right - cxyButton;
        for (index = INDEX_TITLEBAR_MAC; index > INDEX_TITLEBAR_SELF; index--)
        {
            if (index == varChild.lVal)
                break;

            if (!(ti.rgstate[index] & STATE_SYSTEM_INVISIBLE))
                *pxLeft -= cxyButton;
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTitleBar::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::accHitTest(long xLeft, long yTop, VARIANT* pvarChild)
{
    POINT   pt;
    int     cxyButton;
    int     index;
    TITLEBARINFO    ti;

    InitPvar(pvarChild);

    if (! MyGetTitleBarInfo(m_hwnd, &ti) ||
        (ti.rgstate[INDEX_TITLEBAR_SELF] & (STATE_SYSTEM_INVISIBLE || STATE_SYSTEM_OFFSCREEN)))
        return(S_FALSE);

    pt.x = xLeft;
    pt.y = yTop;

    // 
    // We return VT_EMPTY when the point isn't in the titlebar at all.
    //
    if (! PtInRect(&ti.rcTitleBar, pt))
        return(S_FALSE);

    pvarChild->vt = VT_I4;
    pvarChild->lVal = INDEX_TITLEBAR_SELF;

    cxyButton = ti.rcTitleBar.bottom - ti.rcTitleBar.top - GetSystemMetrics(SM_CYBORDER);

    // If yTop is greater than this, the point is on the border drawn below
    // the caption
    if (yTop < ti.rcTitleBar.top + cxyButton)
    {
        //
        // Start at the right side and work backwards.
        //
        pt.x = ti.rcTitleBar.right - cxyButton;

        for (index = INDEX_TITLEBAR_MAC; index > INDEX_TITLEBAR_SELF; index--)
        {
            //
            // This child is here.
            //
            if (!(ti.rgstate[index] & STATE_SYSTEM_INVISIBLE))
            {
                //
                // Is this point where this child is?
                //
                if (xLeft >= pt.x)
                {
                    pvarChild->lVal = index;
                    break;
                }

                pt.x -= cxyButton;
            }
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTitleBar::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTitleBar::accDoDefaultAction(VARIANT varChild)
{
    WPARAM  scCommand = 0;
    int     index;
    TITLEBARINFO    ti;

    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // We return nothing for the titlebar & system menu objects.  Hence it
    // is a real error to attempt to do the default action  on them.
    //
    if (varChild.lVal <= INDEX_TITLEBAR_IMEBUTTON)
        return(E_NOT_APPLICABLE);

    if (! MyGetTitleBarInfo(m_hwnd, &ti) ||
        (ti.rgstate[INDEX_TITLEBAR_SELF] & STATE_SYSTEM_INVISIBLE))
        return(S_FALSE);


    //
    // We do NOT do the default action of an object that is invisible.
    //
    if (ti.rgstate[varChild.lVal] & STATE_SYSTEM_INVISIBLE)
        return(S_FALSE);

    index = GetRealChild(GetWindowLong(m_hwnd, GWL_STYLE), varChild.lVal);

    switch (index)
    {
        case INDEX_TITLEBAR_MINBUTTON:
            scCommand = SC_MINIMIZE;
            break;

        case INDEX_TITLEBAR_HELPBUTTON:
            scCommand = SC_CONTEXTHELP;
            break;

        case INDEX_TITLEBAR_MAXBUTTON:
            scCommand = SC_MAXIMIZE;
            break;

        case INDEX_TITLEBAR_RESTOREBUTTON:
            scCommand = SC_RESTORE;
            break;

        case INDEX_TITLEBAR_CLOSEBUTTON:
            scCommand = SC_CLOSE;
            break;

        default:
            AssertStr( "Invalid ChildID for child of titlebar" );
    }

    //
    // Context help puts into a modal loop, which will block the calling
    // thread until the loop ends.  Hence we post this instead of sending it.
    //
    // Note that we will no doubt do something similar in menus.
    //
    PostMessage(m_hwnd, WM_SYSCOMMAND, scCommand, 0L);
    return(S_OK);
}

