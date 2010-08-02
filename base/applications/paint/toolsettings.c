/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/toolsettings.c
 * PURPOSE:     Window procedure of the tool settings window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include <commctrl.h>
#include "globalvar.h"
#include "drawing.h"
#include "winproc.h"

/* FUNCTIONS ********************************************************/

extern void zoomTo(int, int, int);

LRESULT CALLBACK
SettingsWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_VSCROLL:
            zoomTo(125 << SendMessage(hTrackbarZoom, TBM_GETPOS, 0, 0), 0, 0);
            break;
        case WM_PAINT:
        {
            HDC hdc = GetDC(hwnd);
            RECT rect1 = { 0, 0, 42, 66 };
            RECT rect2 = { 0, 70, 42, 136 };

            DefWindowProc(hwnd, message, wParam, lParam);

            DrawEdge(hdc, &rect1, BDR_SUNKENOUTER, (activeTool == TOOL_ZOOM) ? BF_RECT : BF_RECT | BF_MIDDLE);
            DrawEdge(hdc, &rect2, (activeTool >= TOOL_RECT) ? BDR_SUNKENOUTER : 0, BF_RECT | BF_MIDDLE);
            switch (activeTool)
            {
                case TOOL_FREESEL:
                case TOOL_RECTSEL:
                case TOOL_TEXT:
                {
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                    Rectangle(hdc, 2, transpBg * 31 + 2, 41, transpBg * 31 + 33);
                    DeleteObject(SelectObject(hdc, oldPen));
                    DrawIconEx(hdc, 1, 2, hNontranspIcon, 40, 30, 0, NULL, DI_NORMAL);
                    DrawIconEx(hdc, 1, 33, hTranspIcon, 40, 30, 0, NULL, DI_NORMAL);
                    break;
                }
                case TOOL_RUBBER:
                {
                    int i;
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    for(i = 0; i < 4; i++)
                    {
                        if (rubberRadius == i + 2)
                        {
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                            Rectangle(hdc, 14, i * 15 + 2, 29, i * 15 + 17);
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                        }
                        else
                            SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
                        Rectangle(hdc, 19 - i, i * 14 + 7, 24 + i, i * 16 + 12);
                    }
                    DeleteObject(SelectObject(hdc, oldPen));
                    break;
                }
                case TOOL_BRUSH:
                {
                    int i;
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                    Rectangle(hdc, brushStyle % 3 * 13 + 2, brushStyle / 3 * 15 + 2, brushStyle % 3 * 13 + 15,
                              brushStyle / 3 * 15 + 17);
                    DeleteObject(SelectObject(hdc, oldPen));
                    for(i = 0; i < 12; i++)
                        Brush(hdc, i % 3 * 13 + 7, i / 3 * 15 + 8, i % 3 * 13 + 7, i / 3 * 15 + 8,
                              GetSysColor((i == brushStyle) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), i);
                    break;
                }
                case TOOL_AIRBRUSH:
                {
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                    switch (airBrushWidth)
                    {
                        case 5:
                            Rectangle(hdc, 2, 2, 21, 31);
                            break;
                        case 8:
                            Rectangle(hdc, 20, 2, 41, 31);
                            break;
                        case 3:
                            Rectangle(hdc, 2, 30, 16, 61);
                            break;
                        case 12:
                            Rectangle(hdc, 15, 30, 41, 61);
                            break;
                    }
                    Airbrush(hdc, 10, 15,
                             GetSysColor((airBrushWidth == 5) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 5);
                    Airbrush(hdc, 30, 15,
                             GetSysColor((airBrushWidth == 8) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 8);
                    Airbrush(hdc, 8, 45,
                             GetSysColor((airBrushWidth == 3) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 3);
                    Airbrush(hdc, 27, 45,
                             GetSysColor((airBrushWidth == 12) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 12);
                    DeleteObject(SelectObject(hdc, oldPen));
                    break;
                }
                case TOOL_LINE:
                case TOOL_BEZIER:
                {
                    int i;
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    for(i = 0; i < 5; i++)
                    {
                        if (lineWidth == i + 1)
                        {
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                            Rectangle(hdc, 2, i * 12 + 2, 41, i * 12 + 14);
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                        }
                        else
                            SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
                        Rectangle(hdc, 5, i * 12 + 6, 38, i * 12 + 8 + i);
                    }
                    DeleteObject(SelectObject(hdc, oldPen));
                    break;
                }
                case TOOL_RECT:
                case TOOL_SHAPE:
                case TOOL_ELLIPSE:
                case TOOL_RRECT:
                {
                    int i;
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                    for(i = 0; i < 3; i++)
                    {
                        if (shapeStyle == i)
                        {
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                            Rectangle(hdc, 2, i * 20 + 2, 41, i * 20 + 22);
                        }
                    }
                    Rect(hdc, 5, 6, 37, 16,
                         GetSysColor((shapeStyle == 0) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT),
                         GetSysColor(COLOR_APPWORKSPACE), 1, 0);
                    Rect(hdc, 5, 26, 37, 36,
                         GetSysColor((shapeStyle == 1) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT),
                         GetSysColor(COLOR_APPWORKSPACE), 1, 1);
                    Rect(hdc, 5, 46, 37, 56, GetSysColor(COLOR_APPWORKSPACE), GetSysColor(COLOR_APPWORKSPACE),
                         1, 1);
                    for(i = 0; i < 5; i++)
                    {
                        if (lineWidth == i + 1)
                        {
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                            Rectangle(hdc, 2, i * 12 + 72, 41, i * 12 + 84);
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                        }
                        else
                            SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
                        Rectangle(hdc, 5, i * 12 + 76, 38, i * 12 + 78 + i);
                    }
                    DeleteObject(SelectObject(hdc, oldPen));
                    break;
                }
            }
            ReleaseDC(hwnd, hdc);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            switch (activeTool)
            {
                case TOOL_FREESEL:
                case TOOL_RECTSEL:
                case TOOL_TEXT:
                    if ((HIWORD(lParam) > 1) && (HIWORD(lParam) < 64))
                    {
                        transpBg = (HIWORD(lParam) - 2) / 31;
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    break;
                case TOOL_RUBBER:
                    if ((HIWORD(lParam) > 1) && (HIWORD(lParam) < 62))
                    {
                        rubberRadius = (HIWORD(lParam) - 2) / 15 + 2;
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    break;
                case TOOL_BRUSH:
                    if ((LOWORD(lParam) > 1) && (LOWORD(lParam) < 40) && (HIWORD(lParam) > 1)
                        && (HIWORD(lParam) < 62))
                    {
                        brushStyle = (HIWORD(lParam) - 2) / 15 * 3 + (LOWORD(lParam) - 2) / 13;
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    break;
                case TOOL_AIRBRUSH:
                    if (HIWORD(lParam) < 62)
                    {
                        if (HIWORD(lParam) < 30)
                        {
                            if (LOWORD(lParam) < 20)
                                airBrushWidth = 5;
                            else
                                airBrushWidth = 8;
                        }
                        else
                        {
                            if (LOWORD(lParam) < 15)
                                airBrushWidth = 3;
                            else
                                airBrushWidth = 12;
                        }
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    break;
                case TOOL_LINE:
                case TOOL_BEZIER:
                    if (HIWORD(lParam) <= 62)
                    {
                        lineWidth = (HIWORD(lParam) - 2) / 12 + 1;
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    break;
                case TOOL_RECT:
                case TOOL_SHAPE:
                case TOOL_ELLIPSE:
                case TOOL_RRECT:
                    if (HIWORD(lParam) <= 60)
                    {
                        shapeStyle = (HIWORD(lParam) - 2) / 20;
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    if ((HIWORD(lParam) >= 70) && (HIWORD(lParam) <= 132))
                    {
                        lineWidth = (HIWORD(lParam) - 72) / 12 + 1;
                        SendMessage(hwnd, WM_PAINT, 0, 0);
                    }
                    break;
            }
            break;
        }

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
