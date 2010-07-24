/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/selection.c
 * PURPOSE:     Window procedure of the selection window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include "globalvar.h"
#include "drawing.h"
#include "history.h"
#include "mouse.h"
#include "dib.h"

/* FUNCTIONS ********************************************************/

LPCTSTR cursors[9] = { IDC_SIZEALL, IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW,
    IDC_SIZEWE, IDC_SIZEWE, IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE
};

BOOL moving = FALSE;
int action = 0;
short xPos;
short yPos;
short xFrac;
short yFrac;

int
identifyCorner(short x, short y, short w, short h)
{
    if (y < 3)
    {
        if (x < 3)
            return 1;
        if ((x < w / 2 + 2) && (x >= w / 2 - 1))
            return 2;
        if (x >= w - 3)
            return 3;
    }
    if ((y < h / 2 + 2) && (y >= h / 2 - 1))
    {
        if (x < 3)
            return 4;
        if (x >= w - 3)
            return 5;
    }
    if (y >= h - 3)
    {
        if (x < 3)
            return 6;
        if ((x < w / 2 + 2) && (x >= w / 2 - 1))
            return 7;
        if (x >= w - 3)
            return 8;
    }
    return 0;
}

LRESULT CALLBACK
SelectionWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            if (!moving)
            {
                HDC hDC = GetDC(hwnd);
                DefWindowProc(hwnd, message, wParam, lParam);
                SelectionFrame(hDC, 1, 1, rectSel_dest[2] * zoom / 1000 + 5,
                               rectSel_dest[3] * zoom / 1000 + 5);
                ReleaseDC(hwnd, hDC);
            }
            break;
        }
        case WM_LBUTTONDOWN:
            xPos = LOWORD(lParam);
            yPos = HIWORD(lParam);
            SetCapture(hwnd);
            if (action != 0)
                SetCursor(LoadCursor(NULL, cursors[action]));
            moving = TRUE;
            break;
        case WM_MOUSEMOVE:
            if (moving)
            {
                int xDelta;
                int yDelta;
                resetToU1();
                xFrac += (short)LOWORD(lParam) - xPos;
                yFrac += (short)HIWORD(lParam) - yPos;
                if (zoom < 1000)
                {
                    xDelta = xFrac * 1000 / zoom;
                    xFrac = 0;
                    yDelta = yFrac * 1000 / zoom;
                    yFrac = 0;
                }
                else
                {
                    xDelta = xFrac * 1000 / zoom;
                    xFrac -= (xFrac * 1000 / zoom) * zoom / 1000;
                    yDelta = yFrac * 1000 / zoom;
                    yFrac -= (yFrac * 1000 / zoom) * zoom / 1000;
                }
                switch (action)
                {
                    case 0:
                        rectSel_dest[0] += xDelta;
                        rectSel_dest[1] += yDelta;
                        break;
                    case 1:
                        rectSel_dest[0] += xDelta;
                        rectSel_dest[1] += yDelta;
                        rectSel_dest[2] -= xDelta;
                        rectSel_dest[3] -= yDelta;
                        break;
                    case 2:
                        rectSel_dest[1] += yDelta;
                        rectSel_dest[3] -= yDelta;
                        break;
                    case 3:
                        rectSel_dest[2] += xDelta;
                        rectSel_dest[1] += yDelta;
                        break;
                    case 4:
                        rectSel_dest[0] += xDelta;
                        rectSel_dest[2] -= xDelta;
                        break;
                    case 5:
                        rectSel_dest[2] += xDelta;
                        break;
                    case 6:
                        rectSel_dest[0] += xDelta;
                        rectSel_dest[2] -= xDelta;
                        rectSel_dest[3] += yDelta;
                        break;
                    case 7:
                        rectSel_dest[3] += yDelta;
                        break;
                    case 8:
                        rectSel_dest[2] += xDelta;
                        rectSel_dest[3] += yDelta;
                        break;
                }

                if (action != 0)
                    StretchBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0, GetDIBWidth(hSelBm), GetDIBHeight(hSelBm), SRCCOPY);
                else
                if (transpBg == 0)
                    MaskBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3],
                            hSelDC, 0, 0, hSelMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND));
                else
                {
                    HBITMAP tempMask;
                    HBRUSH oldBrush;
                    HDC tempDC;
                    tempMask = CreateBitmap(rectSel_dest[2], rectSel_dest[3], 1, 1, NULL);
                    oldBrush = SelectObject(hSelDC, CreateSolidBrush(bgColor));
                    tempDC = CreateCompatibleDC(hSelDC);
                    SelectObject(tempDC, tempMask);
                    MaskBlt(tempDC, 0, 0, rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0, hSelMask, 0, 0,
                            MAKEROP4(NOTSRCCOPY, BLACKNESS));
                    DeleteDC(tempDC);
                    DeleteObject(SelectObject(hSelDC, oldBrush));

                    MaskBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3],
                            hSelDC, 0, 0, tempMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND));
                    DeleteObject(tempMask);
                }
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                xPos = LOWORD(lParam);
                yPos = HIWORD(lParam);
                //SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            else
            {
                int w = rectSel_dest[2] * zoom / 1000 + 6;
                int h = rectSel_dest[3] * zoom / 1000 + 6;
                xPos = LOWORD(lParam);
                yPos = HIWORD(lParam);
                action = identifyCorner(xPos, yPos, w, h);
                if (action != 0)
                    SetCursor(LoadCursor(NULL, cursors[action]));
            }
            break;
        case WM_LBUTTONUP:
            if (moving)
            {
                moving = FALSE;
                ReleaseCapture();
                if (action != 0)
                {
                    HDC hTempDC;
                    HBITMAP hTempBm;
                    hTempDC = CreateCompatibleDC(hSelDC);
                    hTempBm = CreateDIBWithProperties(rectSel_dest[2], rectSel_dest[3]);
                    SelectObject(hTempDC, hTempBm);
                    SelectObject(hSelDC, hSelBm);
                    StretchBlt(hTempDC, 0, 0, rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0,
                               GetDIBWidth(hSelBm), GetDIBHeight(hSelBm), SRCCOPY);
                    DeleteObject(hSelBm);
                    hSelBm = hTempBm;
                    hTempBm = CreateBitmap(rectSel_dest[2], rectSel_dest[3], 1, 1, NULL);
                    SelectObject(hTempDC, hTempBm);
                    SelectObject(hSelDC, hSelMask);
                    StretchBlt(hTempDC, 0, 0, rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0,
                               GetDIBWidth(hSelMask), GetDIBHeight(hSelMask), SRCCOPY);
                    DeleteObject(hSelMask);
                    hSelMask = hTempBm;
                    SelectObject(hSelDC, hSelBm);
                    DeleteDC(hTempDC);
                }
                placeSelWin();
                ShowWindow(hSelection, SW_HIDE);
                ShowWindow(hSelection, SW_SHOW);
            }
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
