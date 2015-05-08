/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/scrollbox.cpp
 * PURPOSE:     Functionality surrounding the scroll box window class
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include "scrollbox.h"

/* FUNCTIONS ********************************************************/

void
RegisterWclScrollbox()
{
    WNDCLASSEX wclScroll;
    /* initializing and registering the window class used for the scroll box */
    wclScroll.hInstance     = hProgInstance;
    wclScroll.lpszClassName = _T("Scrollbox");
    wclScroll.lpfnWndProc   = ScrollboxWinProc;
    wclScroll.style         = 0;
    wclScroll.cbSize        = sizeof(WNDCLASSEX);
    wclScroll.hIcon         = NULL;
    wclScroll.hIconSm       = NULL;
    wclScroll.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wclScroll.lpszMenuName  = NULL;
    wclScroll.cbClsExtra    = 0;
    wclScroll.cbWndExtra    = 0;
    wclScroll.hbrBackground = GetSysColorBrush(COLOR_APPWORKSPACE);
    RegisterClassEx (&wclScroll);
}

void
UpdateScrollbox()
{
    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    SCROLLINFO si;
    GetClientRect(hScrollbox, &clientRectScrollbox);
    GetClientRect(hImageArea, &clientRectImageArea);
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_PAGE | SIF_RANGE;
    si.nMax   = clientRectImageArea.right + 6 - 1;
    si.nMin   = 0;
    si.nPage  = clientRectScrollbox.right;
    SetScrollInfo(hScrollbox, SB_HORZ, &si, TRUE);
    GetClientRect(hScrollbox, &clientRectScrollbox);
    si.nMax   = clientRectImageArea.bottom + 6 - 1;
    si.nPage  = clientRectScrollbox.bottom;
    SetScrollInfo(hScrollbox, SB_VERT, &si, TRUE);
    MoveWindow(hScrlClient,
               -GetScrollPos(hScrollbox, SB_HORZ), -GetScrollPos(hScrollbox, SB_VERT),
               max(clientRectImageArea.right + 6, clientRectScrollbox.right),
               max(clientRectImageArea.bottom + 6, clientRectScrollbox.bottom), TRUE);
}

LRESULT CALLBACK
ScrollboxWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SIZE:
            if (hwnd == hScrollbox)
            {
                UpdateScrollbox();
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

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
