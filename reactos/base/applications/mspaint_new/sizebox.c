/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        sizebox.c
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include "sizebox.h"

/* FUNCTIONS ********************************************************/

BOOL resizing = FALSE;
short xOrig;
short yOrig;

void
RegisterWclSizebox()
{
    WNDCLASSEX wclSizebox;
    /* initializing and registering the window class for the size boxes */
    wclSizebox.hInstance       = hProgInstance;
    wclSizebox.lpszClassName   = _T("Sizebox");
    wclSizebox.lpfnWndProc     = SizeboxWinProc;
    wclSizebox.style           = CS_DBLCLKS;
    wclSizebox.cbSize          = sizeof(WNDCLASSEX);
    wclSizebox.hIcon           = NULL;
    wclSizebox.hIconSm         = NULL;
    wclSizebox.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wclSizebox.lpszMenuName    = NULL;
    wclSizebox.cbClsExtra      = 0;
    wclSizebox.cbWndExtra      = 0;
    wclSizebox.hbrBackground   = GetSysColorBrush(COLOR_HIGHLIGHT);
    RegisterClassEx (&wclSizebox);
}

LRESULT CALLBACK
SizeboxWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SETCURSOR:
            if ((hwnd == hSizeboxLeftTop) || (hwnd == hSizeboxRightBottom))
                SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            if ((hwnd == hSizeboxLeftBottom) || (hwnd == hSizeboxRightTop))
                SetCursor(LoadCursor(NULL, IDC_SIZENESW));
            if ((hwnd == hSizeboxLeftCenter) || (hwnd == hSizeboxRightCenter))
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            if ((hwnd == hSizeboxCenterTop) || (hwnd == hSizeboxCenterBottom))
                SetCursor(LoadCursor(NULL, IDC_SIZENS));
            break;
        case WM_LBUTTONDOWN:
            resizing = TRUE;
            xOrig = GET_X_LPARAM(lParam);
            yOrig = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            break;
        case WM_MOUSEMOVE:
            if (resizing)
            {
                TCHAR sizeStr[100];
                short xRel;
                short yRel;
                xRel = (GET_X_LPARAM(lParam) - xOrig) * 1000 / zoom;
                yRel = (GET_Y_LPARAM(lParam) - yOrig) * 1000 / zoom;
                if (hwnd == hSizeboxLeftTop)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes - xRel, imgYRes - yRel);
                if (hwnd == hSizeboxCenterTop)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes, imgYRes - yRel);
                if (hwnd == hSizeboxRightTop)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes + xRel, imgYRes - yRel);
                if (hwnd == hSizeboxLeftCenter)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes - xRel, imgYRes);
                if (hwnd == hSizeboxRightCenter)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes + xRel, imgYRes);
                if (hwnd == hSizeboxLeftBottom)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes - xRel, imgYRes + yRel);
                if (hwnd == hSizeboxCenterBottom)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes, imgYRes + yRel);
                if (hwnd == hSizeboxRightBottom)
                    _stprintf(sizeStr, _T("%d x %d"), imgXRes + xRel, imgYRes + yRel);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
            }
            break;
        case WM_LBUTTONUP:
            if (resizing)
            {
                short xRel;
                short yRel;
                ReleaseCapture();
                resizing = FALSE;
                xRel = (GET_X_LPARAM(lParam) - xOrig) * 1000 / zoom;
                yRel = (GET_Y_LPARAM(lParam) - yOrig) * 1000 / zoom;
                if (hwnd == hSizeboxLeftTop)
                    cropReversible(imgXRes - xRel, imgYRes - yRel, xRel, yRel);
                if (hwnd == hSizeboxCenterTop)
                    cropReversible(imgXRes, imgYRes - yRel, 0, yRel);
                if (hwnd == hSizeboxRightTop)
                    cropReversible(imgXRes + xRel, imgYRes - yRel, 0, yRel);
                if (hwnd == hSizeboxLeftCenter)
                    cropReversible(imgXRes - xRel, imgYRes, xRel, 0);
                if (hwnd == hSizeboxRightCenter)
                    cropReversible(imgXRes + xRel, imgYRes, 0, 0);
                if (hwnd == hSizeboxLeftBottom)
                    cropReversible(imgXRes - xRel, imgYRes + yRel, xRel, 0);
                if (hwnd == hSizeboxCenterBottom)
                    cropReversible(imgXRes, imgYRes + yRel, 0, 0);
                if (hwnd == hSizeboxRightBottom)
                    cropReversible(imgXRes + xRel, imgYRes + yRel, 0, 0);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) _T(""));
            }
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
