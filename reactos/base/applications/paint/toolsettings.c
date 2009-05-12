/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        toolsettings.c
 * PURPOSE:     Window procedure of the tool settings window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include "globalvar.h"

/* FUNCTIONS ********************************************************/

LRESULT CALLBACK SettingsWinProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
            {
                DefWindowProc (hwnd, message, wParam, lParam);
                
                HDC hdc = GetDC(hwnd);
                
                int rectang[4] = {0, 0, 42, 66};
                DrawEdge(hdc, (LPRECT)&rectang, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);
                int rectang2[4] = {0, 70, 42, 136};
                if (activeTool>=13)
                    DrawEdge(hdc, (LPRECT)&rectang2, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);
                else
                    DrawEdge(hdc, (LPRECT)&rectang2, 0, BF_RECT | BF_MIDDLE);
                switch (activeTool)
                {
                    case 1:
                    case 2:
                    case 10:
                        {
                            HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                            Rectangle(hdc, 2, transpBg*31+2, 41, transpBg*31+33);
                            DeleteObject(SelectObject(hdc, oldPen));
                            DrawIconEx(hdc, 1, 2, hNontranspIcon, 40, 30, 0, NULL, DI_NORMAL);
                            DrawIconEx(hdc, 1, 33, hTranspIcon, 40, 30, 0, NULL, DI_NORMAL);
                        }
                        break;
                    case 3:
                        {
                            int i;
                            HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                            for (i=0; i<4; i++)
                            {
                                if (rubberRadius==i+2)
                                {
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                                    Rectangle(hdc, 14, i*15+2, 29, i*15+17);
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                                } else SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
                                Rectangle(hdc, 19-i, i*14+7, 24+i, i*16+12);
                            }
                            DeleteObject(SelectObject(hdc, oldPen));
                        }
                        break;
                    case 8:
                        {
                            HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                            Rectangle(hdc, brushStyle%3*13+2, brushStyle/3*15+2, brushStyle%3*13+15, brushStyle/3*15+17);
                            DeleteObject(SelectObject(hdc, oldPen));
                            int i;
                            for (i=0; i<12; i++)
                            if (i==brushStyle)
                                Brush(hdc, i%3*13+7, i/3*15+8, i%3*13+7, i/3*15+8, GetSysColor(COLOR_HIGHLIGHTTEXT), i);
                            else
                                Brush(hdc, i%3*13+7, i/3*15+8, i%3*13+7, i/3*15+8, GetSysColor(COLOR_WINDOWTEXT), i);
                        }
                        break;
                    case 9:
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
                            if (airBrushWidth==5)
                                Airbrush(hdc, 10, 15, GetSysColor(COLOR_HIGHLIGHTTEXT), 5);
                            else
                                Airbrush(hdc, 10, 15, GetSysColor(COLOR_WINDOWTEXT), 5);
                            if (airBrushWidth==8)
                                Airbrush(hdc, 30, 15, GetSysColor(COLOR_HIGHLIGHTTEXT), 8);
                            else
                                Airbrush(hdc, 30, 15, GetSysColor(COLOR_WINDOWTEXT), 8);
                            if (airBrushWidth==3)
                                Airbrush(hdc, 8, 45, GetSysColor(COLOR_HIGHLIGHTTEXT), 3);
                            else
                                Airbrush(hdc, 8, 45, GetSysColor(COLOR_WINDOWTEXT), 3);
                            if (airBrushWidth==12)
                                Airbrush(hdc, 27, 45, GetSysColor(COLOR_HIGHLIGHTTEXT), 12);
                            else
                                Airbrush(hdc, 27, 45, GetSysColor(COLOR_WINDOWTEXT), 12);
                            DeleteObject(SelectObject(hdc, oldPen));
                        }
                        break;
                    case 11:
                    case 12:
                        {
                            int i;
                            HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                            for (i=0; i<5; i++)
                            {
                                if (lineWidth==i+1)
                                {
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                                    Rectangle(hdc, 2, i*12+2, 41, i*12+14);
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                                } else SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
                                Rectangle(hdc, 5, i*12+6, 38, i*12+8+i);
                            }
                            DeleteObject(SelectObject(hdc, oldPen));
                        }
                        break;
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                        {
                            int i;
                            HPEN oldPen = SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
                            for (i=0; i<3; i++)
                            {
                                if (shapeStyle==i)
                                {
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                                    Rectangle(hdc, 2, i*20+2, 41, i*20+22);
                                }
                            }
                            if (shapeStyle==0)
                                Rect(hdc, 5, 6, 37, 16, GetSysColor(COLOR_HIGHLIGHTTEXT), GetSysColor(COLOR_APPWORKSPACE), 1, FALSE);
                            else
                                Rect(hdc, 5, 6, 37, 16, GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_APPWORKSPACE), 1, FALSE);
                            if (shapeStyle==1)
                                Rect(hdc, 5, 26, 37, 36, GetSysColor(COLOR_HIGHLIGHTTEXT), GetSysColor(COLOR_APPWORKSPACE), 1, TRUE);
                            else
                                Rect(hdc, 5, 26, 37, 36, GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_APPWORKSPACE), 1, TRUE);
                            Rect(hdc, 5, 46, 37, 56, GetSysColor(COLOR_APPWORKSPACE), GetSysColor(COLOR_APPWORKSPACE), 1, TRUE);
                            for (i=0; i<5; i++)
                            {
                                if (lineWidth==i+1)
                                {
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                                    Rectangle(hdc, 2, i*12+72, 41, i*12+84);
                                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                                } else SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
                                Rectangle(hdc, 5, i*12+76, 38, i*12+78+i);
                            }
                            DeleteObject(SelectObject(hdc, oldPen));
                        }
                        break;
                }
                ReleaseDC(hwnd, hdc);
            }
            break;
        case WM_LBUTTONDOWN:
            {
                switch (activeTool)
                {
                    case 1:
                    case 2:
                    case 10:
                        if ((HIWORD(lParam)>1)&&(HIWORD(lParam)<64))
                        {
                            transpBg = (HIWORD(lParam)-2)/31;
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        break;
                    case 3:
                        if ((HIWORD(lParam)>1)&&(HIWORD(lParam)<62))
                        {
                            rubberRadius = (HIWORD(lParam)-2)/15+2;
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        break;
                    case 8:
                        if ((LOWORD(lParam)>1)&&(LOWORD(lParam)<40)&&(HIWORD(lParam)>1)&&(HIWORD(lParam)<62))
                        {
                            brushStyle = (HIWORD(lParam)-2)/15*3+(LOWORD(lParam)-2)/13;
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        break;
                    case 9:
                        if (HIWORD(lParam)<62)
                        {
                            if (HIWORD(lParam)<30)
                            {
                                if (LOWORD(lParam)<20) airBrushWidth=5; else airBrushWidth=8;
                            }else
                            {
                                if (LOWORD(lParam)<15) airBrushWidth=3; else airBrushWidth=12;
                            }
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        break;
                    case 11:
                    case 12:
                        if (HIWORD(lParam)<=62)
                        {
                            lineWidth = (HIWORD(lParam)-2)/12+1;
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        break;
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                        if (HIWORD(lParam)<=60)
                        {
                            shapeStyle = (HIWORD(lParam)-2)/20;
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        if ((HIWORD(lParam)>=70)&&(HIWORD(lParam)<=132))
                        {
                            lineWidth = (HIWORD(lParam)-72)/12+1;
                            SendMessage(hwnd, WM_PAINT, 0, 0);
                        }
                        break;
                }
            }
            break;
        
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

