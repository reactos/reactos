//+-------------------------------------------------------------------
//
//  File:       status.cxx
//
//  Contents:   Functions implementing status bar.
//
//  Classes:    StatusBar
//
//--------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

WCHAR FAR lpstrStatBarClass[] = L"classStatBar";

//+---------------------------------------------------------------
//
//  Function:   _PatB, private
//
//  Synopsis:   inline helper to fill rectangle with a color
//
//----------------------------------------------------------------

inline void _PatB(HDC hdc, int x, int y, int dx, int dy, COLORREF rgb)
{
    RECT    rc;
    SetRect(&rc, x, y, x+dx, y+dy);
    SetBkColor(hdc, rgb);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
};

//+---------------------------------------------------------------
//
//  Member:     StatusBar::ClassInit
//
//  Synopsis:   Registers the status bar window class
//
//  Arguments:  [hinst] -- instance handle of module using status bar
//
//  Returns:    TRUE iff the status bar was registered successfully
//
//  Notes:      This static method is usually called from the WinMain
//              or LibMain of the module using the status bar.
//
//----------------------------------------------------------------

BOOL StatusBar::ClassInit(HINSTANCE hinst)
{
    WNDCLASS wc;

    wc.style = 0L;
    wc.lpfnWndProc = StatusWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(void FAR*);
    wc.hInstance = hinst;
    wc.hIcon = NULL;
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = lpstrStatBarClass;

    return RegisterClass(&wc)!=NULL;
}

//+---------------------------------------------------------------
//
//  Member:     StatusBar::StatusBar, public
//
//  Synopsis:   Constructor for status bar class
//
//  Notes:      Creating a status bar is a two step process.
//              First construct the object, then call the
//              Init method to actually create the status bar window.
//
//----------------------------------------------------------------

StatusBar::StatusBar()
{
    _hwnd = NULL;
    _hfont = NULL;
    _wHeight = (WORD)-1;
    _wUnitBorder = (WORD)-1;
    _wSpace = 6;
    _wAboveBelow = 2;
    lstrcpy(_lpstrString, L"");
}


//+---------------------------------------------------------------
//
//  Member:     StatusBar::~StatusBar, public
//
//  Synopsis:   Destructor for status bar class
//
//  Notes:      This frees any resources held by the status bar
//              including deleting the window
//
//----------------------------------------------------------------

StatusBar::~StatusBar()
{
    if (_hfont != NULL)
        DeleteObject(_hfont);

    if (_hwnd != NULL)
        DestroyWindow(_hwnd);
}


//+-------------------------------------------------------------------
//
//  Member:     StatusBar::Init, public
//
//  Synopsis:   Creates a status bar window as a child of another window
//
//  Arguments:  [hwnd]  -- the window handle of the parent window
//
//  Returns:    The window handle of the status bar child window
//
//--------------------------------------------------------------------

HWND
StatusBar::Init(HINSTANCE hinst, HWND hwndParent)
{
    //
    // create font, save handle, find height of status bar.
    //

    HDC hdc = GetDC(NULL);
    int iFontHeight;
    TEXTMETRIC tm;
    iFontHeight = MulDiv(-10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    _hfont = CreateFont(iFontHeight, 0, 0, 0, 400, 0, 0, 0,
                         ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,
                         VARIABLE_PITCH | FF_SWISS,
                         L"Helv");

    HFONT   hfontOld = SelectFont(hdc, _hfont);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hfontOld);
    ReleaseDC(NULL, hdc);

    _wUnitBorder = (WORD)GetSystemMetrics(SM_CYBORDER);
    _wHeight = (WORD)(tm.tmHeight + tm.tmExternalLeading +     // font height
                               4 +                             // 2 above/below
                               _wUnitBorder +                  // for border
                              _wAboveBelow * 2);               // above and below
    //
    // create window
    //

    RECT rc;
    GetClientRect(hwndParent, &rc);

    // this will automatically set _hwnd!
    CreateWindowEx(0L,         // extended style
        lpstrStatBarClass,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER,  // style
        0, rc.bottom - _wHeight,            // x, y
        rc.right - rc.left, _wHeight,       // cx, cy
        hwndParent,             // hwndParent
        NULL,                   // hmenu
        hinst,
        this);                  // lpParam

    return _hwnd;
}

//+---------------------------------------------------------------
//
//  Member:     StatusBar::OnSize, public
//
//  Synopsis:   Updates the position of the status bar when the
//              parent is resized
//
//  Arguments:  [lprc] -- client rectangle of parent window
//
//  Notes:      The parent window should call this method during
//              WM_SIZE processing.  The parent should get its client
//              rectangle and pass it to this method.  This method will
//              move the status bar window to occupy the bottom of
//              the window and will update the rectangle to remove its
//              area (i.e. the returned rectangle is the parent windows'
//              new, effective client area).
//
//----------------------------------------------------------------

void
StatusBar::OnSize(LPRECT lprc)
{
    lprc->bottom -= _wHeight;
    MoveWindow(_hwnd, 0, lprc->bottom, lprc->right-lprc->left, _wHeight, TRUE);
    InvalidateRect(_hwnd, NULL, TRUE);
}


//+-------------------------------------------------------------------
//
//  Member:     StatusBar::DisplayMessage, public
//
//  Synopsis:   Displays a string on the status bar
//
//  Arguments:  [lpstr] -- string to display in status bar.  NULL
//                         means just clear the current message.
//
//--------------------------------------------------------------------

void
StatusBar::DisplayMessage(LPCWSTR lpstr)
{
    HDC hdc = GetDC(_hwnd);
    RECT rc;
    GetClientRect(_hwnd, &rc);
    InflateRect(&rc, -1, -1);
    rc.left += _wSpace;
    rc.top += _wAboveBelow;
    rc.right -= _wSpace;
    rc.bottom -= _wAboveBelow;
    InflateRect(&rc, -1, -1);
    HFONT hfontOld = SelectFont(hdc, _hfont);

    // clear out the old message
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE|ETO_CLIPPED, &rc, NULL, 0, NULL);

    // put up the new status message
    if (lpstr != NULL)
    {
        lstrcpy(_lpstrString, lpstr);

        SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
        ExtTextOut(hdc,
                    rc.left,
                    rc.top,
                    ETO_OPAQUE | ETO_CLIPPED,
                    &rc,
                    _lpstrString,
                    lstrlen(_lpstrString),
                    (LPINT) NULL);
    }

    SelectObject(hdc, hfontOld);
    ReleaseDC(_hwnd, hdc);
}


//+---------------------------------------------------------------
//
//  Function:   StatusWndProc, private
//
//  Synopsis:   Window procedure for status bar window
//
//----------------------------------------------------------------

extern "C" LRESULT CALLBACK
StatusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //
    //get a pointer to our object
    //
    LPSTATUSBAR pStatusBar = (LPSTATUSBAR)GetWindowLongPtr(hwnd, 0);

    //
    //if it is NULL, then check for a WM_NCCREATE message
    //   when we get one stash the pointer to our object
    //
    if(pStatusBar == NULL)
    {
        if(msg == WM_NCCREATE)
        {
            CREATESTRUCT FAR *lpcs = (CREATESTRUCT FAR *)lParam;
            pStatusBar = (LPSTATUSBAR)lpcs->lpCreateParams;
            pStatusBar->_hwnd = hwnd;
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)pStatusBar);
            //drop through to forward the message
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    if(msg == WM_NCDESTROY)
    {
        SetWindowLongPtr(hwnd, 0, 0L);
        pStatusBar->_hwnd = NULL;
        //drop through to forward the message to the window procedure
    }

    //handle any window message by passing it to the appropriate
    //member function...
    switch(msg)
    {
    HANDLE_MSG(hwnd, WM_PAINT, pStatusBar->OnPaint);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------
//
//  Member:     StatusBar::OnPaint, private
//
//  Synopsis:   Handles the WM_PAINT message for the status bar window
//
//----------------------------------------------------------------

void
StatusBar::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(hwnd, &ps);
    RECT rc;
    COLORREF rgbT;

    // compute rectangle
    GetClientRect(hwnd, &rc);

    //
    //      draw highlight and shadow
    //
    rgbT = GetSysColor(COLOR_BTNHIGHLIGHT);
            // left and top
    _PatB(hdc, rc.left, rc.top, 1, rc.bottom - rc.top, rgbT);
    _PatB(hdc, rc.left, rc.top, rc.right - rc.left, 1, rgbT);
    rgbT = GetSysColor(COLOR_BTNSHADOW);
            // right and bottom
    _PatB(hdc, rc.right - 1, rc.top, 1, rc.bottom - rc.top, rgbT);
    _PatB(hdc, rc.left, rc.bottom - 1, rc.right-rc.left, 1, rgbT);

    InflateRect(&rc, -1, -1);

    //
    //      first do the message box
    //
    RECT rcMbox;
    SetRect(&rcMbox,
            rc.left + _wSpace,
            rc.top + _wAboveBelow,
            rc.right - _wSpace,
            rc.bottom - _wAboveBelow);

    //      draw 3d effects for this box.
    // left and top
    rgbT = GetSysColor(COLOR_BTNSHADOW);
    int dxT = rcMbox.right - rcMbox.left;
    int dyT = rcMbox.bottom - rcMbox.top;
    _PatB(hdc, rcMbox.left, rcMbox.top, 1, dyT, rgbT);
    _PatB(hdc, rcMbox.left, rcMbox.top, dxT, 1, rgbT);
    rgbT = GetSysColor(COLOR_BTNHIGHLIGHT);
    // right and bottom
    _PatB(hdc, rcMbox.right - 1, rcMbox.top, 1, dyT, rgbT);
    _PatB(hdc, rcMbox.left, rcMbox.bottom - 1, dxT, 1, rgbT);

    InflateRect(&rcMbox, -1, -1);

    HFONT hfontOld = SelectFont(hdc, _hfont);

    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    ExtTextOut(hdc,
                rcMbox.left,
                rcMbox.top,
                ETO_OPAQUE | ETO_CLIPPED,
                &rcMbox,
                _lpstrString,
                lstrlen(_lpstrString),
                (LPINT) NULL);

    SelectObject(hdc, hfontOld);

    InflateRect(&rcMbox, 1, 1);
    ExcludeClipRect(hdc,
                rcMbox.left, rcMbox.top, rcMbox.right, rcMbox.bottom);

    //      now do the background.
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE|ETO_CLIPPED, &rc, NULL, 0, NULL);

    EndPaint(hwnd, &ps);
}
