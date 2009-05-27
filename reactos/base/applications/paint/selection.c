/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        selection.c
 * PURPOSE:     Window procedure of the selection window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include "globalvar.h"
#include "drawing.h"
#include "history.h"
#include "mouse.h"

/* FUNCTIONS ********************************************************/

BOOL moving = FALSE;
short xPos;
short yPos;

LRESULT CALLBACK SelectionWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
            {
                if (!moving)
                {
                    DefWindowProc (hwnd, message, wParam, lParam);
                    HDC hdc=GetDC(hwnd);
                    SelectionFrame(hdc, 1, 1, rectSel_dest[2]*zoom/1000+5, rectSel_dest[3]*zoom/1000+5);
                    ReleaseDC(hwnd, hdc);
                }
            }
            break;
        case WM_LBUTTONDOWN:
            xPos = LOWORD(lParam);
            yPos = HIWORD(lParam);
            SetCapture(hwnd);
            moving = TRUE;
            break;
        case WM_MOUSEMOVE:
            if (moving)
            {
                resetToU1();
                rectSel_dest[0]+=(short)LOWORD(lParam)-xPos;
                rectSel_dest[1]+=(short)HIWORD(lParam)-yPos;
                
                Rect(hDrawingDC, rectSel_src[0], rectSel_src[1], rectSel_src[0]+rectSel_src[2], rectSel_src[1]+rectSel_src[3], bgColor, bgColor, 0, TRUE);
                if (transpBg==0)
                    BitBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0, SRCCOPY);
                else
                    BitBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0, SRCAND);
                    //TransparentBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0, rectSel_dest[2], rectSel_dest[3], bgColor);
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                xPos = LOWORD(lParam);
                yPos = HIWORD(lParam);
                //SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            break;
        case WM_LBUTTONUP:
            if (moving)
            {
                moving = FALSE;
                ReleaseCapture();
                placeSelWin();
                ShowWindow(hSelection, SW_HIDE);
                ShowWindow(hSelection, SW_SHOW);
            }
            break;
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
