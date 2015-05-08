/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/palette.cpp
 * PURPOSE:     Window procedure of the palette window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include "palette.h"

/* FUNCTIONS ********************************************************/

void
RegisterWclPal()
{
    WNDCLASSEX wclPal;
    /* initializing and registering the window class used for the palette window */
    wclPal.hInstance        = hProgInstance;
    wclPal.lpszClassName    = _T("Palette");
    wclPal.lpfnWndProc      = PalWinProc;
    wclPal.style            = CS_DBLCLKS;
    wclPal.cbSize           = sizeof(WNDCLASSEX);
    wclPal.hIcon            = NULL;
    wclPal.hIconSm          = NULL;
    wclPal.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wclPal.lpszMenuName     = NULL;
    wclPal.cbClsExtra       = 0;
    wclPal.cbWndExtra       = 0;
    wclPal.hbrBackground    = GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wclPal);
}

LRESULT CALLBACK
PalWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            RECT rc = { 0, 0, 31, 32 };
            HDC hDC = GetDC(hwnd);
            HPEN oldPen;
            HBRUSH oldBrush;
            int i, a, b;

            DefWindowProc(hwnd, message, wParam, lParam);

            for(b = 2; b < 30; b++)
                for(a = 2; a < 29; a++)
                    if ((a + b) % 2 == 1)
                        SetPixel(hDC, a, b, GetSysColor(COLOR_BTNHILIGHT));

            DrawEdge(hDC, &rc, EDGE_RAISED, BF_TOPLEFT);
            DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_TOPLEFT | BF_BOTTOMRIGHT);
            SetRect(&rc, 11, 12, 26, 27);
            DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
            oldPen = (HPEN) SelectObject(hDC, CreatePen(PS_NULL, 0, 0));
            oldBrush = (HBRUSH) SelectObject(hDC, CreateSolidBrush(bgColor));
            Rectangle(hDC, rc.left, rc.top + 2, rc.right - 1, rc.bottom - 1);
            DeleteObject(SelectObject(hDC, oldBrush));
            SetRect(&rc, 4, 5, 19, 20);
            DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
            oldBrush = (HBRUSH) SelectObject(hDC, CreateSolidBrush(fgColor));
            Rectangle(hDC, rc.left + 2, rc.top + 2, rc.right - 1, rc.bottom - 1);
            DeleteObject(SelectObject(hDC, oldBrush));
            DeleteObject(SelectObject(hDC, oldPen));

            for(i = 0; i < 28; i++)
            {
                SetRect(&rc, 31 + (i % 14) * 16,
                        0 + (i / 14) * 16, 16 + 31 + (i % 14) * 16, 16 + 0 + (i / 14) * 16);
                DrawEdge(hDC, &rc, EDGE_RAISED, BF_TOPLEFT);
                DrawEdge(hDC, &rc, BDR_SUNKENOUTER, BF_RECT);
                oldPen = (HPEN) SelectObject(hDC, CreatePen(PS_NULL, 0, 0));
                oldBrush = (HBRUSH) SelectObject(hDC, CreateSolidBrush(palColors[i]));
                Rectangle(hDC, rc.left + 2, rc.top + 2, rc.right - 1, rc.bottom - 1);
                DeleteObject(SelectObject(hDC, oldBrush));
                DeleteObject(SelectObject(hDC, oldPen));
            }
            ReleaseDC(hwnd, hDC);
            break;
        }
        case WM_LBUTTONDOWN:
            if (GET_X_LPARAM(lParam) >= 31)
            {
                fgColor = palColors[(GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14];
                InvalidateRect(hwnd, NULL, FALSE);
                if (activeTool == 10)
                    ForceRefreshSelectionContents();
            }
            break;
        case WM_RBUTTONDOWN:
            if (GET_X_LPARAM(lParam) >= 31)
            {
                bgColor = palColors[(GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14];
                InvalidateRect(hwnd, NULL, FALSE);
                if (activeTool == 10)
                    ForceRefreshSelectionContents();
            }
            break;
        case WM_LBUTTONDBLCLK:
            if (GET_X_LPARAM(lParam) >= 31)
                if (ChooseColor(&choosecolor))
                {
                    palColors[(GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14] =
                        choosecolor.rgbResult;
                    fgColor = choosecolor.rgbResult;
                    InvalidateRect(hwnd, NULL, FALSE);
                    if (activeTool == 10)
                        ForceRefreshSelectionContents();
                }
            break;
        case WM_RBUTTONDBLCLK:
            if (GET_X_LPARAM(lParam) >= 31)
                if (ChooseColor(&choosecolor))
                {
                    palColors[(GET_X_LPARAM(lParam) - 31) / 16 + (GET_Y_LPARAM(lParam) / 16) * 14] =
                        choosecolor.rgbResult;
                    bgColor = choosecolor.rgbResult;
                    InvalidateRect(hwnd, NULL, FALSE);
                    if (activeTool == 10)
                        ForceRefreshSelectionContents();
                }
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
