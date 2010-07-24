/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/winproc.c
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include <commctrl.h>
//#include <htmlhelp.h>
#include <stdio.h>
#include <tchar.h>
#include "definitions.h"
#include "globalvar.h"
#include "dialogs.h"
#include "dib.h"
#include "drawing.h"
#include "history.h"
#include "mouse.h"
#include "registry.h"

/* FUNCTIONS ********************************************************/

void
selectTool(int tool)
{
    ShowWindow(hSelection, SW_HIDE);
    activeTool = tool;
    pointSP = 0;                // resets the point-buffer of the polygon and bezier functions
    SendMessage(hToolSettings, WM_PAINT, 0, 0);
    ShowWindow(hTrackbarZoom, (tool == 6) ? SW_SHOW : SW_HIDE);
}

void
updateCanvasAndScrollbars()
{
    ShowWindow(hSelection, SW_HIDE);
    MoveWindow(hImageArea, 3, 3, imgXRes * zoom / 1000, imgYRes * zoom / 1000, FALSE);
    InvalidateRect(hScrollbox, NULL, TRUE);
    InvalidateRect(hImageArea, NULL, FALSE);

    SetScrollPos(hScrollbox, SB_HORZ, 0, TRUE);
    SetScrollPos(hScrollbox, SB_VERT, 0, TRUE);
}

void
zoomTo(int newZoom, int mouseX, int mouseY)
{
    int tbPos = 0;
    int tempZoom = newZoom;

    long clientRectScrollbox[4];
    long clientRectImageArea[4];
    int x, y, w, h;
    GetClientRect(hScrollbox, (LPRECT) &clientRectScrollbox);
    GetClientRect(hImageArea, (LPRECT) &clientRectImageArea);
    w = clientRectImageArea[2] * clientRectScrollbox[2] / (clientRectImageArea[2] * newZoom / zoom);
    h = clientRectImageArea[3] * clientRectScrollbox[3] / (clientRectImageArea[3] * newZoom / zoom);
    x = max(0, min(clientRectImageArea[2] - w, mouseX - w / 2)) * newZoom / zoom;
    y = max(0, min(clientRectImageArea[3] - h, mouseY - h / 2)) * newZoom / zoom;

    zoom = newZoom;

    ShowWindow(hSelection, SW_HIDE);
    MoveWindow(hImageArea, 3, 3, imgXRes * zoom / 1000, imgYRes * zoom / 1000, FALSE);
    InvalidateRect(hScrollbox, NULL, TRUE);
    InvalidateRect(hImageArea, NULL, FALSE);

    SendMessage(hScrollbox, WM_HSCROLL, SB_THUMBPOSITION | (x << 16), 0);
    SendMessage(hScrollbox, WM_VSCROLL, SB_THUMBPOSITION | (y << 16), 0);

    while (tempZoom > 125)
    {
        tbPos++;
        tempZoom = tempZoom >> 1;
    }
    SendMessage(hTrackbarZoom, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) tbPos);
}

void
drawZoomFrame(int mouseX, int mouseY)
{
    HDC hdc;
    HPEN oldPen;
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    int rop;

    long clientRectScrollbox[4];
    long clientRectImageArea[4];
    int x, y, w, h;
    GetClientRect(hScrollbox, (LPRECT) &clientRectScrollbox);
    GetClientRect(hImageArea, (LPRECT) &clientRectImageArea);
    w = clientRectImageArea[2] * clientRectScrollbox[2] / (clientRectImageArea[2] * 2);
    h = clientRectImageArea[3] * clientRectScrollbox[3] / (clientRectImageArea[3] * 2);
    x = max(0, min(clientRectImageArea[2] - w, mouseX - w / 2));
    y = max(0, min(clientRectImageArea[3] - h, mouseY - h / 2));

    hdc = GetDC(hImageArea);
    oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 0, 0));
    logbrush.lbStyle = BS_HOLLOW;
    oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    rop = SetROP2(hdc, R2_NOT);
    Rectangle(hdc, x, y, x + w, y + h);
    SetROP2(hdc, rop);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    ReleaseDC(hImageArea, hdc);
}

void
alignChildrenToMainWindow()
{
    int x, y, w, h;
    RECT clientRect;
    GetClientRect(hMainWnd, &clientRect);

    if (IsWindowVisible(hToolBoxContainer))
    {
        x = 56;
        w = clientRect.right - 56;
    }
    else
    {
        x = 0;
        w = clientRect.right;
    }
    if (IsWindowVisible(hPalWin))
    {
        y = 49;
        h = clientRect.bottom - 49;
    }
    else
    {
        y = 3;
        h = clientRect.bottom - 3;
    }

    MoveWindow(hScrollbox, x, y, w, IsWindowVisible(hStatusBar) ? h - 23 : h, TRUE);
    MoveWindow(hPalWin, x, 9, 255, 32, TRUE);
}

BOOL drawing;

LRESULT CALLBACK
WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)            /* handle the messages */
    {
        case WM_DESTROY:
            PostQuitMessage(0); /* send a WM_QUIT to the message queue */
            break;

        case WM_CLOSE:
            if (hwnd == hwndMiniature)
            {
                ShowWindow(hwndMiniature, SW_HIDE);
                showMiniature = FALSE;
                break;
            }
            if (!imageSaved)
            {
                TCHAR programname[20];
                TCHAR saveprompttext[100];
                TCHAR temptext[500];
                LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
                LoadString(hProgInstance, IDS_SAVEPROMPTTEXT, saveprompttext, SIZEOF(saveprompttext));
                _stprintf(temptext, saveprompttext, filename);
                switch (MessageBox(hwnd, temptext, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
                {
                    case IDNO:
                        DestroyWindow(hwnd);
                        break;
                    case IDYES:
                        SendMessage(hwnd, WM_COMMAND, IDM_FILESAVEAS, 0);
                        DestroyWindow(hwnd);
                        break;
                }
            }
            else
            {
                DestroyWindow(hwnd);
            }
            break;

        case WM_INITMENUPOPUP:
            switch (lParam)
            {
                case 0:
                    if (isAFile)
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERPLANE,
                                       MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERCENTERED,
                                       MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERSTRETCHED,
                                       MF_ENABLED | MF_BYCOMMAND);
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERPLANE,
                                       MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERCENTERED,
                                       MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERSTRETCHED,
                                       MF_GRAYED | MF_BYCOMMAND);
                    }
                    break;
                case 1:
                    EnableMenuItem(GetMenu(hMainWnd), IDM_EDITUNDO,
                                   (undoSteps > 0) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND));
                    EnableMenuItem(GetMenu(hMainWnd), IDM_EDITREDO,
                                   (redoSteps > 0) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND));
                    if (IsWindowVisible(hSelection))
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCUT, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPY, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITDELETESELECTION, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITINVERTSELECTION, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPYTO, MF_ENABLED | MF_BYCOMMAND);
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCUT, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPY, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITDELETESELECTION, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITINVERTSELECTION, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPYTO, MF_GRAYED | MF_BYCOMMAND);
                    }
                    OpenClipboard(hMainWnd);
                    if (GetClipboardData(CF_BITMAP) != NULL)
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITPASTE, MF_ENABLED | MF_BYCOMMAND);
                    else
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITPASTE, MF_GRAYED | MF_BYCOMMAND);
                    CloseClipboard();
                    break;
                case 3:
                    if (IsWindowVisible(hSelection))
                        EnableMenuItem(GetMenu(hMainWnd), IDM_IMAGECROP, MF_ENABLED | MF_BYCOMMAND);
                    else
                        EnableMenuItem(GetMenu(hMainWnd), IDM_IMAGECROP, MF_GRAYED | MF_BYCOMMAND);
                    CheckMenuItem(GetMenu(hMainWnd), IDM_IMAGEDRAWOPAQUE, (transpBg == 0) ?
                                  (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
                    break;
            }
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWTOOLBOX,
                          IsWindowVisible(hToolBoxContainer) ?
                              (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWCOLORPALETTE,
                          IsWindowVisible(hPalWin) ?
                              (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSTATUSBAR,
                          IsWindowVisible(hStatusBar) ?
                              (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));

            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSHOWGRID,
                          showGrid ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSHOWMINIATURE,
                          showMiniature ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));

            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM125,
                          (zoom == 125) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM25,
                          (zoom == 250) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM50,
                          (zoom == 500) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM100,
                          (zoom == 1000) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM200,
                          (zoom == 2000) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM400,
                          (zoom == 4000) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));
            CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM800,
                          (zoom == 8000) ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));

            break;

        case WM_SIZE:
            if (hwnd == hMainWnd)
            {
                int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
                SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
                SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
                alignChildrenToMainWindow();
            }
            if (hwnd == hImageArea)
            {
                MoveWindow(hSizeboxLeftTop,
                           0,
                           0, 3, 3, TRUE);
                MoveWindow(hSizeboxCenterTop,
                           imgXRes * zoom / 2000 + 3 * 3 / 4,
                           0, 3, 3, TRUE);
                MoveWindow(hSizeboxRightTop,
                           imgXRes * zoom / 1000 + 3,
                           0, 3, 3, TRUE);
                MoveWindow(hSizeboxLeftCenter,
                           0,
                           imgYRes * zoom / 2000 + 3 * 3 / 4, 3, 3, TRUE);
                MoveWindow(hSizeboxRightCenter,
                           imgXRes * zoom / 1000 + 3,
                           imgYRes * zoom / 2000 + 3 * 3 / 4, 3, 3, TRUE);
                MoveWindow(hSizeboxLeftBottom,
                           0,
                           imgYRes * zoom / 1000 + 3, 3, 3, TRUE);
                MoveWindow(hSizeboxCenterBottom,
                           imgXRes * zoom / 2000 + 3 * 3 / 4,
                           imgYRes * zoom / 1000 + 3, 3, 3, TRUE);
                MoveWindow(hSizeboxRightBottom,
                           imgXRes * zoom / 1000 + 3,
                           imgYRes * zoom / 1000 + 3, 3, 3, TRUE);
            }
            if ((hwnd == hImageArea) || (hwnd == hScrollbox))
            {
                long clientRectScrollbox[4];
                long clientRectImageArea[4];
                SCROLLINFO si;
                GetClientRect(hScrollbox, (LPRECT) &clientRectScrollbox);
                GetClientRect(hImageArea, (LPRECT) &clientRectImageArea);
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask  = SIF_PAGE | SIF_RANGE;
                si.nMax   = clientRectImageArea[2] + 6 - 1;
                si.nMin   = 0;
                si.nPage  = clientRectScrollbox[2];
                SetScrollInfo(hScrollbox, SB_HORZ, &si, TRUE);
                GetClientRect(hScrollbox, (LPRECT) clientRectScrollbox);
                si.nMax   = clientRectImageArea[3] + 6 - 1;
                si.nPage  = clientRectScrollbox[3];
                SetScrollInfo(hScrollbox, SB_VERT, &si, TRUE);
                MoveWindow(hScrlClient,
                           -GetScrollPos(hScrollbox, SB_HORZ), -GetScrollPos(hScrollbox, SB_VERT),
                           max(clientRectImageArea[2] + 6, clientRectScrollbox[2]),
                           max(clientRectImageArea[3] + 6, clientRectScrollbox[3]), TRUE);
            }
            break;

        case WM_HSCROLL:
            if (hwnd == hScrollbox)
            {
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_ALL;
                GetScrollInfo(hScrollbox, SB_HORZ, &si);
                switch (LOWORD(wParam))
                {
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION:
                        si.nPos = HIWORD(wParam);
                        break;
                    case SB_LINELEFT:
                        si.nPos -= 5;
                        break;
                    case SB_LINERIGHT:
                        si.nPos += 5;
                        break;
                    case SB_PAGELEFT:
                        si.nPos -= si.nPage;
                        break;
                    case SB_PAGERIGHT:
                        si.nPos += si.nPage;
                        break;
                }
                SetScrollInfo(hScrollbox, SB_HORZ, &si, TRUE);
                MoveWindow(hScrlClient, -GetScrollPos(hScrollbox, SB_HORZ),
                           -GetScrollPos(hScrollbox, SB_VERT), imgXRes * zoom / 1000 + 6,
                           imgYRes * zoom / 1000 + 6, TRUE);
            }
            break;

        case WM_VSCROLL:
            if (hwnd == hScrollbox)
            {
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_ALL;
                GetScrollInfo(hScrollbox, SB_VERT, &si);
                switch (LOWORD(wParam))
                {
                    case SB_THUMBTRACK:
                    case SB_THUMBPOSITION:
                        si.nPos = HIWORD(wParam);
                        break;
                    case SB_LINEUP:
                        si.nPos -= 5;
                        break;
                    case SB_LINEDOWN:
                        si.nPos += 5;
                        break;
                    case SB_PAGEUP:
                        si.nPos -= si.nPage;
                        break;
                    case SB_PAGEDOWN:
                        si.nPos += si.nPage;
                        break;
                }
                SetScrollInfo(hScrollbox, SB_VERT, &si, TRUE);
                MoveWindow(hScrlClient, -GetScrollPos(hScrollbox, SB_HORZ),
                           -GetScrollPos(hScrollbox, SB_VERT), imgXRes * zoom / 1000 + 6,
                           imgYRes * zoom / 1000 + 6, TRUE);
            }
            break;

        case WM_GETMINMAXINFO:
            if (hwnd == hMainWnd)
            {
                MINMAXINFO *mm = (LPMINMAXINFO) lParam;
                (*mm).ptMinTrackSize.x = 330;
                (*mm).ptMinTrackSize.y = 430;
            }
            break;

        case WM_PAINT:
            DefWindowProc(hwnd, message, wParam, lParam);
            if (hwnd == hImageArea)
            {
                HDC hdc = GetDC(hImageArea);
                StretchBlt(hdc, 0, 0, imgXRes * zoom / 1000, imgYRes * zoom / 1000, hDrawingDC, 0, 0, imgXRes,
                           imgYRes, SRCCOPY);
                if (showGrid && (zoom >= 4000))
                {
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, 0x00a0a0a0));
                    int counter;
                    for(counter = 0; counter <= imgYRes; counter++)
                    {
                        MoveToEx(hdc, 0, counter * zoom / 1000, NULL);
                        LineTo(hdc, imgXRes * zoom / 1000, counter * zoom / 1000);
                    }
                    for(counter = 0; counter <= imgXRes; counter++)
                    {
                        MoveToEx(hdc, counter * zoom / 1000, 0, NULL);
                        LineTo(hdc, counter * zoom / 1000, imgYRes * zoom / 1000);
                    }
                    DeleteObject(SelectObject(hdc, oldPen));
                }
                ReleaseDC(hImageArea, hdc);
                SendMessage(hSelection, WM_PAINT, 0, 0);
                SendMessage(hwndMiniature, WM_PAINT, 0, 0);
            }
            else if (hwnd == hwndMiniature)
            {
                long mclient[4];
                HDC hdc;
                GetClientRect(hwndMiniature, (LPRECT) &mclient);
                hdc = GetDC(hwndMiniature);
                BitBlt(hdc, -min(imgXRes * GetScrollPos(hScrollbox, SB_HORZ) / 10000, imgXRes - mclient[2]),
                       -min(imgYRes * GetScrollPos(hScrollbox, SB_VERT) / 10000, imgYRes - mclient[3]),
                       imgXRes, imgYRes, hDrawingDC, 0, 0, SRCCOPY);
                ReleaseDC(hwndMiniature, hdc);
            }
            break;

            // mouse events used for drawing   

        case WM_SETCURSOR:
            if (hwnd == hImageArea)
            {
                switch (activeTool)
                {
                    case TOOL_FILL:
                        SetCursor(hCurFill);
                        break;
                    case TOOL_COLOR:
                        SetCursor(hCurColor);
                        break;
                    case TOOL_ZOOM:
                        SetCursor(hCurZoom);
                        break;
                    case TOOL_PEN:
                        SetCursor(hCurPen);
                        break;
                    case TOOL_AIRBRUSH:
                        SetCursor(hCurAirbrush);
                        break;
                    default:
                        SetCursor(LoadCursor(NULL, IDC_CROSS));
                }
            }
            else
                DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            if (hwnd == hImageArea)
            {
                if ((!drawing) || (activeTool == TOOL_COLOR))
                {
                    SetCapture(hImageArea);
                    drawing = TRUE;
                    startPaintingL(hDrawingDC, LOWORD(lParam) * 1000 / zoom, HIWORD(lParam) * 1000 / zoom,
                                   fgColor, bgColor);
                }
                else
                {
                    SendMessage(hwnd, WM_LBUTTONUP, wParam, lParam);
                    undo();
                }
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if ((activeTool == TOOL_ZOOM) && (zoom < 8000))
                    zoomTo(zoom * 2, (short)LOWORD(lParam), (short)HIWORD(lParam));
            }
            break;

        case WM_RBUTTONDOWN:
            if (hwnd == hImageArea)
            {
                if ((!drawing) || (activeTool == TOOL_COLOR))
                {
                    SetCapture(hImageArea);
                    drawing = TRUE;
                    startPaintingR(hDrawingDC, LOWORD(lParam) * 1000 / zoom, HIWORD(lParam) * 1000 / zoom,
                                   fgColor, bgColor);
                }
                else
                {
                    SendMessage(hwnd, WM_RBUTTONUP, wParam, lParam);
                    undo();
                }
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if ((activeTool == TOOL_ZOOM) && (zoom > 125))
                    zoomTo(zoom / 2, (short)LOWORD(lParam), (short)HIWORD(lParam));
            }
            break;

        case WM_LBUTTONUP:
            if ((hwnd == hImageArea) && drawing)
            {
                ReleaseCapture();
                drawing = FALSE;
                endPaintingL(hDrawingDC, LOWORD(lParam) * 1000 / zoom, HIWORD(lParam) * 1000 / zoom, fgColor,
                             bgColor);
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if (activeTool == TOOL_COLOR)
                {
                    int tempColor =
                        GetPixel(hDrawingDC, LOWORD(lParam) * 1000 / zoom, HIWORD(lParam) * 1000 / zoom);
                    if (tempColor != CLR_INVALID)
                        fgColor = tempColor;
                    SendMessage(hPalWin, WM_PAINT, 0, 0);
                }
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
            }
            break;

        case WM_RBUTTONUP:
            if ((hwnd == hImageArea) && drawing)
            {
                ReleaseCapture();
                drawing = FALSE;
                endPaintingR(hDrawingDC, LOWORD(lParam) * 1000 / zoom, HIWORD(lParam) * 1000 / zoom, fgColor,
                             bgColor);
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if (activeTool == TOOL_COLOR)
                {
                    int tempColor =
                        GetPixel(hDrawingDC, LOWORD(lParam) * 1000 / zoom, HIWORD(lParam) * 1000 / zoom);
                    if (tempColor != CLR_INVALID)
                        bgColor = tempColor;
                    SendMessage(hPalWin, WM_PAINT, 0, 0);
                }
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
            }
            break;

        case WM_MOUSEMOVE:
            if (hwnd == hImageArea)
            {
                short xNow = (short)LOWORD(lParam) * 1000 / zoom;
                short yNow = (short)HIWORD(lParam) * 1000 / zoom;
                if ((!drawing) || (activeTool <= TOOL_AIRBRUSH))
                {
                    TRACKMOUSEEVENT tme;

                    if (activeTool == TOOL_ZOOM)
                    {
                        SendMessage(hImageArea, WM_PAINT, 0, 0);
                        drawZoomFrame((short)LOWORD(lParam), (short)HIWORD(lParam));
                    }

                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hImageArea;
                    tme.dwHoverTime = 0;
                    TrackMouseEvent(&tme);

                    if (!drawing)
                    {
                        TCHAR coordStr[100];
                        _stprintf(coordStr, _T("%d, %d"), xNow, yNow);
                        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) coordStr);
                    }
                }
                if (drawing)
                {
                    /* values displayed in statusbar */
                    short xRel = xNow - startX;
                    short yRel = yNow - startY;
                    /* freesel, rectsel and text tools always show numbers limited to fit into image area */
                    if ((activeTool == TOOL_FREESEL) || (activeTool == TOOL_RECTSEL) || (activeTool == TOOL_TEXT))
                    {
                        if (xRel < 0)
                            xRel = (xNow < 0) ? -startX : xRel;
                        else if (xNow > imgXRes)
                            xRel = imgXRes-startX;
                        if (yRel < 0)
                            yRel = (yNow < 0) ? -startY : yRel;
                        else if (yNow > imgYRes)
                             yRel = imgYRes-startY;
                    }
                    /* rectsel and shape tools always show non-negative numbers when drawing */
                    if ((activeTool == TOOL_RECTSEL) || (activeTool == TOOL_SHAPE))
                    {
                        if (xRel < 0)
                            xRel = -xRel;
                        if (yRel < 0)
                            yRel =  -yRel;
                    }
                    /* while drawing, update cursor coordinates only for tools 3, 7, 8, 9, 14 */
                    switch(activeTool)
                    {
                        case TOOL_RUBBER:
                        case TOOL_PEN:
                        case TOOL_BRUSH:
                        case TOOL_AIRBRUSH:
                        case TOOL_SHAPE:
                        {
                            TCHAR coordStr[100];
                            _stprintf(coordStr, _T("%d, %d"), xNow, yNow);
                            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) coordStr);
                            break;
                        }
                    }
                    if ((wParam & MK_LBUTTON) != 0)
                    {
                        whilePaintingL(hDrawingDC, xNow, yNow, fgColor, bgColor);
                        SendMessage(hImageArea, WM_PAINT, 0, 0);
                        if ((activeTool >= TOOL_TEXT) || (activeTool == TOOL_RECTSEL) || (activeTool == TOOL_FREESEL))
                        {
                            TCHAR sizeStr[100];
                            _stprintf(sizeStr, _T("%d x %d"), xRel, yRel);
                            SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
                        }
                    }
                    if ((wParam & MK_RBUTTON) != 0)
                    {
                        whilePaintingR(hDrawingDC, xNow, yNow, fgColor, bgColor);
                        SendMessage(hImageArea, WM_PAINT, 0, 0);
                        if (activeTool >= TOOL_TEXT)
                        {
                            TCHAR sizeStr[100];
                            _stprintf(sizeStr, _T("%d x %d"), xRel, yRel);
                            SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
                        }
                    }
                }
            }
            break;

        case WM_MOUSELEAVE:
            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) _T(""));
            if (activeTool == TOOL_ZOOM)
                SendMessage(hImageArea, WM_PAINT, 0, 0);
            break;

        // menu and button events

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_HELPINFO:
                {
                    HICON paintIcon = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
                    TCHAR infotitle[100];
                    TCHAR infotext[200];
                    LoadString(hProgInstance, IDS_INFOTITLE, infotitle, SIZEOF(infotitle));
                    LoadString(hProgInstance, IDS_INFOTEXT, infotext, SIZEOF(infotext));
                    ShellAbout(hMainWnd, infotitle, infotext, paintIcon);
                    DeleteObject(paintIcon);
                    break;
                }
                case IDM_HELPHELPTOPICS:
                    //HtmlHelp(hMainWnd, "help\\Paint.chm", 0, 0);
                    break;
                case IDM_FILEEXIT:
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                    break;
                case IDM_FILENEW:
                    Rectangle(hDrawingDC, 0 - 1, 0 - 1, imgXRes + 1, imgYRes + 1);
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_FILEOPEN:
                    if (GetOpenFileName(&ofn) != 0)
                    {
                        HBITMAP bmNew = NULL;
                        LoadDIBFromFile(&bmNew, ofn.lpstrFile, &fileTime, &fileSize, &fileHPPM, &fileVPPM);
                        if (bmNew != NULL)
                        {
                            TCHAR tempstr[1000];
                            TCHAR resstr[100];
                            insertReversible(bmNew);
                            updateCanvasAndScrollbars();
                            CopyMemory(filename, ofn.lpstrFileTitle, sizeof(filename));
                            CopyMemory(filepathname, ofn.lpstrFileTitle, sizeof(filepathname));
                            LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
                            _stprintf(tempstr, resstr, filename);
                            SetWindowText(hMainWnd, tempstr);
                            clearHistory();
                            isAFile = TRUE;
                        }
                    }
                    break;
                case IDM_FILESAVE:
                    if (isAFile)
                    {
                        SaveDIBToFile(hBms[currInd], filepathname, hDrawingDC, &fileTime, &fileSize, fileHPPM,
                                      fileVPPM);
                        imageSaved = TRUE;
                    }
                    else
                        SendMessage(hwnd, WM_COMMAND, IDM_FILESAVEAS, 0);
                    break;
                case IDM_FILESAVEAS:
                    if (GetSaveFileName(&sfn) != 0)
                    {
                        TCHAR tempstr[1000];
                        TCHAR resstr[100];
                        SaveDIBToFile(hBms[currInd], sfn.lpstrFile, hDrawingDC, &fileTime, &fileSize,
                                      fileHPPM, fileVPPM);
                        CopyMemory(filename, sfn.lpstrFileTitle, sizeof(filename));
                        CopyMemory(filepathname, sfn.lpstrFile, sizeof(filepathname));
                        LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
                        _stprintf(tempstr, resstr, filename);
                        SetWindowText(hMainWnd, tempstr);
                        isAFile = TRUE;
                        imageSaved = TRUE;
                    }
                    break;
                case IDM_FILEASWALLPAPERPLANE:
                    SetWallpaper(filepathname, 1, 1);
                    break;
                case IDM_FILEASWALLPAPERCENTERED:
                    SetWallpaper(filepathname, 1, 0);
                    break;
                case IDM_FILEASWALLPAPERSTRETCHED:
                    SetWallpaper(filepathname, 2, 0);
                    break;
                case IDM_EDITUNDO:
                    undo();
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_EDITREDO:
                    redo();
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_EDITCOPY:
                    OpenClipboard(hMainWnd);
                    EmptyClipboard();
                    SetClipboardData(CF_BITMAP, CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
                    CloseClipboard();
                    break;
                case IDM_EDITPASTE:
                    OpenClipboard(hMainWnd);
                    if (GetClipboardData(CF_BITMAP) != NULL)
                    {
                        DeleteObject(SelectObject(hSelDC, hSelBm = CopyImage(GetClipboardData(CF_BITMAP),
                                                                             IMAGE_BITMAP, 0, 0,
                                                                             LR_COPYRETURNORG)));
                        newReversible();
                        rectSel_src[0] = rectSel_src[1] = rectSel_src[2] = rectSel_src[3] = 0;
                        rectSel_dest[0] = rectSel_dest[1] = 0;
                        rectSel_dest[2] = GetDIBWidth(hSelBm);
                        rectSel_dest[3] = GetDIBHeight(hSelBm);
                        BitBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3],
                               hSelDC, 0, 0, SRCCOPY);
                        placeSelWin();
                        ShowWindow(hSelection, SW_SHOW);
                    }
                    CloseClipboard();
                    break;
                case IDM_EDITDELETESELECTION:
                {
                    /* remove selection window and already painted content using undo(),
                    paint Rect for rectangular selections and nothing for freeform selections */
                    undo();
                    if (activeTool == TOOL_RECTSEL)
                    {
                        newReversible();
                        Rect(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2] + rectSel_dest[0],
                             rectSel_dest[3] + rectSel_dest[1], bgColor, bgColor, 0, TRUE);
                    }
                    break;
                }
                case IDM_EDITSELECTALL:
                    if (activeTool == TOOL_RECTSEL)
                    {
                        startPaintingL(hDrawingDC, 0, 0, fgColor, bgColor);
                        whilePaintingL(hDrawingDC, imgXRes, imgYRes, fgColor, bgColor);
                        endPaintingL(hDrawingDC, imgXRes, imgYRes, fgColor, bgColor);
                    }
                    break;
                case IDM_EDITCOPYTO:
                    if (GetSaveFileName(&ofn) != 0)
                        SaveDIBToFile(hSelBm, ofn.lpstrFile, hDrawingDC, NULL, NULL, fileHPPM, fileVPPM);
                    break;
                case IDM_COLORSEDITPALETTE:
                    if (ChooseColor(&choosecolor))
                    {
                        fgColor = choosecolor.rgbResult;
                        SendMessage(hPalWin, WM_PAINT, 0, 0);
                    }
                    break;
                case IDM_IMAGEINVERTCOLORS:
                {
                    RECT tempRect;
                    newReversible();
                    SetRect(&tempRect, 0, 0, imgXRes, imgYRes);
                    InvertRect(hDrawingDC, &tempRect);
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                }
                case IDM_IMAGEDELETEIMAGE:
                    newReversible();
                    Rect(hDrawingDC, 0, 0, imgXRes, imgYRes, bgColor, bgColor, 0, TRUE);
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_IMAGEROTATEMIRROR:
                    switch (mirrorRotateDlg())
                    {
                        case 1:
                            newReversible();
                            StretchBlt(hDrawingDC, imgXRes - 1, 0, -imgXRes, imgYRes, hDrawingDC, 0, 0,
                                       imgXRes, imgYRes, SRCCOPY);
                            SendMessage(hImageArea, WM_PAINT, 0, 0);
                            break;
                        case 2:
                            newReversible();
                            StretchBlt(hDrawingDC, 0, imgYRes - 1, imgXRes, -imgYRes, hDrawingDC, 0, 0,
                                       imgXRes, imgYRes, SRCCOPY);
                            SendMessage(hImageArea, WM_PAINT, 0, 0);
                            break;
                        case 4:
                            newReversible();
                            StretchBlt(hDrawingDC, imgXRes - 1, imgYRes - 1, -imgXRes, -imgYRes, hDrawingDC,
                                       0, 0, imgXRes, imgYRes, SRCCOPY);
                            SendMessage(hImageArea, WM_PAINT, 0, 0);
                            break;
                    }
                    break;
                case IDM_IMAGEATTRIBUTES:
                {
                    int retVal = attributesDlg();
                    if ((LOWORD(retVal) != 0) && (HIWORD(retVal) != 0))
                    {
                        cropReversible(LOWORD(retVal), HIWORD(retVal), 0, 0);
                        updateCanvasAndScrollbars();
                    }
                    break;
                }
                case IDM_IMAGECHANGESIZE:
                {
                    int retVal = changeSizeDlg();
                    if ((LOWORD(retVal) != 0) && (HIWORD(retVal) != 0))
                    {
                        insertReversible(CopyImage(hBms[currInd], IMAGE_BITMAP,
                                                   imgXRes * LOWORD(retVal) / 100,
                                                   imgYRes * HIWORD(retVal) / 100, 0));
                        updateCanvasAndScrollbars();
                    }
                    break;
                }
                case IDM_IMAGEDRAWOPAQUE:
                    transpBg = 1 - transpBg;
                    SendMessage(hToolSettings, WM_PAINT, 0, 0);
                    break;
                case IDM_IMAGECROP:
                    insertReversible(CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
                    updateCanvasAndScrollbars();
                    break;

                case IDM_VIEWTOOLBOX:
                    ShowWindow(hToolBoxContainer, IsWindowVisible(hToolBoxContainer) ? SW_HIDE : SW_SHOW);
                    alignChildrenToMainWindow();
                    break;
                case IDM_VIEWCOLORPALETTE:
                    ShowWindow(hPalWin, IsWindowVisible(hPalWin) ? SW_HIDE : SW_SHOW);
                    alignChildrenToMainWindow();
                    break;
                case IDM_VIEWSTATUSBAR:
                    ShowWindow(hStatusBar, IsWindowVisible(hStatusBar) ? SW_HIDE : SW_SHOW);
                    alignChildrenToMainWindow();
                    break;

                case IDM_VIEWSHOWGRID:
                    showGrid = !showGrid;
                    break;
                case IDM_VIEWSHOWMINIATURE:
                    showMiniature = !showMiniature;
                    ShowWindow(hwndMiniature, showMiniature ? SW_SHOW : SW_HIDE);
                    break;

                case IDM_VIEWZOOM125:
                    zoomTo(125, 0, 0);
                    break;
                case IDM_VIEWZOOM25:
                    zoomTo(250, 0, 0);
                    break;
                case IDM_VIEWZOOM50:
                    zoomTo(500, 0, 0);
                    break;
                case IDM_VIEWZOOM100:
                    zoomTo(1000, 0, 0);
                    break;
                case IDM_VIEWZOOM200:
                    zoomTo(2000, 0, 0);
                    break;
                case IDM_VIEWZOOM400:
                    zoomTo(4000, 0, 0);
                    break;
                case IDM_VIEWZOOM800:
                    zoomTo(8000, 0, 0);
                    break;
                case ID_FREESEL:
                    selectTool(1);
                    break;
                case ID_RECTSEL:
                    selectTool(2);
                    break;
                case ID_RUBBER:
                    selectTool(3);
                    break;
                case ID_FILL:
                    selectTool(4);
                    break;
                case ID_COLOR:
                    selectTool(5);
                    break;
                case ID_ZOOM:
                    selectTool(6);
                    break;
                case ID_PEN:
                    selectTool(7);
                    break;
                case ID_BRUSH:
                    selectTool(8);
                    break;
                case ID_AIRBRUSH:
                    selectTool(9);
                    break;
                case ID_TEXT:
                    selectTool(10);
                    break;
                case ID_LINE:
                    selectTool(11);
                    break;
                case ID_BEZIER:
                    selectTool(12);
                    break;
                case ID_RECT:
                    selectTool(13);
                    break;
                case ID_SHAPE:
                    selectTool(14);
                    break;
                case ID_ELLIPSE:
                    selectTool(15);
                    break;
                case ID_RRECT:
                    selectTool(16);
                    break;
            }
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
