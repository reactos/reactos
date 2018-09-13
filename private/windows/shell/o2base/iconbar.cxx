//+---------------------------------------------------------------------
//
//   File:       iconbar.cxx
//
//------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <windowsx.h>
#include <shellapi.h>
#include <stdlib.h>

#define ICONBAR_CLASS_NAME L"IconBar"

#define MARGIN_X    4
#define MARGIN_Y    4
#define ICON_WIDTH  32
#define ICON_HEIGHT 32
#define CELL_WIDTH  (ICON_WIDTH + 8)
#define CELL_HEIGHT (ICON_HEIGHT + 8)
#define CAPTION_Y   7
#define FRAME_X     1
#define FRAME_Y     1



    BOOL IconBar::_fInit = FALSE;
    ATOM IconBar::_atomClass = 0;
    COLORREF IconBar::crLtGray;
    COLORREF IconBar::crGray;
    COLORREF IconBar::crDkGray;
    COLORREF IconBar::crBlack;
    HBRUSH IconBar::hbrActiveCaption = NULL;
    HBRUSH IconBar::hbrInActiveCaption = NULL;
    HBRUSH IconBar::hbrWindowFrame = NULL;
    HBRUSH IconBar::hbrSysBox = NULL;
    WORD IconBar::wCnt = 0;

//
//  Routines & definitions for creating windows with custom NON-CLIENT areas
//

#define SetWF(hwnd,wf)  SetWindowLong(hwnd, GWL_STYLE, \
            GetWindowLong(hwnd,GWL_STYLE) | (wf))
#define ClrWF(hwnd,wf)  SetWindowLong(hwnd, GWL_STYLE, \
            GetWindowLong(hwnd,GWL_STYLE) &~(wf))
#define TestWF(hwnd,wf) (GetWindowLong(hwnd,GWL_STYLE) & (wf))

//  Window styles used by ncMsgFilter
#define WF_SIZEFRAME    WS_THICKFRAME
#define WF_SYSMENU      WS_SYSMENU
#define WF_MINIMIZED    WS_MINIMIZE
#define WF_SIZEBOX      0x0002
#define WF_ACTIVE       0x0001



//+---------------------------------------------------------------
//
//  Member: IconBar::Create (static)
//
//---------------------------------------------------------------
LPICONBAR
IconBar::Create(HINSTANCE hinst,
        HWND hwndParent,
        HWND hwndNotify,
        SHORT sIdChild)
{
    //
    //If we haven't yet done our base initialization, do it now
    //
    if(_fInit == FALSE)
    {
        if(IconBar::InitClass(hinst) == 0)
            return NULL;
    }

    LPICONBAR pIconBar;
    if ((pIconBar = new IconBar(hwndParent, hwndNotify, sIdChild)) != NULL)
    {
        if (hwndParent == NULL)
        {
            pIconBar->_hwnd = CreateWindow(
                    ICONBAR_CLASS_NAME,
                    NULL,
                    //WS_BORDER,
                    WS_OVERLAPPED,
                    0,0,0,0,
                    hwndNotify,     // parent (for minimize behavior)
                    NULL,           // menu
                    hinst,
                    pIconBar);
        }
        else
        {
            pIconBar->_hwnd = CreateWindow(
                    ICONBAR_CLASS_NAME,
                    NULL,           // no title
                    WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS,
                    0, 0, 0, 0,     // will be resized
                    hwndParent,
                    (HMENU)sIdChild,
                    hinst,
                    pIconBar);
        }
    }
    return pIconBar;
}


//+---------------------------------------------------------------
//
//  Member: IconBar::IconBar
//
//---------------------------------------------------------------
IconBar::IconBar(HWND hwndParent, HWND hwndNotify, SHORT sIdChild)
{
    _hwnd = NULL;
    _cellsPerRow = 0;
    _numCells = 0;
    _selectedCell = 0;
    _fStuck = FALSE;
    _iCellAtCursor = -1;
    _dxMargin = MARGIN_X;
    _dyMargin = MARGIN_Y;
    _dxCell = CELL_WIDTH;
    _dyCell = CELL_HEIGHT;
    _cyCaption = CAPTION_Y;
    _cxFrame = FRAME_X;
    _cyFrame = FRAME_Y;
    _cxSize = _cyCaption;
    _cySize = _cyCaption;
    if(hwndParent == NULL)
        _fFloating = TRUE;
    else
        _fFloating = FALSE;

    _hwndNotify = hwndNotify;
    _sIdChild = sIdChild;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::Position
//
//---------------------------------------------------------------

void
IconBar::Position(int sTop, int sLeft, int sWhere)
{
    RECT rcBounds;
    GetWindowRect(_hwnd,&rcBounds);
    //int nWidth = (_cellsPerRow * _dxCell) + (2 * _dxMargin);
    int nWidth = rcBounds.right - rcBounds.left;
    int nHeight = rcBounds.bottom - rcBounds.top;
    switch(sWhere)
    {
    default:
    case IBP_LEFTBELOW:
        sLeft -= nWidth;
        break;
    case IBP_RIGHTABOVE:
        sTop -= nHeight;
        break;
    case IBP_RIGHTBELOW:
        break;
    case IBP_LEFTABOVE:
        sLeft -= nWidth;
        sTop -= nHeight;
        break;
    }
    SetWindowPos(_hwnd, 0, //HWND_TOPMOST,
            sLeft, sTop, nWidth, nHeight,
            SWP_NOACTIVATE|SWP_NOZORDER);
}

//+---------------------------------------------------------------
//
//  Member: IconBar::SetCellAspect
//
//  Notes:  Either cWide or cHigh must be > 0
//
//---------------------------------------------------------------
void
IconBar::SetCellAspect( int cWide, int cHigh )
{
    Assert((cWide > 0) || (cHigh > 0));

    int cCells = _numCells;
    RECT rc = { 0, 0, 0, 0 };

    if(cWide > 0)
    {
        rc.right = (cWide * _dxCell) + (2 * _dxMargin);
        if(cHigh > 0)
        {
            rc.bottom = (cHigh * _dyCell) + (2 * _dyMargin);
        }
        else
        {
            cCells /= cWide;

            // Account for a partially filled row.
            if ((_numCells % cWide) > 0)
                cCells++;

            rc.bottom = (cCells * _dyCell) + (2 * _dyMargin);
        }
    }
    else
    {
        rc.bottom = (cHigh * _dyCell) + (2 * _dyMargin);
        cWide = cCells / cHigh;
        // Account for a partially filled column.
        if ((cCells % cHigh) > 0)
            cWide++;

        rc.right = (cWide * _dxCell) + (2 * _dxMargin);
    }
    _cellsPerRow = cWide;
    if(_fFloating)
    {
        //
        //using our special knowledge of non-client metrics...
        //
        InflateRect(&rc,_cxFrame,_cyFrame);
        rc.top -= _cyCaption;
    }
    else
    {
        AdjustWindowRect(&rc, GetWindowLong(_hwnd, GWL_STYLE),
            GetMenu(_hwnd) != NULL);
    }
    ClientToScreen(_hwnd,(LPPOINT)&rc.left);
    ClientToScreen(_hwnd,(LPPOINT)&rc.right);
    SetWindowPos(_hwnd, 0, //HWND_TOPMOST,
            rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOACTIVATE|SWP_NOZORDER);
}

//+---------------------------------------------------------------
//
//  Member: IconBar::~IconBar
//
//---------------------------------------------------------------
IconBar::~IconBar(void)
{
    if (_hwnd != NULL)
        DestroyWindow(_hwnd);

    int i;
    if (--_cUsed == 0)
    {
#ifdef FIXED340BUGS
        for (i = 0; i < _cCache; i++)
            DestroyIcon(_cache[i].hIcon);
        _cCache = 0;
#endif
    }
    for (i = 0; i < _numCells; i++)
    {
        if(_cell[i].hBmp != NULL)
            DeleteObject(_cell[i].hBmp);
    }
}

//+---------------------------------------------------------------
//
//  Function:   IconBarWndProc
//
//---------------------------------------------------------------
extern "C" LRESULT CALLBACK
IconBarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //get a pointer to the IconBar object
    LPICONBAR pIconBar = (LPICONBAR)GetWindowLongPtr(hwnd, 0);

    //if it is NULL, then check for a WM_NCCREATE message
    //   when we get one stash the pointer to our object
    if(pIconBar == NULL)
    {
        if(msg == WM_NCCREATE)
        {
            CREATESTRUCT FAR *lpcs = (CREATESTRUCT FAR *)lParam;
            pIconBar = (LPICONBAR)lpcs->lpCreateParams;
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)pIconBar);
            if(pIconBar->_fFloating)
                return(pIconBar->ncMsgFilter(hwnd, msg, wParam, lParam));
            //
            // drop through to forward the message
            //
        }
        return(DefWindowProc(hwnd, msg, wParam, lParam));
    }

    if(msg == WM_NCDESTROY)
    {
        SetWindowLongPtr(hwnd, 0, 0L);
        //drop through to forward the message to the window procedure
    }

    //handle any window message by passing it to the appropriate
    //member function...
    switch(msg)
    {
    HANDLE_MSG(hwnd, WM_CREATE, pIconBar->OnCreate);
    HANDLE_MSG(hwnd, WM_DESTROY, pIconBar->OnDestroy);
    HANDLE_MSG(hwnd, WM_PAINT, pIconBar->OnPaint);
    HANDLE_MSG(hwnd, WM_GETMINMAXINFO, pIconBar->OnGetMinMaxInfo);
    HANDLE_MSG(hwnd, WM_MOUSEMOVE, pIconBar->OnMouseMove);
    HANDLE_MSG(hwnd, WM_LBUTTONDOWN, pIconBar->OnLButtonDown);
    HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, pIconBar->OnLButtonDown);
    }

    if(pIconBar->_fFloating)
    {
        return pIconBar->ncMsgFilter(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


void
IconBar::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMaxSize.x = 1024;
    lpMinMaxInfo->ptMaxSize.y = 768;
    lpMinMaxInfo->ptMaxPosition.x = lpMinMaxInfo->ptMaxPosition.y = 0;
    lpMinMaxInfo->ptMinTrackSize.x = 8;
    lpMinMaxInfo->ptMinTrackSize.y = 16;
    lpMinMaxInfo->ptMaxTrackSize.x = 1024;
    lpMinMaxInfo->ptMaxTrackSize.y = 768;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::ncCalcRect
//
//---------------------------------------------------------------
void
IconBar::ncCalcRect(HWND hwnd, LPRECT lprc)
{
    InflateRect(lprc, -_cxFrame, -_cyFrame);
    lprc->top += _cyCaption + _cyFrame;
}



//+---------------------------------------------------------------
//
//  Member: IconBar::ncHitTest
//
//---------------------------------------------------------------
LONG
IconBar::ncHitTest(HWND hwnd, POINT pt)
{
    register int x;
    register int y;
    RECT rcWindow;
    RECT rcClient;

    x = pt.x;
    y = pt.y;

    // If the window is minimized and iconic, return HTCAPTION
    if (TestWF(hwnd, WF_MINIMIZED))
        return HTCAPTION;

    // Get Client and Window rects in screen coordinates
    GetWindowRect(hwnd, &rcWindow);
    rcClient = rcWindow;
    ncCalcRect(hwnd, &rcClient);
    //
    //lie about actual client area: count the margin as NC
    //
    InflateRect(&rcClient, -_dxMargin, -_dyMargin);


    if (PtInRect(&rcClient, pt))
    {
        return HTCLIENT;
    }

    // Does the window have a frame?
    if (TestWF(hwnd, WF_SIZEFRAME))
    {
        // Are we touching the frame?
        InflateRect(&rcWindow, -_cxFrame, -_cyFrame);

        if (!PtInRect(&rcWindow, pt))
        {
            // We're somewhere on the window frame.
            if (y >= rcWindow.bottom)
            {
                if (x <= rcWindow.left + _cxSize)
                    return HTBOTTOMLEFT;
                if (x >= rcWindow.right - _cxSize)
                    return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            else if (y <= rcWindow.top)
            {
                if (x <= rcWindow.left + _cxSize)
                    return(HTTOPLEFT);
                if (x >= rcWindow.right - _cxSize)
                    return(HTTOPRIGHT);
                return HTTOP;
            }
            else if (x <= rcWindow.left)
            {
                if (y <= rcWindow.top + _cySize)
                    return HTTOPLEFT;
                if (y >= rcWindow.bottom - _cySize)
                    return HTBOTTOMLEFT;
                return HTLEFT;
            }
            else
            {
                if (y <= rcWindow.top + _cySize)
                    return HTTOPRIGHT;
                if (y >= rcWindow.bottom - _cySize)
                    return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else if(y >= rcClient.top)
    {
        return HTCAPTION;
    }

    // Are we above the client area?
    if (y < rcClient.top)
    {
        if (y <= (rcWindow.top + _cyCaption + _cyFrame) && y >= rcWindow.top)
        {
            // In caption area.  Now see if we're in the system menu box
            if ((TestWF(hwnd, WF_SYSMENU)) &&
                x < rcWindow.left +
                (_cxSize + _cxFrame) && x >= rcWindow.left)
            {
                return HTSYSMENU;
            }

            // In caption area.  Now see if we're in the grow box
            if ((TestWF(hwnd, WF_SIZEBOX)) &&
                x > rcWindow.right -
                (_cxSize + _cxFrame) && x <= rcWindow.right)
            {
                return HTGROWBOX;
            }

            return HTCAPTION;
        }

        // We're hitting on the very top of a window without a sizing frame.
        return HTNOWHERE;
    }

    return HTNOWHERE;
}


//+---------------------------------------------------------------
//
//  Member: IconBar::ncDrawFrame
//
//---------------------------------------------------------------
void
IconBar::ncDrawFrame(HWND hwnd)
{
    HDC  hdc;
    RECT rc;

    hdc = GetWindowDC(hwnd);

    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);

    SelectObject(hdc, hbrWindowFrame);

    // This will work as long as cxFrame and cyFrame are both one.
    FrameRect(hdc, &rc, hbrWindowFrame);

    InflateRect(&rc, -_cxFrame, -_cyFrame);

    rc.top += _cyCaption;
    rc.bottom = rc.top + _cyFrame;
    FillRect(hdc, &rc, hbrWindowFrame);

    rc.top -= _cyCaption;
    rc.bottom -= _cyFrame;

    if (TestWF(hwnd, WF_ACTIVE))
    {
        FillRect(hdc, &rc, hbrActiveCaption);

        if (TestWF(hwnd, WF_SYSMENU))
        {
            SelectObject(hdc, hbrSysBox);
            PatBlt(hdc, rc.left + _cxFrame, rc.top + _cyFrame,
                _cyCaption - 2 * _cyFrame, _cyCaption - 2 * _cyFrame,
                 PATCOPY);
        }

        if (TestWF(hwnd, WF_SIZEBOX))
        {
            SelectObject(hdc, hbrSysBox);
            PatBlt(hdc, rc.right + _cxFrame - _cxSize, rc.top +
                _cyFrame, _cyCaption - 2 * _cyFrame, _cyCaption - 2 *
                _cyFrame, PATCOPY);
        }
    }
    else
    {
        FillRect(hdc, &rc, hbrInActiveCaption);
    }

    ReleaseDC(hwnd, hdc);
}


//+---------------------------------------------------------------
//
//  Member: IconBar::IconBar
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
IconBar::ncMsgFilter(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

    case WM_INITMENU:
        EnableMenuItem((HMENU)wParam, SC_SIZE, MF_GRAYED);
        EnableMenuItem((HMENU)wParam, SC_MOVE,     MF_ENABLED);
        EnableMenuItem((HMENU)wParam, SC_MINIMIZE, MF_GRAYED);
        EnableMenuItem((HMENU)wParam, SC_MAXIMIZE, MF_GRAYED);
        EnableMenuItem((HMENU)wParam, SC_RESTORE,  MF_GRAYED);
        EnableMenuItem((HMENU)wParam, SC_TASKLIST, MF_ENABLED);
        EnableMenuItem((HMENU)wParam, SC_CLOSE,    MF_ENABLED);
        return 0L;

    case WM_COMMAND:
        if (lParam == 0L && LOWORD(wParam) >= SC_SIZE)
            PostMessage(hwnd, WM_SYSCOMMAND, wParam, lParam);

        break;

    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0)
        {
        //
        //  Drop the system menu
        //
        case SC_KEYMENU:
            if (LOWORD(lParam) != ' ')
            {
                break;
            }
            else
            {
                wParam = HTSYSMENU;
            }

            //
            // Fall through!
            //

        case SC_MOUSEMENU:
            if ((wParam & 0x000F) == HTSYSMENU)
            {
                RECT rc;

                GetWindowRect(hwnd, &rc);
                TrackPopupMenu(GetSystemMenu(hwnd, FALSE),
                     0,
                     rc.left + _cxFrame, rc.top + _cyFrame + _cyCaption,
                     0,
                     hwnd,
                     NULL);

                return 0L;
            }
            break;

        case SC_MOVE:
            //
            //  Turn off the WS_THICKFRAME style bit during a
            //  move operation so we get a "thin" move frame
            //
            if (TestWF(hwnd, WS_THICKFRAME))
            {
                ClrWF(hwnd, WS_THICKFRAME);
                lParam = DefWindowProc(hwnd, msg, wParam,
                     lParam);
                SetWF(hwnd, WS_THICKFRAME);
                return lParam;
            }
            break;
        }
        break;

    case WM_NCLBUTTONDOWN:
    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONUP:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONUP:
    case WM_NCMBUTTONDBLCLK:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


//+---------------------------------------------------------------
//
//  Member: IconBar::InitClass (static)
//
//---------------------------------------------------------------
ATOM
IconBar::InitClass(HINSTANCE hinst)
{
    crLtGray = PALETTEINDEX(7);
    crGray = PALETTEINDEX(248);
    crDkGray = PALETTEINDEX(249);
    crBlack = PALETTEINDEX(0);
    // register our window class
    WNDCLASS wc;
    wc.style = CS_NOCLOSE | CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = IconBarWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;        // for pointer to IconBar object
    wc.hInstance = hinst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockBrush(LTGRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = ICONBAR_CLASS_NAME;
    if((IconBar::_atomClass = RegisterClass(&wc)) != 0)
    {
        IconBar::_fInit = TRUE;
    }
    return _atomClass;
}


//+---------------------------------------------------------------
//
//  Member: IconBar::AddCell (internal version)
//
//---------------------------------------------------------------
int
IconBar::AddCell(LPICONCELL pCells,
        int i,
        CLSID& clsid,
        HICON hIcon,
        LPWSTR szTitle)
{
    if (i < MAX_CELLS)
    {
        pCells[i].clsid = clsid;
        pCells[i].hBmp = NULL;
        pCells[i].hIcon = hIcon;
        lstrcpy(pCells[i].achTitle,szTitle);
        pCells[i].hBmp = NULL;

#ifdef LATER
        HDC hdcScreen = GetDC(NULL);
        HDC hdcSrc = CreateCompatibleDC(hdcScreen);
        if (hdcSrc != NULL)
        {
            HBITMAP hbmpColor = CreateCompatibleBitmap(hdcScreen,
                    ICON_WIDTH,ICON_HEIGHT);
            HBITMAP hbmpOldSrc = (HBITMAP)SelectObject(hdcSrc,hbmpColor);
            SetMapMode(hdcSrc,MM_TEXT);
            DrawIcon(hdcSrc,0,0,hIcon);
            SelectObject(hdcSrc,hbmpOldSrc);
            DeleteDC(hdcSrc);

            //HANDLE hDib = CopyBmpToDib(hbmpColor,NULL);
            //DeleteObject(hbmpColor);
            //
            //edit DIB bits such that we transform to grey-scale
            //  Y = (2R + 5G + 1B) >> 3
            //
            //hbmpColor = ConvertDibToBmp(hDib);
            //GlobalFree(hDib);
            pCells[i].hBmp = hbmpColor;
        }
        ReleaseDC(NULL,hdcScreen);
#endif

        ++i;
    }
    return i;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::AddCell (public version)
//
//---------------------------------------------------------------
void
IconBar::AddCell(CLSID& clsid, HICON hIcon, LPWSTR szTitle)
{
    _numCells = AddCell(&_cell[0], _numCells, clsid, hIcon, szTitle);
}

    int IconBar::_cUsed = 0;              // outstanding users of cached cells
    int IconBar::_cCache = 0;             // total cached cells
    IconBar::IconCell IconBar::_cache[MAX_CELLS];  // a cache array of icon cells

//+---------------------------------------------------------------
//
//  Member: IconBar::AddCache (private)
//
//  Notes:  Either cWide or cHigh must be > 0
//
//---------------------------------------------------------------
void
IconBar::AddCache(void)
{
    _cUsed++;
    for(int i = 0; i < _cCache; i++)
    {
        AddCell(_cache[i].clsid, _cache[i].hIcon, _cache[i].achTitle);
    }
}

//+---------------------------------------------------------------
//
//  Member: IconBar::AddCellsFromRegDB
//
//  pszFilter will be either:
//      L"\\Insertable"
//  OR
//      L"\\Control"
//
//---------------------------------------------------------------
HRESULT
IconBar::AddCellsFromRegDB(LPWSTR pszFilter)
{
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(_hwnd, GWLP_HINSTANCE);

    HKEY  hkRoot;
    WCHAR szBuff[128], szValue[64], achName[64];
    DWORD dwIndex;
    LONG  cb;
    HICON hIcon = NULL;

    if(_cCache != 0)
    {
        AddCache();
        return NOERROR;
    }

    // enumerate the top-level keys
    if (RegOpenKey(HKEY_CLASSES_ROOT, L"CLSID", &hkRoot) == ERROR_SUCCESS)
    {
        for (dwIndex = 0;
            RegEnumKey(hkRoot,
                       dwIndex,
                       szBuff,
                       sizeof(szBuff)/sizeof(WCHAR)) == ERROR_SUCCESS;
            ++dwIndex)
        {
            lstrcat(szBuff,pszFilter);

            cb = sizeof(szValue)/sizeof(WCHAR);

            if (RegQueryValue(hkRoot, (LPWSTR)szBuff, szValue, &cb)
                    == ERROR_SUCCESS)
            {
                //
                // extract the (presentable) class name
                //

                LPWSTR lpstr;

                for (lpstr = szBuff; *lpstr != L'\\'; lpstr++);
                    *lpstr = L'\0';

                cb = sizeof(achName)/sizeof(WCHAR);

                RegQueryValue(hkRoot, (LPWSTR) szBuff, achName, &cb);

                // query for the "DefaultIcon" entry

                lstrcat(szBuff, L"\\DefaultIcon");

                cb = sizeof(szValue)/sizeof(WCHAR);

                if (RegQueryValue(hkRoot, (LPWSTR) szBuff, szValue,
                        &cb) == ERROR_SUCCESS)
                {
                    UINT iIcon;
                    for (lpstr = szValue; *lpstr && *lpstr != L','; lpstr++)
                        ;

                    if (*lpstr == L',')
                    {
                        *lpstr++ = L'\0';
                        iIcon = (int)wcstol(lpstr, NULL, 10);
                    }
                    else
                    {
                        iIcon = 0;
                    }
                    hIcon = ExtractIcon(hinst, szValue, iIcon);
                }

                //  Provide default icon if class doesn't specify
                if (hIcon == NULL)
                    hIcon = LoadIcon(NULL, IDI_APPLICATION);

                if (hIcon != (HICON) -1)
                {
                    CLSID   clsid;

                    // convert the string to a class id
                    for (lpstr = szBuff; *lpstr != L'\\'; lpstr++)
                        ;

                    *lpstr = L'\0';

                    // add (CLSID, HICON, DESCRIPTION) to our list.
                    if (SUCCEEDED(CLSIDFromString(szBuff, &clsid)))
                    {
                        _cCache = AddCell(_cache, _cCache,
                            clsid, hIcon, achName);
                    }
                }
            }
        }
        RegCloseKey(hkRoot);
    }

    AddCache();

    return NOERROR; //BUGBUG sure?
}



//+---------------------------------------------------------------
//
//  Member: IconBar::SelectCell
//
//---------------------------------------------------------------

void
IconBar::SelectCell(int iCell, BOOL fStick)
{
    // if they have made a new selection...
    if (iCell != _selectedCell)
    {
        // record the new selection and invalidate the cell rectangles
        InvalidateCell(_hwnd, _selectedCell);
        _selectedCell = iCell;
        InvalidateCell(_hwnd, _selectedCell);

        // send notification of selection
        FORWARD_WM_COMMAND(_hwndNotify, _sIdChild, _hwnd,
            IBI_NEWSELECTION, SendMessage);
    }

    // regardless of whether it was a new selection, if they
    // want the button stuck then so be it!
    _fStuck = fStick;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::GetSelectedClassId
//
//---------------------------------------------------------------
CLSID&
IconBar::GetSelectedClassId(void)
{
    return _cell[_selectedCell].clsid;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::GetStatusMessage
//
//---------------------------------------------------------------
LPWSTR
IconBar::GetStatusMessage(void)
{
    LPWSTR lpstr;
    if (_iCellAtCursor == -1)
        lpstr = NULL;
    else
        lpstr = _cell[_iCellAtCursor].achTitle;
    return lpstr;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::GetCellPoint
//
//---------------------------------------------------------------
void
IconBar::GetCellPoint(LPPOINT lppt, int i)
{
    lppt->x = _dxMargin + (i % _cellsPerRow) * _dxCell;
    lppt->y = _dyMargin + (i / _cellsPerRow) * _dyCell;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::GetCellRect
//
//---------------------------------------------------------------
void
IconBar::GetCellRect(LPRECT lprc, int i)
{
    POINT pt;
    GetCellPoint(&pt, i);
    lprc->left = pt.x;
    lprc->top = pt.y;
    lprc->right = lprc->left + _dxCell;
    lprc->bottom = lprc->top + _dyCell;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::InvalidateCell
//
//---------------------------------------------------------------
void
IconBar::InvalidateCell(HWND hwnd, int i)
{
    if(i < 0 || i >= _numCells)
    {
        return;
    }

    RECT rc;
    GetCellRect(&rc, i);
    InvalidateRect(hwnd, &rc, TRUE);
}


//+---------------------------------------------------------------
//
//  Member: IconBar::OnPaint
//
//---------------------------------------------------------------
void
IconBar::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    int i;
    POINT pt;
    if(_numCells)
    {
        //
        //Draw an outline around the block of icons
        //
        RECT r;
        GetCellPoint(&pt, 0);
        r.top = pt.y - 1;
        r.left = pt.x - 1;
        GetCellPoint(&pt, _numCells - 1);
        r.bottom = pt.y + _dyCell;
        r.right  = pt.x + _dxCell;
        FrameRect(ps.hdc, &r, GetStockBrush(BLACK_BRUSH));
    }
    for (i = 0; i < _numCells; i++)
    {
        GetCellPoint(&pt, i);
        Draw(ps.hdc, pt, i == _selectedCell, i);
    }

    EndPaint(hwnd, &ps);
    return;
}


//+---------------------------------------------------------------
//
//  Member: IconBar::OnMouseMove
//
//---------------------------------------------------------------
void
IconBar::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    POINT pt; pt.x = x; pt.y = y;
    int iCell = CellIndexFromPoint(pt);

    if (iCell != _iCellAtCursor)
    {
        _iCellAtCursor = iCell;
        // send status text change notification
        FORWARD_WM_COMMAND(_hwndNotify, _sIdChild, _hwnd, IBI_STATUSAVAIL,
                SendMessage);
    }
}

//+---------------------------------------------------------------
//
//  Member: IconBar::OnLButtonDown
//
//---------------------------------------------------------------
void
IconBar::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    POINT pt; pt.x = x; pt.y = y;
    int newSel = CellIndexFromPoint(pt);    // which cell that was hit?
    if (newSel != -1)
        SelectCell(newSel, keyFlags & MK_CONTROL);
}

//+---------------------------------------------------------------
//
//  Member: IconBar::CellIndexFromPoint
//
//---------------------------------------------------------------
int
IconBar::CellIndexFromPoint(POINT pt)
{
    int i; RECT rc;
    for (i = 0; i < _numCells; i++)
    {
        GetCellRect(&rc, i);
        if (PtInRect(&rc, pt))
            break;
    }
    return i < _numCells ? i : -1;
}

//+---------------------------------------------------------------
//
//  Member: IconBar::Draw
//
//---------------------------------------------------------------
void
IconBar::Draw(HDC hdc, POINT pt, BOOL fDown, int iCell )
{
    if(hdc == NULL)
        return;

    HICON hIcon = _cell[iCell].hIcon;
    //
    // Erase the background
    //
    RECT rc;
    SetRect(&rc, pt.x, pt.y, pt.x + _dxCell, pt.y + _dyCell);
    FillRect(hdc, &rc, GetStockBrush(LTGRAY_BRUSH));
    FrameRect(hdc, &rc, GetStockBrush(BLACK_BRUSH));

    //
    //Draw beveled edge around icon...
    //
    RECT rShade;
    HBRUSH hbrGrey = GetStockBrush(GRAY_BRUSH);
    HBRUSH hbrWhite = GetStockBrush(WHITE_BRUSH);
    if(fDown)
    {
        //DOWN:
        //
        //draw darker (topleft) half of border
        //
        rShade.top = rc.top;
        rShade.left = rc.left;
        rShade.bottom = rc.top + 3;
        rShade.right = rc.right - 1;
        FillRect(hdc, &rShade, hbrGrey);
        rShade.top = rc.top;
        rShade.left = rc.left;
        rShade.bottom = rc.bottom - 1;
        rShade.right = rc.left + 3;
        FillRect(hdc, &rShade, hbrGrey);

        DrawIcon(hdc, pt.x + 6, pt.y + 6, hIcon);
    }
    else
    {
        //UP:
        //
        //draw brighter (topLeft) half of border
        //
        rShade.top = rc.top;
        rShade.left = rc.left;
        rShade.bottom = rc.top + 2;
        rShade.right = rc.right - 1;
        FillRect(hdc, &rShade, hbrWhite);
        rShade.top = rc.top;
        rShade.left = rc.left;
        rShade.bottom = rc.bottom - 1;
        rShade.right = rc.left + 2;
        FillRect(hdc, &rShade, hbrWhite);
        //
        //draw darkter (bottomRight) half of border
        //
        rShade.top = rc.top;
        rShade.left = rc.right - 2;
        rShade.bottom = rc.bottom - 1;
        rShade.right = rc.right - 1;
        FillRect(hdc, &rShade, hbrGrey);
        rShade.top = rc.bottom - 2;
        rShade.left = rc.left;
        rShade.bottom = rc.bottom - 1;
        rShade.right = rc.right - 1;
        FillRect(hdc, &rShade, hbrGrey);
        //
        rShade.top = rc.top + 1;
        rShade.left = rc.right - 3;
        rShade.bottom = rc.bottom - 1;
        rShade.right = rc.right - 2;
        FillRect(hdc, &rShade, hbrGrey);
        rShade.top = rc.bottom - 3;
        rShade.left = rc.left + 1;
        rShade.bottom = rc.bottom - 2;
        rShade.right = rc.right - 1;
        FillRect(hdc, &rShade, hbrGrey);

        if(_cell[iCell].hBmp)
        {
            //HDC hdcSrc = CreateCompatibleDC(hdc);
            HDC hdcSrc = CreateCompatibleDC(NULL);
            HBITMAP hbmpOldSrc = (HBITMAP)SelectObject(hdcSrc,_cell[iCell].hBmp);
            BitBlt(hdc, pt.x + 4, pt.y + 4, ICON_WIDTH, ICON_HEIGHT,
                    hdcSrc, 0, 0, SRCCOPY);
            SelectObject(hdcSrc,hbmpOldSrc);
            DeleteDC(hdcSrc);
        }
        else
        {
            DrawIcon(hdc, pt.x + 4, pt.y + 4, hIcon);
        }
    }
}
