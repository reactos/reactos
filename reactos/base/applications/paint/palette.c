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
                int i;
                long rectang[4] = {0, 0, 31, 32};
                int a;
                int b;
                HDC hdc = GetDC(hwnd);
                HPEN oldPen;
                HBRUSH oldBrush;

                DefWindowProc (hwnd, message, wParam, lParam);

                for (b=2; b<30; b++) for (a=2; a<29; a++) if ((a+b)%2==1) SetPixel(hdc, a, b, GetSysColor(COLOR_BTNHILIGHT));
                DrawEdge(hdc, (LPRECT)&rectang, EDGE_RAISED, BF_TOPLEFT);
                DrawEdge(hdc, (LPRECT)&rectang, BDR_SUNKENOUTER, BF_TOPLEFT|BF_BOTTOMRIGHT);
                    rectang[0] = 11;
                    rectang[1] = 12;
                    rectang[2] = 26;
                    rectang[3] = 27;
                    DrawEdge(hdc, (LPRECT)&rectang, BDR_RAISEDINNER, BF_RECT|BF_MIDDLE);
                    oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    oldBrush = SelectObject(hdc, CreateSolidBrush(bgColor));
                    Rectangle( hdc, rectang[0]+2,rectang[1]+2,rectang[2]-1,rectang[3]-1);
                    DeleteObject(SelectObject(hdc, oldBrush));
                    DeleteObject(SelectObject(hdc, oldPen));
                    rectang[0] = 4;
                    rectang[1] = 5;
                    rectang[2] = 19;
                    rectang[3] = 20;
                    DrawEdge(hdc, (LPRECT)&rectang, BDR_RAISEDINNER, BF_RECT|BF_MIDDLE);
                    oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    oldBrush = SelectObject(hdc, CreateSolidBrush(fgColor));
                    Rectangle( hdc, rectang[0]+2,rectang[1]+2,rectang[2]-1,rectang[3]-1);
                    DeleteObject(SelectObject(hdc, oldBrush));
                    DeleteObject(SelectObject(hdc, oldPen));
                for (i=0; i<28; i++)
                {
                    rectang[0] = 31+(i%14)*16;
                    rectang[1] = 0+(i/14)*16;
                    rectang[2] = 16+rectang[0];
                    rectang[3] = 16+rectang[1];
                    DrawEdge(hdc, (LPRECT)&rectang, EDGE_RAISED, BF_TOPLEFT);
                    DrawEdge(hdc, (LPRECT)&rectang, BDR_SUNKENOUTER, BF_RECT);
                    oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    oldBrush = SelectObject(hdc, CreateSolidBrush(palColors[i]));
                    Rectangle( hdc, rectang[0]+2,rectang[1]+2,rectang[2]-1,rectang[3]-1);
                    DeleteObject(SelectObject(hdc, oldBrush));
                    DeleteObject(SelectObject(hdc, oldPen));
                }
                ReleaseDC(hwnd, hdc);
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

