// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  WINDOW.CPP
//
//  Window class.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "window.h"


#define MaskBit(n)                  (1 << (n))

#define IndexFromNavDir(navdir)     (navdir - NAVDIR_UP)

// Remember, these are negative!
#define OBJID_WINDOW_FIRST      OBJID_SIZEGRIP
#define OBJID_WINDOW_LAST       OBJID_SYSMENU

typedef struct tagNAVIGATE
{
    long    NavPeer[4];
} NAVIGATE;

#ifndef CCHILDREN_FRAME
#define CCHILDREN_FRAME  7
#endif

// Order is Up, Down, Left, Right
NAVIGATE    rgFrameNavigate[CCHILDREN_FRAME] =
{
    // System menu
    {
        0, IndexFromObjid(OBJID_MENU), 0, IndexFromObjid(OBJID_TITLEBAR)
    },

    // Title bar
    {
        0, IndexFromObjid(OBJID_MENU), IndexFromObjid(OBJID_SYSMENU), 0
    },

    // Menu bar
    {
        IndexFromObjid(OBJID_TITLEBAR), IndexFromObjid(OBJID_CLIENT), 0, 0
    },

    // Client
    {
        IndexFromObjid(OBJID_MENU), IndexFromObjid(OBJID_HSCROLL), 0, IndexFromObjid(OBJID_VSCROLL)
    },

    // Vertical scrollbar
    {
        IndexFromObjid(OBJID_MENU), IndexFromObjid(OBJID_SIZEGRIP), IndexFromObjid(OBJID_CLIENT), 0
    },

    // Horizontal scrollbar
    {
        IndexFromObjid(OBJID_CLIENT), 0, 0, IndexFromObjid(OBJID_SIZEGRIP)
    },

    // Size grip
    {
        IndexFromObjid(OBJID_VSCROLL), 0, IndexFromObjid(OBJID_HSCROLL), 0
    }
};



// --------------------------------------------------------------------------
//
//  CreateWindowObject()
//
//  External function for CreateDefault...
//
// --------------------------------------------------------------------------
HRESULT CreateWindowObject(HWND hwnd, long idObject, REFIID riid, void** ppvWindow)
{
    UNUSED(idObject);

    InitPv(ppvWindow);

    if (!IsWindow(hwnd))
        return(E_FAIL);

    // Look for (and create) a suitable proxy/handler if one
    // exists. Use CreateWindowThing as default if none found.
    // (TRUE => use window, as opposed to client, classes)
    return FindAndCreateWindowClass( hwnd, TRUE, CreateWindowThing,
                                     riid, 0, ppvWindow );
}


// --------------------------------------------------------------------------
//
//  CreateWindowThing()
//
//  Private function that uses atom type to decide what class of window
//  this is.  If there is a private create function, uses that one.  Else
//  uses generic window frame handler.
//
// --------------------------------------------------------------------------
HRESULT CreateWindowThing(HWND hwnd, long idChildCur, REFIID riid, void** ppvWindow)
{
    CWindow * pwindow;
    HRESULT     hr;

    InitPv(ppvWindow);

    pwindow = new CWindow();
    if (!pwindow)
        return(E_OUTOFMEMORY);

    // Can't be in the constructor--derived classes can't call the init
    // code if so.
    pwindow->Initialize(hwnd, idChildCur);

    hr = pwindow->QueryInterface(riid, ppvWindow);
    if (!SUCCEEDED(hr))
        delete pwindow;

    return(hr);
}


// --------------------------------------------------------------------------
//
//  CWindow::Initialize()
//
// --------------------------------------------------------------------------
void CWindow::Initialize(HWND hwnd, LONG iChild)
{
    m_hwnd = hwnd;
    m_cChildren = CCHILDREN_FRAME;
    m_idChildCur = iChild;
}



// --------------------------------------------------------------------------
//
//  CWindow::ValidateChild()
//
//  The window children are the OBJID_s of the elements that compose the
//  frame.  These are NEGATIVE values.  Hence we override the validation.
//
// --------------------------------------------------------------------------
BOOL CWindow::ValidateChild(VARIANT* pvar)
{
    //
    // This validates a VARIANT parameter and translates missing/empty
    // params.
    //

TryAgain:
    // Missing parameter, a la VBA
    switch (pvar->vt)
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy(pvar, pvar->pvarVal);
            goto TryAgain;

        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

// remove this! VT_I2 is not valid!!
#ifdef  VT_I2_IS_VALID  // it isn't now...
        case VT_I2:
            pvar->vt = VT_I4;
            pvar->lVal = (long)pvar->iVal;
            // FALL THROUGH
#endif

        case VT_I4:
            if ((pvar->lVal > 0) || (pvar->lVal < (long)OBJID_WINDOW_FIRST))
                return(FALSE);
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  CWindow::get_accParent()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accParent(IDispatch ** ppdispParent)
{
    HWND    hwndParent;

    InitPv(ppdispParent);

    hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
    if (! hwndParent)
        return(S_FALSE);

    return(AccessibleObjectFromWindow(hwndParent, OBJID_CLIENT, IID_IDispatch,
        (void **)ppdispParent));
}



// --------------------------------------------------------------------------
//
//  CWindow::get_accChild()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
    InitPv(ppdispChild);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
        IID_IDispatch, (void**)ppdispChild));
}



// --------------------------------------------------------------------------
//
//  CWindow::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accName(VARIANT varChild, BSTR* pszName)
{
    IAccessible * poleacc;
    HRESULT hr;

    InitPv(pszName);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // If the caller want's our name, forward to the client object
    //
    if (varChild.lVal == CHILDID_SELF)
        varChild.lVal = OBJID_CLIENT;


    //
    // Get the name of our child frame object.
    //
    poleacc = NULL;
    hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
        IID_IAccessible, (void **)&poleacc);
    if (!SUCCEEDED(hr))
        return(hr);

    varChild.lVal = CHILDID_SELF;
    hr = poleacc->get_accName(varChild, pszName);
    poleacc->Release();

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CWindow::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accDescription(VARIANT varChild, BSTR* pszDesc)
{
    InitPv(pszDesc);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == CHILDID_SELF)
    {
        return(S_FALSE);
    }
    else
    {
        IAccessible * poleacc;
        HRESULT hr;

        //
        // Get the description of our child frame object.
        //
        poleacc = NULL;
        hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal, IID_IAccessible,
            (void **)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);
        if (!poleacc)
            return(S_FALSE);

        varChild.lVal = CHILDID_SELF;
        hr = poleacc->get_accDescription(varChild, pszDesc);
        poleacc->Release();

        return(hr);
    }

}



// --------------------------------------------------------------------------
//
//  CWindow::get_accHelp()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    InitPv(pszHelp);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == CHILDID_SELF)
        return(E_NOT_APPLICABLE);
    else
    {
        IAccessible * poleacc;
        HRESULT hr;

        //
        // Get the help for our child frame object.
        //
        poleacc = NULL;
        hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
            IID_IAccessible, (void **)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);
        if (!poleacc)
            return(S_FALSE);

        varChild.lVal = CHILDID_SELF;
        hr = poleacc->get_accHelp(varChild, pszHelp);
        poleacc->Release();

        return(hr);
    }

}



// --------------------------------------------------------------------------
//
//  CWindow::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == CHILDID_SELF)
    {
        //
        // Fill in our role.
        //
        pvarRole->vt = VT_I4;
        pvarRole->lVal = ROLE_SYSTEM_WINDOW;
    }
    else
    {
        IAccessible * poleacc;
        HRESULT hr;

        //
        // Get the role of our child frame object.
        //
        poleacc = NULL;
        hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
            IID_IAccessible, (void **)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);
        if (!poleacc)
            return(S_FALSE);

        varChild.lVal = CHILDID_SELF;
        hr = poleacc->get_accRole(varChild, pvarRole);
        poleacc->Release();

        return(hr);
    }

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CWindow::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    HWND    hwndParent;

    InitPvar(pvarState);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (varChild.lVal == CHILDID_SELF)
    {
        //
        // Get our state.
        //
        WINDOWINFO  wi;
        RECT        rcParent;

        if (! MyGetWindowInfo(m_hwnd, &wi))
        {
            pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
            return(S_OK);
        }

        if (!(wi.dwStyle & WS_VISIBLE))
            pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

        if (wi.dwStyle & WS_DISABLED)
            pvarState->lVal |= STATE_SYSTEM_UNAVAILABLE;

        if (wi.dwStyle & WS_THICKFRAME)
            pvarState->lVal |= STATE_SYSTEM_SIZEABLE;

        if ((wi.dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            pvarState->lVal |= STATE_SYSTEM_MOVEABLE;
            pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
        }

// Windows are not selectable, so they shouldn't be selected either.
#if 0
        if (wi.dwWindowStatus & WS_ACTIVECAPTION)
            pvarState->lVal |= STATE_SYSTEM_SELECTED;
#endif

        if (MyGetFocus() == m_hwnd)
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;

        if (GetForegroundWindow() == MyGetAncestor(m_hwnd, GA_ROOT))
            pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        // This is the _real_ parent window.
        if (hwndParent = MyGetAncestor(m_hwnd, GA_PARENT))
        {
            MyGetRect(hwndParent, &rcParent, FALSE);
            MapWindowPoints(hwndParent, NULL, (LPPOINT)&rcParent, 2);

			// SMD 09/16/97 Offscreen things are things never on the screen,
			// and that doesn't apply to this. Changed from OFFSCREEN to
			// INVISIBLE.
            if (! IntersectRect(&rcParent, &rcParent, &wi.rcWindow))
                pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        }
    }
    else
    {
        IAccessible * poleacc;
        HRESULT hr;

        //
        // Ask the frame element what its state is.
        //
        poleacc = NULL;
        hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
            IID_IAccessible, (void **)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);
        if (!poleacc)
        {
            pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
            return(S_OK);
        }

        varChild.lVal = CHILDID_SELF;
        hr = poleacc->get_accState(varChild, pvarState);
        poleacc->Release();

        return(hr);
    }

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CWindow::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
    IAccessible * poleacc;
    HRESULT hr;

    InitPv(pszShortcut);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    //
    // If the caller is asking us for our shortcut, forward to the client.
    //
    if (varChild.lVal == CHILDID_SELF)
        varChild.lVal = OBJID_CLIENT;

    //
    // Ask the child.
    //
    poleacc = NULL;
    hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
        IID_IAccessible, (void **)&poleacc);
    if (!SUCCEEDED(hr))
        return(hr);

    varChild.lVal = CHILDID_SELF;
    hr = poleacc->get_accKeyboardShortcut(varChild, pszShortcut);
    poleacc->Release();

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CWindow::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::get_accFocus(VARIANT* pvarChild)
{
    HWND    hwndFocus;

    InitPvar(pvarChild);

    //
    // BOGUS!  If we are in menu mode, then menu object has focus.  If
    // we are in scrolling mode, scrollbar has the focus.  etc.
    //
    hwndFocus = MyGetFocus();

    if ((m_hwnd == hwndFocus) || IsChild(m_hwnd, hwndFocus))
        return(GetNoncObject(m_hwnd, OBJID_CLIENT, pvarChild));

    return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CWindow::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    InitPvar(pvarEnd);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varStart)   ||
        ! ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir == NAVDIR_FIRSTCHILD)
        return(FrameNavigate(m_hwnd, 0, NAVDIR_NEXT, pvarEnd));
    else if (dwNavDir == NAVDIR_LASTCHILD)
        return(FrameNavigate(m_hwnd, OBJID_SIZEGRIP-1, NAVDIR_PREVIOUS, pvarEnd));
    else if (varStart.lVal == CHILDID_SELF)
    {
        HWND    hwndParent;

        hwndParent = MyGetAncestor(m_hwnd, GA_PARENT);
        if (!hwndParent)
            return(S_FALSE);

        return (GetParentToNavigate(HWNDIDFromHwnd(m_hwnd), hwndParent,
            OBJID_CLIENT, dwNavDir, pvarEnd));
    }
    else
        return(FrameNavigate(m_hwnd, varStart.lVal, dwNavDir, pvarEnd));

}


// --------------------------------------------------------------------------
//
//  CWindow::accSelect()
//
//  Selecting a window is SWP'ing it to activate/bring to
//  front.
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::accSelect(long lSelFlags, VARIANT varChild)
{
    if (! ValidateChild(&varChild) ||
        ! ValidateSelFlags(lSelFlags))
        return(E_INVALIDARG);

    if (lSelFlags != SELFLAG_TAKEFOCUS)
        return(E_NOT_APPLICABLE);

    if (varChild.lVal)
        return(S_FALSE);

    if (MyGetAncestor(m_hwnd, GA_PARENT) == GetDesktopWindow())
        SetForegroundWindow(m_hwnd);
    else
        SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CWindow::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    RECT    rc;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal == 0)
    {
        MyGetRect(m_hwnd, &rc, TRUE);

        *pxLeft = rc.left;
        *pyTop = rc.top;
        *pcxWidth = rc.right - rc.left;
        *pcyHeight = rc.bottom - rc.top;
    }
    else
    {
        //
        // Ask the child.
        //
        IAccessible * poleacc;
        HRESULT hr;

        //
        // Get the help for our child frame object.
        //
        poleacc = NULL;
        hr = AccessibleObjectFromWindow(m_hwnd, varChild.lVal,
            IID_IAccessible, (void **)&poleacc);
        if (!SUCCEEDED(hr))
            return(hr);
        if (!poleacc)
            return(S_FALSE);

        varChild.lVal = CHILDID_SELF;
        hr = poleacc->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
        poleacc->Release();

        return(hr);
    }

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CWindow::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::accHitTest(long xLeft, long yTop, VARIANT* pvarHit)
{
    WINDOWINFO wi;
    long    lEnd;
    long    lHit;
    POINT   pt;

    InitPvar(pvarHit);

    lEnd = 0;

    if (! MyGetWindowInfo(m_hwnd, &wi))
        return(S_FALSE);

    //
    // Find out where point is.  But special case the client area!
    //
    pt.x = xLeft;
    pt.y = yTop;
    if (PtInRect(&wi.rcClient, pt))
        goto ReallyTheClient;

    lHit = SendMessageINT(m_hwnd, WM_NCHITTEST, 0, MAKELONG(xLeft, yTop));

    switch (lHit)
    {
        case HTERROR:
        case HTNOWHERE:
            return(S_FALSE);

        case HTCAPTION:
        case HTMINBUTTON:
        case HTMAXBUTTON:
        case HTHELP:
        case HTCLOSE:
        // case HTIME!
            lEnd = OBJID_TITLEBAR;
            break;

        case HTMENU:
            lEnd = OBJID_MENU;
            break;

        case HTSYSMENU:
            lEnd = OBJID_SYSMENU;
            break;

        case HTHSCROLL:
            lEnd = OBJID_HSCROLL;
            break;

        case HTVSCROLL:
            lEnd = OBJID_VSCROLL;
            break;

        case HTCLIENT:
        case HTTRANSPARENT:
ReallyTheClient:
            lEnd = OBJID_CLIENT;
            break;

        case HTGROWBOX:
            lEnd = OBJID_SIZEGRIP;
            break;

        case HTBOTTOMRIGHT:
            // Note that for sizeable windows, being over the size grip may
            // return in fact HTBOTTOMRIGHT for sizing purposes.  If this
            // point is inside the window borders, that is the case.
            if ((xLeft < wi.rcWindow.right - (int)wi.cxWindowBorders) &&
                (yTop < wi.rcWindow.bottom - (int)wi.cyWindowBorders))
            {
                lEnd = OBJID_SIZEGRIP;
            }
            break;

        // Includes borders!
        default:
            break;
    }

    if (lEnd)
        return(GetNoncObject(m_hwnd, lEnd, pvarHit));
    else
    {
        pvarHit->vt = VT_I4;
        pvarHit->lVal = lEnd;
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CWindow::Next()
//
//  We do loop from 0 to cChildren, it's just that the IDs are NEGATIVE,
//  not positive.  We accept child ids that are OBJIDs.
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::Next(ULONG celt, VARIANT* rgvar, ULONG* pceltFetched)
{
    VARIANT* pvar;
    long    cFetched;
    long    iCur;

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    pvar = rgvar;
    cFetched = 0;
    iCur = m_idChildCur;

    //
    // Loop through our items
    //
    while ((cFetched < (long)celt) && (iCur < m_cChildren))
    {
        cFetched++;
        iCur++;

        //
        // Note this gives us -((index)+1), which means we start at -1 and
        // decrement.  Conveniently, this corresponds to OBJID values!
        //
        pvar->vt = VT_I4;
        pvar->lVal = 0 - iCur;
        ++pvar;
    }

    //
    // Advance the current position
    //
    m_idChildCur = iCur;

    //
    // Fill in the number fetched
    //
    if (pceltFetched)
        *pceltFetched = cFetched;

    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CWindow::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CWindow::Clone(IEnumVARIANT ** ppenum)
{
    InitPv(ppenum);

    // Look for (and create) a suitable proxy/handler if one
    // exists. Use CreateWindowThing as default if none found.
    // (TRUE => use window, as opposed to client, classes)
    return FindAndCreateWindowClass( m_hwnd, TRUE, CreateWindowThing,
                           IID_IEnumVARIANT, m_idChildCur, (void**)ppenum );
}




// --------------------------------------------------------------------------
//
//  FrameNavigate()
//
//  Default handling of navigation among frame children.  The standard
//  frame widget handlers (titlebar, menubar, scrollbar, etc.) hand off
//  peer navigation to us, their parent.  There are two big reasons for this:
//
//  (1) It saves on code and ease of implementation, since the knowledge of
//      what is to the left of what, what is below what, etc. only has to
//      be coded in one place.
//
//  (2) It allows apps that want to manage their own frame and e.g. add a
//      new element that acts like a frame piece yet still have navigation
//      work properly.  Their frame handler can hand off to the default
//      implementation but trap navigation.
//
// --------------------------------------------------------------------------
HRESULT FrameNavigate(HWND hwndFrame, long lStart, long dwNavDir,
    VARIANT * pvarEnd)
{
    long        lEnd;
    long        lMask;
    WINDOWINFO  wi;
    TCHAR       szClassName[128];
    BOOL        bFound = FALSE;
    IAccessible *   poleacc;
    IDispatch * pdispEl;
    HRESULT     hr;

    //
    // Currently, we get an index (fix validation layer so IDs are OBJIDs)
    //
    lEnd = 0;

    lStart = IndexFromObjid(lStart);

    //
    // Figure out what is present, what isn't.
    //
    if (!MyGetWindowInfo(hwndFrame, &wi))
        return(E_FAIL);

    lMask = 0;
    lMask |= MaskBit(IndexFromObjid(OBJID_CLIENT));

    if ((wi.dwStyle & WS_CAPTION)== WS_CAPTION)
        lMask |= MaskBit(IndexFromObjid(OBJID_TITLEBAR));

    if (wi.dwStyle & WS_SYSMENU)
        lMask |= MaskBit(IndexFromObjid(OBJID_SYSMENU));

    if (wi.dwStyle & WS_VSCROLL)
        lMask |= MaskBit(IndexFromObjid(OBJID_VSCROLL));

    if (wi.dwStyle & WS_HSCROLL)
        lMask |= MaskBit(IndexFromObjid(OBJID_HSCROLL));

    if ((wi.dwStyle & (WS_HSCROLL | WS_VSCROLL)) == (WS_HSCROLL | WS_VSCROLL))
        lMask |= MaskBit(IndexFromObjid(OBJID_SIZEGRIP));

    if (!(wi.dwStyle & WS_CHILD) && GetMenu(hwndFrame))
        lMask |= MaskBit(IndexFromObjid(OBJID_MENU));

    // HACKISH BIT for new IE4/Shell Menubands
    // The menus aren't menus, so we have to see if this thing
    // has menubands.
    // First, check the classname - only the browser and shell
    // windows have these things...
    // The reason we have to do this is because the IE4 guys are
    // slackers and didn't do very much for accessibility.
    GetClassName (hwndFrame, szClassName,ARRAYSIZE(szClassName));
    if ((0 == lstrcmp (szClassName,TEXT("IEFrame"))) ||
        (0 == lstrcmp (szClassName,TEXT("CabinetWClass"))))
    {
        HWND            hwndWorker;
        HWND            hwndRebar;
        HWND            hwndSysPager;
        HWND            hwndToolbar;

        // We can just send a WM_GETOBJECT to the menuband window,
        // we just have to find it. Let's use FindWindowEx to do that.
        // This is not easy: There are 4 children of an IEFrame Window,
        // and I am not sure how many children of a shell window (CabinetWClass).
        // For IEFrame windows, the menuband is the:
        // ToolbarWindow32 child of a SysPager that is the child of a
        // RebarWindow32 that is the child of a Worker.
        // But there are 2 Worker windows at the 1st level down,
        // and 2 SysPagers that are children of the RebarWindow32.

        bFound = FALSE;
        hwndWorker = NULL;
        while (!bFound)
        {
            hwndWorker = FindWindowEx (hwndFrame,hwndWorker,TEXT("Worker"),NULL);
            if (!hwndWorker)
                break;

            hwndRebar = FindWindowEx (hwndWorker,NULL,TEXT("RebarWindow32"),NULL);
            if (!hwndRebar)
                continue;

            hwndSysPager = NULL;
            while (!bFound)
            {
                hwndSysPager = FindWindowEx (hwndRebar,hwndSysPager,TEXT("SysPager"),NULL);
                if (!hwndSysPager)
                    break;
                hwndToolbar = FindWindowEx (hwndSysPager,NULL,TEXT("ToolbarWindow32"),NULL);
                hr = AccessibleObjectFromWindow (hwndToolbar,OBJID_MENU,
                                                 IID_IAccessible, (void **)&poleacc);
                if (SUCCEEDED(hr))
                {
                    bFound = TRUE;
                    lMask |= MaskBit(IndexFromObjid(OBJID_MENU));
                }
            }
        }
    } // end if we are talking to something that might have a menuband

    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
            lEnd = lStart;
            while (++lEnd <= CCHILDREN_FRAME)
            {
                // Is the next item present?
                if (lMask & MaskBit(lEnd))
                    break;
            }

            if (lEnd > CCHILDREN_FRAME)
                lEnd = 0;
            break;

        case NAVDIR_PREVIOUS:
            lEnd = lStart;
            while (--lEnd > 0)
            {
                // Is the previous item present?
                if (lMask & MaskBit(lEnd))
                    break;
            }

            Assert(lEnd >= 0);
            break;

        case NAVDIR_UP:
        case NAVDIR_DOWN:
        case NAVDIR_LEFT:
        case NAVDIR_RIGHT:
            lEnd = lStart;
            while (lEnd = rgFrameNavigate[lEnd-1].NavPeer[IndexFromNavDir(dwNavDir)])
            {
                // Is this item around?
                if (lMask & MaskBit(lEnd))
                    break;
            }
            break;
    }

    if (lEnd)
    {
        // now finish up our hackish work. For normal things, we just
        // return GetNoncObject, which is basically just a call to
        // AccessibleObjectFromWindow with the id of the frame element,
        // and then it just stuffs the return value (an IDispatch) into
        // the VARIANT.
        // For IE4 hackish stuff, we have an IAccessible, we'll QI for
        // IDispatch, Release the IAccessible, and stuff the IDispatch
        // into a VARIANT.
        if (bFound && lEnd == IndexFromObjid(OBJID_MENU))
        {
            hr = poleacc->QueryInterface(IID_IDispatch,(void**)&pdispEl);
			poleacc->Release();

            if (!SUCCEEDED(hr))
                return(hr);
            if (!pdispEl)
                return(E_FAIL);

            pvarEnd->vt = VT_DISPATCH;
            pvarEnd->pdispVal = pdispEl;
            return (S_OK);
        }
        else
            return(GetNoncObject(hwndFrame, ObjidFromIndex(lEnd), pvarEnd));
    }
    else
        return(S_FALSE);
}


