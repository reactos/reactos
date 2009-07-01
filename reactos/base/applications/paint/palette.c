/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        palette.c
 * PURPOSE:     Window procedure of the palette window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include "globalvar.h"

/* FUNCTIONS ********************************************************/

LRESULT CALLBACK PalWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
            {
                RECT rc = {0, 0, 31, 32};
                HDC hDC = GetDC(hwnd);
                HPEN oldPen;
                HBRUSH oldBrush;
                int i, a, b;

                DefWindowProc (hwnd, message, wParam, lParam);

                for (b = 2; b < 30; b++)
                    for (a = 2; a < 29; a++)
                        if ((a + b) % 2 == 1)
                            SetPixel(hDC, a, b, GetSysColor(COLOR_BTNHILIGHT));

                DrawEdge(hDC, &rc, EDGE_RAISED, BF_TOPLEFT);
                DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_TOPLEFT|BF_BOTTOMRIGHT);
                SetRect(&rc, 11, 12, 26, 27);
                DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT|BF_MIDDLE);
                oldPen = SelectObject(hDC, CreatePen(PS_NULL, 0, 0));
                oldBrush = SelectObject(hDC, CreateSolidBrush(bgColor));
                Rectangle(hDC, rc.left, rc.top + 2, rc.right -1, rc.bottom - 1);
                DeleteObject(SelectObject(hDC, oldBrush));
                SetRect(&rc, 4, 5, 19, 20);
                DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT|BF_MIDDLE);
                oldBrush = SelectObject(hDC, CreateSolidBrush(fgColor));
                Rectangle( hDC, rc.left + 2,rc.top + 2, rc.right - 1, rc.bottom - 1);
                DeleteObject(SelectObject(hDC, oldBrush));
                DeleteObject(SelectObject(hDC, oldPen));

                for (i=0; i<28; i++)
                {
                    SetRect(&rc, 31 + (i % 14) * 16,
                                 0 + (i / 14) * 16,
                                 16 + 31 + (i % 14) * 16,
                                16 + 0 + (i / 14) * 16);
                    DrawEdge(hDC, &rc, EDGE_RAISED, BF_TOPLEFT);
                    DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_RECT);
                    oldPen = SelectObject(hDC, CreatePen(PS_NULL, 0, 0));
                    oldBrush = SelectObject(hDC, CreateSolidBrush(palColors[i]));
                    Rectangle(hDC, rc.left + 2,rc.top + 2,rc.right - 1, rc.bottom - 1);
                    DeleteObject(SelectObject(hDC, oldBrush));
                    DeleteObject(SelectObject(hDC, oldPen));
                }
                ReleaseDC(hwnd, hDC);
            }
            break;
        case WM_LBUTTONDOWN:
            if (LOWORD(lParam)>=31)
            {
                fgColor = palColors[(LOWORD(lParam)-31)/16+(HIWORD(lParam)/16)*14];
                SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            break;
        case WM_RBUTTONDOWN:
            if (LOWORD(lParam)>=31)
            {
                bgColor = palColors[(LOWORD(lParam)-31)/16+(HIWORD(lParam)/16)*14];
                SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            break;
        case WM_LBUTTONDBLCLK:
            if (LOWORD(lParam)>=31) if (ChooseColor(&choosecolor))
            {
                palColors[(LOWORD(lParam)-31)/16+(HIWORD(lParam)/16)*14] = choosecolor.rgbResult;
                fgColor = choosecolor.rgbResult;
                SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            break;
        case WM_RBUTTONDBLCLK:
            if (LOWORD(lParam)>=31) if (ChooseColor(&choosecolor))
            {
                palColors[(LOWORD(lParam)-31)/16+(HIWORD(lParam)/16)*14] = choosecolor.rgbResult;
                bgColor = choosecolor.rgbResult;
                SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            break;
        
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

