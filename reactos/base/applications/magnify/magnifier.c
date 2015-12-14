/*
 * PROJECT:     ReactOS Magnify
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/magnify/magnifier.c
 * PURPOSE:     Magnification of parts of the screen.
 * AUTHORS:
 *     Marc Piulachs <marc.piulachs@codexchange.net>
 *     David Quintana <gigaherz@gmail.com>
 */

/* TODO: Support AppBar types other than ABE_TOP */

#include "magnifier.h"

#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <shellapi.h>
#include <windowsx.h>

#include "resource.h"

#define APPMSG_NOTIFYICON (WM_APP+1)
#define APPMSG_APPBAR     (WM_APP+2)

const TCHAR szWindowClass[] = TEXT("MAGNIFIER");

/* Global Variables */
HINSTANCE hInst;
HWND hMainWnd;

#define MAX_LOADSTRING 100
TCHAR szTitle[MAX_LOADSTRING];

#define TIMER_SPEED   1
#define REPAINT_SPEED 100

DWORD lastTicks = 0;

HWND hDesktopWindow = NULL;

NOTIFYICONDATA nid;
HICON notifyIcon;
HMENU notifyMenu;
HWND  hOptionsDialog;
BOOL  bOptionsDialog = FALSE;
BOOL  bRecreateOffscreenDC = TRUE;
LONG  sourceWidth = 0;
LONG  sourceHeight = 0;
HDC     hdcOffscreen = NULL;
HBITMAP hbmpOffscreen = NULL;
HANDLE  hbmpOld;
POINT ptDragOffset;
INT nearEdge;

/* Current magnified area */
POINT cp;

/* Last positions */
POINT pMouse;
POINT pCaret;
POINT pFocus;
HWND pCaretWnd;
HWND pFocusWnd;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AboutProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    OptionsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    WarningProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
          SetProcessDefaultLayout(LAYOUT_RTL);
          break;

        default:
          break;
    }

    /* Initialize global strings */
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAGNIFIER));

    /* Main message loop */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


    SelectObject(hdcOffscreen, hbmpOld);
    DeleteObject (hbmpOffscreen);
    DeleteDC(hdcOffscreen);

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName     = NULL; //MAKEINTRESOURCE(IDC_MAGNIFIER);
    wc.lpszClassName    = szWindowClass;

    return RegisterClass(&wc);
}

void DoAppBarStuff(DWORD mode)
{
    UINT uState;
    APPBARDATA data = {0};
    data.cbSize = sizeof(data);
    data.hWnd = hMainWnd;
    data.uCallbackMessage = APPMSG_APPBAR;

    if (mode == ABM_NEW || mode == ABM_SETPOS)
    {
        HWND hWndOrder = HWND_BOTTOM;
        int rcw, rch;
        RECT rcWorkArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

        if(mode == ABM_NEW)
        {
            SHAppBarMessage(ABM_NEW, &data);

            switch(AppBarConfig.uEdge)
            {
                case ABE_LEFT:
                    data.rc.top = rcWorkArea.top;
                    data.rc.bottom = rcWorkArea.bottom;
                    data.rc.left = rcWorkArea.left;
                    data.rc.right = data.rc.left + AppBarConfig.appBarSizes.left;
                    break;
                case ABE_TOP:
                    data.rc.left = rcWorkArea.left;
                    data.rc.right = rcWorkArea.right;
                    data.rc.top = rcWorkArea.top;
                    data.rc.bottom = data.rc.top + AppBarConfig.appBarSizes.top;
                    break;
                case ABE_RIGHT:
                    data.rc.top = rcWorkArea.top;
                    data.rc.bottom = rcWorkArea.bottom;
                    data.rc.right = rcWorkArea.left;
                    data.rc.left = data.rc.right - AppBarConfig.appBarSizes.right;
                    break;
                case ABE_BOTTOM:
                    data.rc.left = rcWorkArea.left;
                    data.rc.right = rcWorkArea.right;
                    data.rc.bottom = rcWorkArea.bottom;
                    data.rc.top = data.rc.bottom - AppBarConfig.appBarSizes.bottom;
                    break;
            }
        }
        else
        {
            GetWindowRect(hMainWnd, &data.rc);
        }

        data.uEdge = AppBarConfig.uEdge;
        uState = SHAppBarMessage(ABM_QUERYPOS, &data);

        uState = SHAppBarMessage(ABM_SETPOS, &data);

        rcw = data.rc.right-data.rc.left;
        rch = data.rc.bottom-data.rc.top;

        uState = SHAppBarMessage(ABM_GETSTATE, &data);
        if(uState & ABS_ALWAYSONTOP)
            hWndOrder = HWND_TOPMOST;

        SetWindowPos(hMainWnd, hWndOrder, data.rc.left, data.rc.top, rcw, rch, SWP_SHOWWINDOW|SWP_NOCOPYBITS);

    }
    else if(mode == ABM_GETSTATE)
    {
        HWND hWndOrder = HWND_BOTTOM;
        uState = SHAppBarMessage(ABM_GETSTATE, &data);
        if(uState & ABS_ALWAYSONTOP)
            hWndOrder = HWND_TOPMOST;
        SetWindowPos(hMainWnd, hWndOrder, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }
    else if(mode == ABM_ACTIVATE)
    {
        SHAppBarMessage(ABM_ACTIVATE, &data);
    }
    else if(mode == ABM_WINDOWPOSCHANGED)
    {
        SHAppBarMessage(ABM_WINDOWPOSCHANGED, &data);
    }
    else if(mode == ABM_REMOVE)
    {
        SHAppBarMessage(ABM_REMOVE, &data);
    }
}

void AttachAppBar(INT uEdge)
{
    if (AppBarConfig.uEdge == uEdge)
        return;

    if(AppBarConfig.uEdge < 0 && uEdge >= 0)
    {
        SetWindowLongPtr(hMainWnd, GWL_STYLE, GetWindowLongPtr(hMainWnd, GWL_STYLE) & (~WS_CAPTION));
    }
    else if(uEdge < 0 && AppBarConfig.uEdge>=0)
    {
        SetWindowLongPtr(hMainWnd, GWL_STYLE, GetWindowLongPtr(hMainWnd, GWL_STYLE) | WS_CAPTION);
        SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
    }

    if(AppBarConfig.uEdge >= 0)
    {
        DoAppBarStuff(ABM_REMOVE);
    }

    if (uEdge >=0)
    {
        AppBarConfig.uEdge = uEdge;
        DoAppBarStuff(ABM_NEW);
    }
    else
    {
        RECT rc = AppBarConfig.rcFloating;
        SetWindowPos(hMainWnd, HWND_TOPMOST, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 0);
    }

    AppBarConfig.uEdge = uEdge;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT rc;
    DWORD exStyles = WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT;
    DWORD dwStyles = WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;

    /* Load settings from registry */
    LoadSettings();

    rc = AppBarConfig.rcFloating;

    hInst = hInstance; // Store instance handle in our global variable

    if (AppBarConfig.uEdge<0)
    {
        dwStyles |= WS_CAPTION;
        exStyles |= WS_EX_TOPMOST;
    }

    /* Create the Window */
    hMainWnd = CreateWindowEx(
        exStyles,
        szWindowClass,
        szTitle,
        dwStyles,
        rc.left,
        rc.top,
        rc.right-rc.left,
        rc.bottom-rc.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hMainWnd)
        return FALSE;

    if (AppBarConfig.uEdge>=0) DoAppBarStuff(ABM_NEW);
    else SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);

    // In Windows 2003's Magnifier, the "Start Minimized" setting
    // refers exclusively to the options dialog, not the main window itself.
    hOptionsDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOGOPTIONS), hMainWnd, OptionsProc);
    if (bStartMinimized)
        ShowWindow(hOptionsDialog, SW_HIDE);
    else
        ShowWindow(hOptionsDialog, SW_SHOW);

    if (bShowWarning)
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_WARNINGDIALOG), hMainWnd, WarningProc);

    return TRUE;
}

void Refresh()
{
    if (!IsIconic(hMainWnd))
    {
        /* Invalidate the client area forcing a WM_PAINT message */
        InvalidateRgn(hMainWnd, NULL, TRUE);
    }
}

void GetBestOverlapWithMonitors(LPRECT rect)
{
    int rcLeft, rcTop;
    int rcWidth, rcHeight;
    RECT rcMon;
    HMONITOR hMon = MonitorFromRect(rect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(info);

    GetMonitorInfo(hMon, &info);

    rcMon = info.rcMonitor;

    rcLeft = rect->left;
    rcTop = rect->top;
    rcWidth  = (rect->right - rect->left);
    rcHeight = (rect->bottom - rect->top);

    if (rcLeft < rcMon.left)
       rcLeft = rcMon.left;

    if (rcTop < rcMon.top)
       rcTop = rcMon.top;

    if (rcLeft > (rcMon.right - rcWidth))
        rcLeft = (rcMon.right - rcWidth);

    if (rcTop > (rcMon.bottom - rcHeight))
        rcTop = (rcMon.bottom - rcHeight);

    OffsetRect(rect, (rcLeft-rect->left), (rcTop-rect->top));
}

void Draw(HDC aDc)
{
    HDC desktopHdc = NULL;

    RECT sourceRect, intersectedRect;
    RECT targetRect, appRect;
    DWORD rop = SRCCOPY;
    CURSORINFO cinfo;
    ICONINFO iinfo;

    int AppWidth, AppHeight;

    if (bInvertColors)
        rop = NOTSRCCOPY;

    desktopHdc = GetDC(0);

    GetClientRect(hMainWnd, &appRect);
    AppWidth  = (appRect.right - appRect.left);
    AppHeight = (appRect.bottom - appRect.top);

    ZeroMemory(&cinfo, sizeof(cinfo));
    ZeroMemory(&iinfo, sizeof(iinfo));
    cinfo.cbSize = sizeof(cinfo);
    GetCursorInfo(&cinfo);
    GetIconInfo(cinfo.hCursor, &iinfo);

    targetRect = appRect;
    ClientToScreen(hMainWnd, (POINT*)&targetRect.left);
    ClientToScreen(hMainWnd, (POINT*)&targetRect.right);

    if (bRecreateOffscreenDC || !hdcOffscreen)
    {
        bRecreateOffscreenDC = FALSE;

        if(hdcOffscreen)
        {
            SelectObject(hdcOffscreen, hbmpOld);
            DeleteObject (hbmpOffscreen);
            DeleteDC(hdcOffscreen);
        }

        sourceWidth  = AppWidth / iZoom;
        sourceHeight = AppHeight / iZoom;

         /* Create a memory DC compatible with client area DC */
        hdcOffscreen = CreateCompatibleDC(desktopHdc);

        /* Create a bitmap compatible with the client area DC */
        hbmpOffscreen = CreateCompatibleBitmap(
            desktopHdc,
            sourceWidth,
            sourceHeight);

        /* Select our bitmap in memory DC and save the old one */
        hbmpOld = SelectObject(hdcOffscreen , hbmpOffscreen);
    }

    GetWindowRect(hDesktopWindow, &sourceRect);
    sourceRect.left = (cp.x) - (sourceWidth /2);
    sourceRect.top = (cp.y) - (sourceHeight /2);
    sourceRect.right = sourceRect.left + sourceWidth;
    sourceRect.bottom = sourceRect.top + sourceHeight;

    GetBestOverlapWithMonitors(&sourceRect);

    /* Paint the screen bitmap to our in memory DC */
    BitBlt(
        hdcOffscreen,
        0,
        0,
        sourceWidth,
        sourceHeight,
        desktopHdc,
        sourceRect.left,
        sourceRect.top,
        rop);

    if (IntersectRect(&intersectedRect, &sourceRect, &targetRect))
    {
        OffsetRect(&intersectedRect, -sourceRect.left, -sourceRect.top);
        FillRect(hdcOffscreen, &intersectedRect, GetStockObject(DC_BRUSH));
    }

    /* Draw the mouse pointer in the right position */
    DrawIcon(
        hdcOffscreen ,
        pMouse.x - iinfo.xHotspot - sourceRect.left, // - 10,
        pMouse.y - iinfo.yHotspot - sourceRect.top, // - 10,
        cinfo.hCursor);

    /* Blast the stretched image from memory DC to window DC */
    StretchBlt(
        aDc,
        0,
        0,
        AppWidth,
        AppHeight,
        hdcOffscreen,
        0,
        0,
        sourceWidth,
        sourceHeight,
        SRCCOPY | NOMIRRORBITMAP);

    /* Cleanup */
    if (iinfo.hbmMask)
        DeleteObject(iinfo.hbmMask);
    if (iinfo.hbmColor)
        DeleteObject(iinfo.hbmColor);
    ReleaseDC(hDesktopWindow, desktopHdc);
}

void HandleNotifyIconMessage(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt;

    switch(lParam)
    {
        case WM_LBUTTONUP:
            PostMessage(hMainWnd, WM_COMMAND, IDM_OPTIONS, 0);
            break;
        case WM_RBUTTONUP:
            GetCursorPos (&pt);
            TrackPopupMenu(notifyMenu, 0, pt.x, pt.y, 0, hWnd, NULL);
            break;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;

    switch (message)
    {
        case WM_TIMER:
        {
            BOOL hasMoved = FALSE;

            GUITHREADINFO guiInfo;
            guiInfo.cbSize = sizeof(guiInfo);

            GetGUIThreadInfo(0, &guiInfo);

            if (bFollowMouse)
            {
                POINT pNewMouse;

                //Get current mouse position
                GetCursorPos (&pNewMouse);

#define PointsAreEqual(pt1, pt2) (((pt1).x == (pt2).x) && ((pt1).y == (pt2).y))

                //If mouse has moved ...
                if (!PointsAreEqual(pMouse, pNewMouse))
                {
                    //Update to new position
                    pMouse = pNewMouse;
                    cp = pNewMouse;
                    hasMoved = TRUE;
                }
            }

            if(guiInfo.hwndActive != hMainWnd)
            {
                if (bFollowCaret)
                {
                    if (guiInfo.hwndCaret)
                    {
                        POINT ptCaret;
                        ptCaret.x = (guiInfo.rcCaret.left + guiInfo.rcCaret.right) / 2;
                        ptCaret.y = (guiInfo.rcCaret.top + guiInfo.rcCaret.bottom) / 2;

                        if ((pCaretWnd != guiInfo.hwndCaret) || !PointsAreEqual(pCaret, ptCaret))
                        {
                            //Update to new position
                            pCaret = ptCaret;
                            pCaretWnd = guiInfo.hwndCaret;
                            if(!hasMoved)
                            {
                                ClientToScreen (guiInfo.hwndCaret, (LPPOINT) &ptCaret);
                                cp = ptCaret;
                                hasMoved = TRUE;
                            }
                        }
                    }
                    else
                    {
                        pCaretWnd = NULL;
                    }
                }

                if (bFollowFocus)
                {
                    if(guiInfo.hwndFocus && !guiInfo.hwndCaret)
                    {
                        POINT ptFocus;
                        RECT activeRect;

                        //Get current control focus
                        GetWindowRect(guiInfo.hwndFocus, &activeRect);
                        ptFocus.x = (activeRect.left + activeRect.right) / 2;
                        ptFocus.y = (activeRect.top + activeRect.bottom) / 2;

                        if((guiInfo.hwndFocus != pFocusWnd) || !PointsAreEqual(pFocus, ptFocus))
                        {
                            //Update to new position
                            pFocus = ptFocus;
                            pFocusWnd = guiInfo.hwndFocus;
                            if(!hasMoved)
                            {
                                cp = ptFocus;
                                hasMoved = TRUE;
                            }
                        }
                    }
                    else
                    {
                        pFocusWnd = NULL;
                    }
                }
            }

            if(!hasMoved)
            {
                DWORD newTicks = GetTickCount();
                DWORD elapsed = (newTicks - lastTicks);
                if(elapsed > REPAINT_SPEED)
                {
                    hasMoved = TRUE;
                }
            }

            if(hasMoved)
            {
                lastTicks = GetTickCount();
                Refresh();
            }

            return 0;
        }

        case WM_COMMAND:
        {
            wmId = LOWORD(wParam);
            /* Parse the menu selections */
            switch (wmId)
            {
                case IDM_OPTIONS:
                    if(bOptionsDialog)
                    {
                        ShowWindow(hOptionsDialog, SW_HIDE);
                    }
                    else
                    {
                        ShowWindow(hOptionsDialog, SW_SHOW);
                    }
                    break;
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutProc);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC dc;
            dc = BeginPaint(hWnd, &PaintStruct);
            Draw(dc);
            EndPaint(hWnd, &PaintStruct);
            return 0;
        }

        case WM_CONTEXTMENU:
            TrackPopupMenu(notifyMenu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, NULL);
            return 0;

        case WM_LBUTTONDOWN:
        {
            RECT rc;
            POINT pt;
            SetCapture(hWnd);

            GetCursorPos(&pt);
            GetWindowRect(hWnd, &rc);
            ptDragOffset.x = pt.x - rc.left;
            ptDragOffset.y = pt.y - rc.top;

            nearEdge = AppBarConfig.uEdge;

            break;
        }
        case WM_MOUSEMOVE:
            if(GetCapture() == hWnd)
            {
                RECT rc;
                POINT pt;
                RECT rcWorkArea;
                SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
                GetCursorPos(&pt);
                GetWindowRect(hWnd, &rc);

                if(AppBarConfig.uEdge>=0)
                {
                    if (pt.x >= rcWorkArea.left && pt.x <= rcWorkArea.right &&
                        pt.y >= rcWorkArea.top && pt.y <= rcWorkArea.bottom)
                    {
                        AttachAppBar(-2);

                        // Fixup offset
                        GetWindowRect(hWnd, &rc);
                        ptDragOffset.x = (rc.right-rc.left)/2;
                        ptDragOffset.y = 2;

                        rc.left = pt.x - ptDragOffset.x;
                        rc.top = pt.y - ptDragOffset.y;

                        SetWindowPos(hWnd, HWND_TOPMOST, rc.left, rc.top, 0, 0, SWP_NOSIZE);
                    }
                }
                else
                {
                    if(pt.x <= rcWorkArea.left+8 && nearEdge != ABE_LEFT)
                    {
                        AttachAppBar(ABE_LEFT);
                        nearEdge = ABE_LEFT;
                    }
                    else if(pt.y <= rcWorkArea.top+8 && nearEdge != ABE_TOP)
                    {
                        AttachAppBar(ABE_TOP);
                        nearEdge = ABE_TOP;
                    }
                    else if(pt.x >= rcWorkArea.right-8 && nearEdge != ABE_RIGHT)
                    {
                        AttachAppBar(ABE_RIGHT);
                        nearEdge = ABE_RIGHT;
                    }
                    else if(pt.y >= rcWorkArea.bottom-8 && nearEdge != ABE_BOTTOM)
                    {
                        AttachAppBar(ABE_BOTTOM);
                        nearEdge = ABE_BOTTOM;
                    }
                    else
                    {
                        rc.left = pt.x - ptDragOffset.x;
                        rc.top = pt.y - ptDragOffset.y;

                        SetWindowPos(hWnd, HWND_TOPMOST, rc.left, rc.top, 0, 0, SWP_NOSIZE);
                        nearEdge = -1;
                    }
                }

                pMouse = pt;
                Refresh();
            }
            break;
        case WM_LBUTTONUP:
            if(GetCapture() == hWnd)
            {
                if (AppBarConfig.uEdge>=0)
                    DoAppBarStuff(ABM_GETSTATE);
                else
                    SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
                ReleaseCapture();
            }

        case WM_SIZE:
            if(AppBarConfig.uEdge>=0) DoAppBarStuff(ABM_SETPOS);
            /* fallthrough */
        case WM_DISPLAYCHANGE:
            bRecreateOffscreenDC = TRUE;
            Refresh();
            break;

        case WM_ERASEBKGND:
            // handle WM_ERASEBKGND by simply returning non-zero because we did all the drawing in WM_PAINT.
            return 0;

        case WM_DESTROY:
            if(AppBarConfig.uEdge>=0) DoAppBarStuff(ABM_REMOVE);

            /* Save settings to registry */
            SaveSettings();
            KillTimer(hWnd , 1);
            PostQuitMessage(0);

            /* Cleanup notification icon */
            ZeroMemory(&nid, sizeof(nid));
            nid.cbSize = sizeof(nid);
            nid.uFlags = NIF_MESSAGE;
            nid.hWnd = hWnd;
            nid.uCallbackMessage = APPMSG_NOTIFYICON;
            Shell_NotifyIcon(NIM_DELETE, &nid);

            DestroyIcon(notifyIcon);

            DestroyWindow(hOptionsDialog);
            return 0;

        case WM_CREATE:
        {
            HMENU tempMenu;

            /* Get the desktop window */
            hDesktopWindow = GetDesktopWindow();

            /* Set the timer */
            SetTimer (hWnd , 1, TIMER_SPEED , NULL);

            /* Notification icon */
            notifyIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, 0);

            ZeroMemory(&nid, sizeof(nid));
            nid.cbSize = sizeof(nid);
            nid.uFlags = NIF_ICON | NIF_MESSAGE;
            nid.hWnd = hWnd;
            nid.uCallbackMessage = APPMSG_NOTIFYICON;
            nid.hIcon = notifyIcon;
            Shell_NotifyIcon(NIM_ADD, &nid);
            
            tempMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_MAGNIFIER));
            notifyMenu = GetSubMenu(tempMenu, 0);
            RemoveMenu(tempMenu, 0, MF_BYPOSITION);
            DestroyMenu(tempMenu);
            return 0;
        }

        case APPMSG_APPBAR:
        {
            switch (wParam)
            {
                case ABN_STATECHANGE:
                    DoAppBarStuff(ABM_GETSTATE);
                    break;
                case ABN_POSCHANGED:
                    DoAppBarStuff(ABM_SETPOS);
                    break;
                case ABN_FULLSCREENAPP:
                {
                    if(!lParam)
                    {
                        DoAppBarStuff(ABM_GETSTATE);
                        break;
                    }

                    SetWindowPos(hMainWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
                    break;
                }
               case ABN_WINDOWARRANGE:
                    if(lParam)
                        ShowWindow(hMainWnd, SW_HIDE);
                    else
                        ShowWindow(hMainWnd, SW_SHOW);
            }
            return 0;
        }

        case APPMSG_NOTIFYICON:
            HandleNotifyIconMessage(hWnd, wParam, lParam);
            return 0;

        case WM_ACTIVATE:
            if(AppBarConfig.uEdge>=0) DoAppBarStuff(ABM_ACTIVATE);
            break;

        case WM_WINDOWPOSCHANGED:
            if(AppBarConfig.uEdge>=0) DoAppBarStuff(ABM_WINDOWPOSCHANGED);
            Refresh();
            break;

        default:
            break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK OptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_SHOWWINDOW:
            bOptionsDialog = wParam;
            break;
        case WM_INITDIALOG:
        {
            // Add the zoom items...
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("1"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("2"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("3"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("4"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("5"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("6"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("7"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("8"));

            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_SETCURSEL, iZoom - 1, 0);

            if (bFollowMouse)
                SendDlgItemMessage(hDlg,IDC_FOLLOWMOUSECHECK,BM_SETCHECK , wParam ,0);

            if (bFollowFocus)
                SendDlgItemMessage(hDlg,IDC_FOLLOWKEYBOARDCHECK,BM_SETCHECK , wParam ,0);

            if (bFollowCaret)
                SendDlgItemMessage(hDlg,IDC_FOLLOWTEXTEDITINGCHECK,BM_SETCHECK , wParam ,0);

            if (bInvertColors)
                SendDlgItemMessage(hDlg,IDC_INVERTCOLORSCHECK,BM_SETCHECK , wParam ,0);

            if (bStartMinimized)
                SendDlgItemMessage(hDlg,IDC_STARTMINIMIZEDCHECK,BM_SETCHECK , wParam ,0);

            if (bShowMagnifier)
                SendDlgItemMessage(hDlg,IDC_SHOWMAGNIFIERCHECK,BM_SETCHECK , wParam ,0);

            return (INT_PTR)TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            case IDCANCEL:
                ShowWindow(hDlg, SW_HIDE);
                return (INT_PTR)TRUE;

            case IDC_BUTTON_HELP:
                /* Unimplemented */
                MessageBox(hDlg , TEXT("Magnifier help not available yet!") , TEXT("Help") , MB_OK);
                break;
            case IDC_ZOOM:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    HWND hCombo = GetDlgItem(hDlg,IDC_ZOOM);

                    /* Get index of current selection and the text of that selection */
                    iZoom = SendMessage( hCombo, CB_GETCURSEL, (WPARAM) wParam, (LPARAM) lParam ) + 1;

                    /* Update the magnifier UI */
                    Refresh();
                }
                break;
            case IDC_INVERTCOLORSCHECK:
                bInvertColors = IsDlgButtonChecked(hDlg, IDC_INVERTCOLORSCHECK);
                Refresh();
                break;
            case IDC_FOLLOWMOUSECHECK:
                bFollowMouse = IsDlgButtonChecked(hDlg, IDC_FOLLOWMOUSECHECK);
                break;
            case IDC_FOLLOWKEYBOARDCHECK:
                bFollowFocus = IsDlgButtonChecked(hDlg, IDC_FOLLOWKEYBOARDCHECK);
                break;
            case IDC_FOLLOWTEXTEDITINGCHECK:
                bFollowCaret = IsDlgButtonChecked(hDlg, IDC_FOLLOWTEXTEDITINGCHECK);
                break;
            case IDC_STARTMINIMIZEDCHECK:
                bStartMinimized = IsDlgButtonChecked(hDlg, IDC_STARTMINIMIZEDCHECK);
                break;
            case IDC_SHOWMAGNIFIER:
                bShowMagnifier = IsDlgButtonChecked(hDlg, IDC_SHOWMAGNIFIERCHECK);
                if (bShowMagnifier)
                    ShowWindow (hMainWnd , SW_SHOW);
                else
                    ShowWindow (hMainWnd , SW_HIDE);
                break;
        }
    }

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK WarningProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SHOWWARNINGCHECK:
                    bShowWarning = !IsDlgButtonChecked(hDlg, IDC_SHOWWARNINGCHECK);
                    break;
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }

    return (INT_PTR)FALSE;
}
