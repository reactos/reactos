// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  CARET.CPP
//
//  This file has the implementation of the caret system object.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "caret.h"

// --------------------------------------------------------------------------
// prototypes for local functions
// --------------------------------------------------------------------------
int AddInts (int Value1, int Value2);
BOOL GetDeviceRect (HDC hDestDC,RECT ClientRect,LPRECT lpDeviceRect);


BOOL Rect1IsOutsideRect2( RECT const & rc1, RECT const & rc2 );


// --------------------------------------------------------------------------
//
//  CreateCaretObject()
//
// --------------------------------------------------------------------------
HRESULT CreateCaretObject(HWND hwnd, long idObject, REFIID riid, void** ppvCaret)
{
    UNUSED(idObject);

    return(CreateCaretThing(hwnd, riid, ppvCaret));
}



// --------------------------------------------------------------------------
//
//  CreateCaretThing()
//
// --------------------------------------------------------------------------
HRESULT CreateCaretThing(HWND hwnd, REFIID riid, void **ppvCaret)
{
    CCaret * pcaret;
    HRESULT hr;

    InitPv(ppvCaret);

    pcaret = new CCaret();
    if (pcaret)
    {
        if (! pcaret->FInitialize(hwnd))
        {
            delete pcaret;
            return(E_FAIL);
        }
    }
    else
        return(E_OUTOFMEMORY);

    hr = pcaret->QueryInterface(riid, ppvCaret);
    if (!SUCCEEDED(hr))
        delete pcaret;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CCaret::FInitialize()
//
// --------------------------------------------------------------------------
BOOL CCaret::FInitialize(HWND hwnd)
{
    // Is this an OK window?
    m_dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
    if (! m_dwThreadId)
        return(FALSE);

    //
    // NOTE:  We always initialize, even if this window doesn't own the
    // caret.  We will treat it as invisible in that case.
    //
    m_hwnd = hwnd;
    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  CCaret::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::Clone(IEnumVARIANT** ppenum)
{
    return(CreateCaretThing(m_hwnd, IID_IEnumVARIANT, (void **)ppenum));
}



// --------------------------------------------------------------------------
//
//  CCaret::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    return(HrCreateString(STR_CARETNAME, pszName));
}



// --------------------------------------------------------------------------
//
//  CCaret::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::get_accRole(VARIANT varChild, VARIANT * pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_CARET;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCaret::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::get_accState(VARIANT varChild, VARIANT * pvarState)
{
    GUITHREADINFO   gui;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (! MyGetGUIThreadInfo(m_dwThreadId, &gui) ||
          (gui.hwndCaret != m_hwnd))
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
        return(S_FALSE);
    }

    if (!(gui.flags & GUI_CARETBLINKING))
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CCaret::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
GUITHREADINFO   gui;
HDC             hDC;
RECT            rcDevice;
    

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!MyGetGUIThreadInfo(m_dwThreadId, &gui) ||
        (gui.hwndCaret != m_hwnd))
    {
        return(S_FALSE);
    }

    // Instead of using MapWindowPoints, we use a private
    // function called GetDeviceRect that takes private mapping
    // modes, etc. into account.

    hDC = GetDC (m_hwnd);
    GetDeviceRect (hDC,gui.rcCaret,&rcDevice);
    ReleaseDC (m_hwnd,hDC);

    // Sanity check against returned rect - sometimes we get back
    // gabage cursor coords (of the order of (FFFFB205, FFFFB3C5))
    // - eg. when in notepad, place cursor at top/bottom of doc,
    // click on scrollbar arrowheads to scroll cursor off top/bottom
    // of visible area.
    // We still get back a valid hwnd and a CURSORBLINKING flag fom
    // GetGUIThreadInfo, so they aren't much use to detect this.

    RECT rcWindow;
    GetWindowRect( m_hwnd, & rcWindow );
    if( Rect1IsOutsideRect2( rcDevice, rcWindow ) )
    {
        return S_FALSE;
    }


    *pxLeft = rcDevice.left;
    *pyTop = rcDevice.top;
    *pcxWidth = rcDevice.right - rcDevice.left;
    *pcyHeight = rcDevice.bottom - rcDevice.top;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CCaret::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CCaret::accHitTest(long xLeft, long yTop, VARIANT * pvarChild)
{
    GUITHREADINFO gui;
    POINT pt;

    InitPvar(pvarChild);

    if (! MyGetGUIThreadInfo(m_dwThreadId, &gui) ||
        (gui.hwndCaret != m_hwnd))
    {
        return(S_FALSE);
    }

    pt.x = xLeft;
    pt.y = yTop;
    ScreenToClient(m_hwnd, &pt);

    if (PtInRect(&gui.rcCaret, pt))
    {
        pvarChild->vt = VT_I4;
        pvarChild->lVal = 0;
        return(S_OK);
    }
    else
        return(S_FALSE);
}

//============================================================================
// This function takes a destination DC, a rectangle in client coordinates,
// and a pointer to the rectangle structure that will hold the device 
// coordinates of the rectangle. The device coordinates can be used as screen
// coordinates.
//============================================================================
BOOL GetDeviceRect (HDC hDestDC,RECT ClientRect,LPRECT lpDeviceRect)
{
POINT   aPoint;
int	    temp;
    
    lpDeviceRect->left = ClientRect.left;
    lpDeviceRect->top = ClientRect.top;
    
    // just set the device rect to the rect given and then do LPtoDP for both points
    lpDeviceRect->right = ClientRect.right;
    lpDeviceRect->bottom = ClientRect.bottom;
    LPtoDP (hDestDC,(LPPOINT)lpDeviceRect,2);
    
    // Now we need to convert from client coords to screen coords. We do this by 
    // getting the DC Origin and then using the AddInts function to add the origin 
    // of the 'drawing area' to the client coordinates. This is safer and easier 
    // than using ClientToScreen, MapWindowPoints, and/or getting the WindowRect if
    // it is a WindowDC. 
    GetDCOrgEx(hDestDC,&aPoint);
    
    lpDeviceRect->left = AddInts (lpDeviceRect->left,aPoint.x);
    lpDeviceRect->top = AddInts (lpDeviceRect->top,aPoint.y);
    lpDeviceRect->right = AddInts (lpDeviceRect->right,aPoint.x);
    lpDeviceRect->bottom = AddInts (lpDeviceRect->bottom,aPoint.y);
    
    // make sure that the top left is less than the bottom right!!!
    if (lpDeviceRect->left > lpDeviceRect->right)
    {
        temp = lpDeviceRect->right;
        lpDeviceRect->right = lpDeviceRect->left;
        lpDeviceRect->left = temp;
    }
    
    if (lpDeviceRect->top > lpDeviceRect->bottom)
    {
        temp = lpDeviceRect->bottom;
        lpDeviceRect->bottom = lpDeviceRect->top;
        lpDeviceRect->top = temp;
    }
    
    return TRUE;
} // end GetDeviceRect

//============================================================================
// AddInts adds two integers and makes sure that the result does not overflow
// the size of an integer.
// Theory: positive + positive = positive
//         negative + negative = negative
//         positive + negative = (sign of operand with greater absolute value)
//         negative + positive = (sign of operand with greater absolute value)
// On the second two cases, it can't wrap, so I don't check those.
//============================================================================
int AddInts (int Value1, int Value2)
{
int result;
    
    result = Value1 + Value2;
    
    if (Value1 > 0 && Value2 > 0 && result < 0)
        result = INT_MAX;
    
    if (Value1 < 0 && Value2 < 0 && result > 0)
        result = INT_MIN;
    
    return result;
}


