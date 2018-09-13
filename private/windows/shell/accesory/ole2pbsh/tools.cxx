
//+---------------------------------------------------------------------
//
//   File:      tools.cxx
//
//   Notes:	    Simple tool-tray window base class and derived
//		        trays for PBrush drawing tools
//
//   Classes:
//              PBToolTray (a window class)
//              CTray
//
//------------------------------------------------------------------------

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>

#include <ole2.h>
#include <o2base.hxx>     // the base classes and utilities

#include "pbs.hxx"


#define MARGIN_X            4
#define MARGIN_Y            4
#define FRAME_X             1
#define FRAME_Y             1
#define TRAY_DEFAULT_WIDTH  64
#define TRAY_DEFAULT_HEIGHT 256
#define LSIZETOOL_HEIGHT    64
#define COLORTRAY_WIDTH     256
#define COLORTRAY_HEIGHT    48


    BOOL Tray::_fInit = FALSE;
    ATOM Tray::_atomClass = 0;
    COLORREF Tray::crLtGray;
    COLORREF Tray::crGray;
    COLORREF Tray::crDkGray;
    COLORREF Tray::crBlack;
    HBRUSH Tray::hbrActiveCaption = NULL;
    HBRUSH Tray::hbrInActiveCaption = NULL;
    HBRUSH Tray::hbrWindowFrame = NULL;
    HBRUSH Tray::hbrSysBox = NULL;
    WORD Tray::wCnt = 0;


//
//  Routines & definitions for creating windows with custom NON-CLIENT areas
//
#define SetWF(hwnd,wf)  SetWindowLong(hwnd, GWL_STYLE, \
            GetWindowLong(hwnd,GWL_STYLE) | (wf))
#define ClrWF(hwnd,wf)  SetWindowLong(hwnd, GWL_STYLE, \
            GetWindowLong(hwnd,GWL_STYLE) &~(wf))
#define TestWF(hwnd,wf) (GetWindowLong(hwnd,GWL_STYLE) & (wf))

//
//  Window styles used by ncMsgFilter
//
#define WF_SIZEFRAME    WS_THICKFRAME
#define WF_SYSMENU      WS_SYSMENU
#define WF_MINIMIZED    WS_MINIMIZE
#define WF_SIZEBOX      0x0002
#define WF_ACTIVE       0x0001


//+---------------------------------------------------------------
//
//  Member: Tray::Create (static)
//
//---------------------------------------------------------------

LPTRAY
Tray::Create(HINSTANCE hinst,
        		HWND hwndParent,
        		HWND hwndNotify,
        		int iChild)
{
    DOUT(L"Tray::Create\r\n");

    //
    //If we haven't yet done our base initialization, do it now
    //
    if(_fInit == FALSE)
    {
        if(Tray::InitClass(hinst) == 0)
            return NULL;
    }

    LPTRAY pTray;
    if ((pTray = new Tray(hwndParent, hwndNotify, iChild)) != NULL)
    {
        if(NULL == pTray->CreateToolWindow(hinst, hwndParent))
        {
            DOUT(L"Tray::Create Failing!\r\n");

            delete pTray;
            pTray = NULL;
        }
    }
    return pTray;
}


//+---------------------------------------------------------------
//
//  Member: Tray::InitClass (static)
//
//---------------------------------------------------------------

ATOM
Tray::InitClass(HINSTANCE hinst)
{
    crLtGray = PALETTEINDEX(7);
    crGray = PALETTEINDEX(248);
    crDkGray = PALETTEINDEX(249);
    crBlack = PALETTEINDEX(0);
    //
    // register our window class
    //
    WNDCLASS wc;
    wc.style = CS_NOCLOSE | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = TrayWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;        // for pointer to Tray object
    wc.hInstance = hinst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = CLASS_NAME_PBTRAY;
    if((Tray::_atomClass = RegisterClass(&wc)) != 0)
    {
        Tray::_fInit = TRUE;
    }
    return _atomClass;
}

//+---------------------------------------------------------------
//
//  Member: Tray::Tray
//
//---------------------------------------------------------------

Tray::Tray(HWND hwndParent, HWND hwndNotify, int iChild)
{
    _hwndNotify = hwndNotify;
    _iChild = iChild;
    _hwnd = NULL;
    _cxFrame = FRAME_X;
    _cyFrame = FRAME_Y;
    _dxMargin = MARGIN_X;
    _dyMargin = MARGIN_Y;

    _xWidth = TRAY_DEFAULT_WIDTH;
    _yHeight = TRAY_DEFAULT_HEIGHT;
}

//+---------------------------------------------------------------
//
//  Member: Tray::~Tray
//
//---------------------------------------------------------------

Tray::~Tray(void)
{
    if (_hwnd != NULL)
    {
        DOUT(L"Tray::~Tray Destroying Window\r\n");
        DestroyWindow(_hwnd);
    }
}


//+---------------------------------------------------------------
//
// Member:  Tray::Position
//
// Notes:   The idea for this method is for the derived class
//          to provide behavior such that it positions itself
//          in it's "natural" position relative to the x, y parms
//
//---------------------------------------------------------------

void
Tray::Position(int x, int y)
{
    SetWindowPos(_hwnd, 0, x, y, _xWidth, _yHeight, SWP_NOACTIVATE | SWP_NOZORDER);
}

void
Tray::GetToolRect(LPRECT lprc)
{
    GetClientRect(_hwnd, lprc);
    InflateRect(lprc, -_dxMargin, -_dyMargin);
}

//+---------------------------------------------------------------
//
//  Member: Tray::OnCreate
//
//---------------------------------------------------------------

BOOL
Tray::OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member: Tray::OnDestroy
//
//---------------------------------------------------------------

void
Tray::OnDestroy(HWND hwnd)
{
    _hwnd = NULL;
}

//+---------------------------------------------------------------
//
//  Member: Tray::OnPaint
//
//---------------------------------------------------------------

void
Tray::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    HBRUSH hbr = (HBRUSH)GetStockBrush(LTGRAY_BRUSH);
    FillRect(ps.hdc, &ps.rcPaint, hbr);
    EndPaint(hwnd, &ps);
}


void
Tray::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMaxSize.x = _xWidth;
    lpMinMaxInfo->ptMaxSize.y = _yHeight;
    lpMinMaxInfo->ptMaxPosition.x = lpMinMaxInfo->ptMaxPosition.y = 0;
    lpMinMaxInfo->ptMinTrackSize.x = 8;
    lpMinMaxInfo->ptMinTrackSize.y = 16;
    lpMinMaxInfo->ptMaxTrackSize.x = 1024;
    lpMinMaxInfo->ptMaxTrackSize.y = 768;
}

void
Tray::OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos)
{
    if (lpwpos->flags & SWP_HIDEWINDOW)
       return;

    DOUT(L"Tray::OnWindowPosChanged\r\n");
}

//+---------------------------------------------------------------
//
//  Function:   TrayWndProc
//
//---------------------------------------------------------------

extern "C" LRESULT CALLBACK
TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //
    // get a pointer to the Tray object
    // if it is NULL, then check for a WM_NCCREATE message
    // when we get one stash the pointer to our object
    //
    LPTRAY pTray = (LPTRAY)GetWindowLong(hwnd, 0);
    if(pTray == NULL)
    {
        if(msg == WM_NCCREATE)
        {
            CREATESTRUCT FAR *lpcs = (CREATESTRUCT FAR *)lParam;
            pTray = (LPTRAY)lpcs->lpCreateParams;
            SetWindowLong(hwnd, 0, (LONG)pTray);
            return(pTray->ncMsgFilter(hwnd, msg, wParam, lParam));
        }
        return(DefWindowProc(hwnd, msg, wParam, lParam));
    }

    if(msg == WM_NCDESTROY)
    {
        SetWindowLong(hwnd, 0, 0L);
        return(DefWindowProc(hwnd, msg, wParam, lParam));
    }

    //
    // handle any window message by passing it to the appropriate
    // member function...
    //
    switch(msg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, pTray->OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, pTray->OnDestroy);
        HANDLE_MSG(hwnd, WM_PAINT, pTray->OnPaint);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO, pTray->OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, pTray->OnWindowPosChanged);
    }

    return pTray->ncMsgFilter(hwnd, msg, wParam, lParam);
}


//+---------------------------------------------------------------
//
//  Member: Tray::ncCalcRect
//
//---------------------------------------------------------------

inline void
Tray::ncCalcRect(HWND hwnd, LPRECT lprc)
{
    InflateRect(lprc, -_cxFrame, -_cyFrame);
}



//+---------------------------------------------------------------
//
//  Member: Tray::ncHitTest
//
//---------------------------------------------------------------

LONG
Tray::ncHitTest(HWND hwnd, POINT pt)
{
    //
    // If the window is minimized and iconic, return HTCAPTION
    //
    if (TestWF(hwnd, WF_MINIMIZED))
        return HTCAPTION;

    //
    // Get Client and Window rects in screen coordinates
    //
    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    RECT rcClient = rcWindow;
    ncCalcRect(hwnd, &rcClient);

    //
    //lie about actual client area: count the margin as NC
    //
    InflateRect(&rcClient, -_dxMargin, -_dyMargin);

    if (PtInRect(&rcClient, pt))
        return HTCLIENT;

    return HTCAPTION;
}


//+---------------------------------------------------------------
//
//  Member: Tray::ncDrawFrame
//
//---------------------------------------------------------------

void
Tray::ncDrawFrame(HWND hwnd)
{
    HDC  hdc = GetWindowDC(hwnd);

    RECT rc;
    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);

    SelectObject(hdc, hbrWindowFrame);

    //
    // This works as long as cxFrame and cyFrame are both one
    //
    FrameRect(hdc, &rc, hbrWindowFrame);

    InflateRect(&rc, -_cxFrame, -_cyFrame);

    rc.bottom = rc.top + _cyFrame;
    FillRect(hdc, &rc, hbrWindowFrame);

    ReleaseDC(hwnd, hdc);
}


//+---------------------------------------------------------------
//
//  Member: Tray::ncMsgFilter
//
//
// This method handles Windows non-client
// messages so that the non-client area of a window is drawn as a low
// key floating tool box.  The title bar of the window is much smaller
// than the standard Windows title bar, and no caption is displayed.
// The system menu box is replaced by a small black rectangle.
//
// When using this method, the following messages must be passed in
// order for the window to correctly operate within the Windows
// environment: all non-client messages, whose symbolic names begin with
// the prefix WM_NC, the WM_SYSCOMMAND, WM_COMMAND, and WM_INITMENU
// messages.  If the caller also needs to process any of these messages,
// the caller must pre-process the message and then call ncMsgFilter.
//
// If an unrecognized message is passed to ncMsgFilter,
// DefWindowProc is called.
//
//---------------------------------------------------------------

LRESULT
Tray::ncMsgFilter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCACTIVATE:     // wParam == active state
        if (wParam)
            SetWF(hwnd, WF_ACTIVE);
        else
            ClrWF(hwnd, WF_ACTIVE);

        ncDrawFrame(hwnd);
        return TRUE;

    case WM_NCCALCSIZE:     // lParam == LPRECT of window rect
        ncCalcRect(hwnd, (LPRECT)lParam);
        if(wParam)
            return WVR_REDRAW;
        return 0L;

    case WM_NCCREATE:       // Sent before WM_CREATE
        if (wCnt++ == 0)
        {
            hbrActiveCaption = CreateSolidBrush(
                    GetSysColor(COLOR_ACTIVECAPTION));
            hbrInActiveCaption = CreateSolidBrush(
                    GetSysColor(COLOR_INACTIVECAPTION));
            hbrWindowFrame = CreateSolidBrush(
                    GetSysColor(COLOR_WINDOWFRAME));
            hbrSysBox = (HBRUSH)GetStockObject(BLACK_BRUSH);
        }
        break;

    case WM_NCDESTROY:      // Sent before WM_DESTROY
        if (wCnt == 0)
            break;  // haven't created one yet - don't delete

        if (--wCnt == 0)
        {
            DeleteObject(hbrActiveCaption);
            DeleteObject(hbrInActiveCaption);
            DeleteObject(hbrWindowFrame);
        }
        break;

    case WM_NCHITTEST:      // lParam is POINT in screen cords
       {
           POINT pt = { LOWORD(lParam), HIWORD(lParam) };
           return ncHitTest(hwnd, pt);
       }

    case WM_NCPAINT:
        ncDrawFrame(hwnd);
        return 0L;

    //
    //  Dont activate the window when it is clicked on in the CLIENT
    //  area, this is useful for "tool" windows.
    //
    //  Should this behaviour be controled by a style bit?
    //
    case WM_MOUSEACTIVATE:
        if (LOWORD(lParam) == HTCLIENT)
            return MA_NOACTIVATE;

        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// Derived Classes
//======================
//

//+---------------------------------------------------------------
//
//  ToolTray
//
//---------------------------------------------------------------

LPTOOLTRAY
ToolTray::Create(HINSTANCE hinst,
        		HWND hwndParent,
        		HWND hwndNotify,
        		int iChild)
{
    DOUT(L"ToolTray::Create\r\n");

    //
    //If we haven't yet done our base initialization, do it now
    //
    if(Tray::_fInit == FALSE)
    {
        if(Tray::InitClass(hinst) == 0)
            return NULL;
    }

    LPTOOLTRAY pTray;
    if ((pTray = new ToolTray(hwndParent, hwndNotify, iChild)) != NULL)
    {
        if(NULL == pTray->CreateToolWindow(hinst, hwndParent))
        {
            DOUT(L"CreateTray::Create Failing!\r\n");

            delete pTray;
            pTray = NULL;
        }
    }
    return pTray;
}


ToolTray::ToolTray(HWND hwndParent, HWND hwndNotify, int iChild) :
        Tray(hwndParent, hwndNotify, iChild)
{
    _yOffset = LSIZETOOL_HEIGHT;
    _xWidth = TRAY_DEFAULT_WIDTH;
    _yHeight = TRAY_DEFAULT_HEIGHT + _yOffset;
}

void
ToolTray::GetToolRect(LPRECT lprc)
{
    GetClientRect(_hwnd, lprc);
    InflateRect(lprc, -_dxMargin, -_dyMargin);
    lprc->bottom -= _yOffset;
}

void
ToolTray::GetLSizeRect(LPRECT lprc)
{
    GetClientRect(_hwnd, lprc);
    InflateRect(lprc, -_dxMargin, -_dyMargin);
    lprc->top = lprc->bottom - _yOffset;
}

void
ToolTray::Position(int x, int y)
{
    int yMax = GetSystemMetrics(SM_CYSCREEN);
    int yHeight = _yHeight + _dyMargin;
    if(y > yMax - yHeight)
        y = yMax - yHeight;
    x -= _xWidth + _dxMargin;
    if(x < 0)
        x = 0;
    Tray::Position(x, y);
}

void
ToolTray::OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos)
{
    if (lpwpos->flags & SWP_HIDEWINDOW)
       return;

    DOUT(L"ToolTray::OnWindowPosChanged\r\n");

    GetToolRect(&gprcApp[iTool]);
    MoveWindow(gpahwndApp[iTool],
            gprcApp[iTool].left,
            gprcApp[iTool].top,
            gprcApp[iTool].right - gprcApp[iTool].left,
            gprcApp[iTool].bottom - gprcApp[iTool].top,
            TRUE);
    UpdateWindow(gpahwndApp[iTool]);

    GetLSizeRect(&gprcApp[iSize]);
    MoveWindow(gpahwndApp[iSize],
            gprcApp[iSize].left,
            gprcApp[iSize].top,
            gprcApp[iSize].right - gprcApp[iSize].left,
            gprcApp[iSize].bottom - gprcApp[iSize].top,
            TRUE);
    UpdateWindow(gpahwndApp[iSize]);
    //InvalidateRect(gpahwndApp[iSize], NULL, TRUE);
}

//+---------------------------------------------------------------
//
//  ColorTray
//
//---------------------------------------------------------------

LPCOLORTRAY
ColorTray::Create(HINSTANCE hinst,
        		HWND hwndParent,
        		HWND hwndNotify,
        		int iChild)
{
    DOUT(L"ColorTray::Create\r\n");

    //
    //If we haven't yet done our base initialization, do it now
    //
    if(Tray::_fInit == FALSE)
    {
        if(Tray::InitClass(hinst) == 0)
            return NULL;
    }

    LPCOLORTRAY pTray;
    if ((pTray = new ColorTray(hwndParent, hwndNotify, iChild)) != NULL)
    {
        if(NULL == pTray->CreateToolWindow(hinst, hwndParent))
        {
            DOUT(L"ColorTray::Create Failing!\r\n");

            delete pTray;
            pTray = NULL;
        }
    }
    return pTray;
}


ColorTray::ColorTray(HWND hwndParent, HWND hwndNotify, int iChild) :
        Tray(hwndParent, hwndNotify, iChild)
{
    _xWidth = COLORTRAY_WIDTH;
    _yHeight = COLORTRAY_HEIGHT;
}

void
ColorTray::GetColorRect(LPRECT lprc)
{
    GetClientRect(_hwnd, lprc);
    InflateRect(lprc, -_dxMargin, -_dyMargin);
}

void
ColorTray::Position(int x, int y)
{
    int yMax = GetSystemMetrics(SM_CYSCREEN);
    int yHeight = _yHeight + _dyMargin;
    if(y > yMax - yHeight)
        y = yMax - yHeight;
    x += _dxMargin;
    y += _dyMargin;
    Tray::Position(x, y);
}

void
ColorTray::OnWindowPosChanged(HWND hwnd, LPWINDOWPOS lpwpos)
{
    if (lpwpos->flags & SWP_HIDEWINDOW)
       return;

    DOUT(L"ColorTray::OnWindowPosChanged\r\n");

    GetColorRect(&gprcApp[iColor]);
    MoveWindow(gpahwndApp[iColor],
            gprcApp[iColor].left,
            gprcApp[iColor].top,
            gprcApp[iColor].right - gprcApp[iColor].left,
            gprcApp[iColor].bottom - gprcApp[iColor].top,
            TRUE);
    UpdateWindow(gpahwndApp[iColor]);
}

