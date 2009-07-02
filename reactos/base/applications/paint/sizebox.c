/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        sizebox.c
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include "globalvar.h"
#include "drawing.h"
#include "history.h"
#include "mouse.h"

/* FUNCTIONS ********************************************************/

BOOL resizing = FALSE;
short xOrig;
short yOrig;

LRESULT CALLBACK SizeboxWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SETCURSOR:
            {
                if ((hwnd==hSizeboxLeftTop)||(hwnd==hSizeboxRightBottom))
                    SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
                if ((hwnd==hSizeboxLeftBottom)||(hwnd==hSizeboxRightTop))
                    SetCursor(LoadCursor(NULL, IDC_SIZENESW));
                if ((hwnd==hSizeboxLeftCenter)||(hwnd==hSizeboxRightCenter))
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                if ((hwnd==hSizeboxCenterTop)||(hwnd==hSizeboxCenterBottom))
                    SetCursor(LoadCursor(NULL, IDC_SIZENS));
            }
            break;
        case WM_LBUTTONDOWN:
            resizing = TRUE;
            xOrig = LOWORD(lParam);
            yOrig = HIWORD(lParam);
            SetCapture(hwnd);
            break;
        case WM_MOUSEMOVE:
            // TODO: print things in the status bar
            break;
        case WM_LBUTTONUP:
            if (resizing)
            {
                short xRel;
                short yRel;
                ReleaseCapture();
                resizing = FALSE;
                xRel = ((short)LOWORD(lParam)-xOrig)*1000/zoom;
                yRel = ((short)HIWORD(lParam)-yOrig)*1000/zoom;
                if (hwnd==hSizeboxLeftTop)
                    cropReversible(imgXRes-xRel, imgYRes-yRel, xRel, yRel);
                if (hwnd==hSizeboxCenterTop)
                    cropReversible(imgXRes, imgYRes-yRel, 0, yRel);
                if (hwnd==hSizeboxRightTop)
                    cropReversible(imgXRes+xRel, imgYRes-yRel, 0, yRel);
                if (hwnd==hSizeboxLeftCenter)
                    cropReversible(imgXRes-xRel, imgYRes, xRel, 0);
                if (hwnd==hSizeboxRightCenter)
                    cropReversible(imgXRes+xRel, imgYRes, 0, 0);
                if (hwnd==hSizeboxLeftBottom)
                    cropReversible(imgXRes-xRel, imgYRes+yRel, xRel, 0);
                if (hwnd==hSizeboxCenterBottom)
                    cropReversible(imgXRes, imgYRes+yRel, 0, 0);
                if (hwnd==hSizeboxRightBottom)
                    cropReversible(imgXRes+xRel, imgYRes+yRel, 0, 0);
            }
            break;

        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
