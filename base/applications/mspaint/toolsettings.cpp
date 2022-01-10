/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/toolsettings.cpp
 * PURPOSE:     Window procedure of the tool settings window
 * PROGRAMMERS: Benedikt Freisen
 *              Stanislav Motylkov
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LRESULT CToolSettingsWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, WINBOOL& bHandled)
{
    RECT trackbarZoomPos = {1, 1, 1 + 40, 1 + 64};
    trackbarZoom.Create(TRACKBAR_CLASS, m_hWnd, trackbarZoomPos, NULL, WS_CHILD | TBS_VERT | TBS_AUTOTICKS);
    trackbarZoom.SendMessage(TBM_SETRANGE, (WPARAM) TRUE, MAKELPARAM(0, 6));
    trackbarZoom.SendMessage(TBM_SETPOS, (WPARAM) TRUE, (LPARAM) 3);
    return 0;
}

LRESULT CToolSettingsWindow::OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!zoomTo(125 << trackbarZoom.SendMessage(TBM_GETPOS, 0, 0), 0, 0))
    {
        OnToolsModelZoomChanged(nMsg, wParam, lParam, bHandled);
    }
    return 0;
}

LRESULT CToolSettingsWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rect1 = { 0, 0, 42, 66 };
    RECT rect2 = { 0, 70, 42, 136 };

    DefWindowProc(WM_PAINT, wParam, lParam);

    HDC hdc = GetDC();
    DrawEdge(hdc, &rect1, BDR_SUNKENOUTER, (toolsModel.GetActiveTool() == TOOL_ZOOM) ? BF_RECT : BF_RECT | BF_MIDDLE);
    DrawEdge(hdc, &rect2, (toolsModel.GetActiveTool() >= TOOL_RECT) ? BDR_SUNKENOUTER : 0, BF_RECT | BF_MIDDLE);
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_TEXT:
        {
            HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
            Rectangle(hdc, 2, toolsModel.IsBackgroundTransparent() * 31 + 2, 41, toolsModel.IsBackgroundTransparent() * 31 + 33);
            DeleteObject(SelectObject(hdc, oldPen));
            DrawIconEx(hdc, 1, 2, hNontranspIcon, 40, 30, 0, NULL, DI_NORMAL);
            DrawIconEx(hdc, 1, 33, hTranspIcon, 40, 30, 0, NULL, DI_NORMAL);
            break;
        }
        case TOOL_RUBBER:
        {
            int i;
            HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
            for(i = 0; i < 4; i++)
            {
                if (toolsModel.GetRubberRadius() == i + 2)
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
            HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
            Rectangle(hdc, toolsModel.GetBrushStyle() % 3 * 13 + 2, toolsModel.GetBrushStyle() / 3 * 15 + 2, toolsModel.GetBrushStyle() % 3 * 13 + 15,
                      toolsModel.GetBrushStyle() / 3 * 15 + 17);
            DeleteObject(SelectObject(hdc, oldPen));
            for(i = 0; i < 12; i++)
                Brush(hdc, i % 3 * 13 + 7, i / 3 * 15 + 8, i % 3 * 13 + 7, i / 3 * 15 + 8,
                      GetSysColor((i == toolsModel.GetBrushStyle()) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), i);
            break;
        }
        case TOOL_AIRBRUSH:
        {
            HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
            SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
            switch (toolsModel.GetAirBrushWidth())
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
                     GetSysColor((toolsModel.GetAirBrushWidth() == 5) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 5);
            Airbrush(hdc, 30, 15,
                     GetSysColor((toolsModel.GetAirBrushWidth() == 8) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 8);
            Airbrush(hdc, 8, 45,
                     GetSysColor((toolsModel.GetAirBrushWidth() == 3) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 3);
            Airbrush(hdc, 27, 45,
                     GetSysColor((toolsModel.GetAirBrushWidth() == 12) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT), 12);
            DeleteObject(SelectObject(hdc, oldPen));
            break;
        }
        case TOOL_LINE:
        case TOOL_BEZIER:
        {
            int i;
            HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
            for(i = 0; i < 5; i++)
            {
                if (toolsModel.GetLineWidth() == i + 1)
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
            HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_NULL, 0, 0));
            for(i = 0; i < 3; i++)
            {
                if (toolsModel.GetShapeStyle() == i)
                {
                    SelectObject(hdc, GetSysColorBrush(COLOR_HIGHLIGHT));
                    Rectangle(hdc, 2, i * 20 + 2, 41, i * 20 + 22);
                }
            }
            Rect(hdc, 5, 6, 37, 16,
                 GetSysColor((toolsModel.GetShapeStyle() == 0) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT),
                 GetSysColor(COLOR_APPWORKSPACE), 1, 0);
            Rect(hdc, 5, 26, 37, 36,
                 GetSysColor((toolsModel.GetShapeStyle() == 1) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT),
                 GetSysColor(COLOR_APPWORKSPACE), 1, 1);
            Rect(hdc, 5, 46, 37, 56, GetSysColor(COLOR_APPWORKSPACE), GetSysColor(COLOR_APPWORKSPACE),
                 1, 1);
            for(i = 0; i < 5; i++)
            {
                if (toolsModel.GetLineWidth() == i + 1)
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
        case TOOL_FILL:
        case TOOL_COLOR:
        case TOOL_ZOOM:
        case TOOL_PEN:
            break;
    }
    ReleaseDC(hdc);
    return 0;
}

LRESULT CToolSettingsWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            if ((y > 1) && (y < 64))
                toolsModel.SetBackgroundTransparent((y - 2) / 31);
            break;
        case TOOL_RUBBER:
            if ((y > 1) && (y < 62))
                toolsModel.SetRubberRadius((y - 2) / 15 + 2);
            break;
        case TOOL_BRUSH:
            if ((x > 1) && (x < 40) && (y > 1) && (y < 62))
                toolsModel.SetBrushStyle((y - 2) / 15 * 3 + (x - 2) / 13);
            break;
        case TOOL_AIRBRUSH:
            if (y < 62)
            {
                if (y < 30)
                {
                    if (x < 20)
                        toolsModel.SetAirBrushWidth(5);
                    else
                        toolsModel.SetAirBrushWidth(8);
                }
                else
                {
                    if (x < 15)
                        toolsModel.SetAirBrushWidth(3);
                    else
                        toolsModel.SetAirBrushWidth(12);
                }
            }
            break;
        case TOOL_LINE:
        case TOOL_BEZIER:
            if (y <= 62)
                toolsModel.SetLineWidth((y - 2) / 12 + 1);
            break;
        case TOOL_RECT:
        case TOOL_SHAPE:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            if (y <= 60)
                toolsModel.SetShapeStyle((y - 2) / 20);
            if ((y >= 70) && (y <= 132))
                toolsModel.SetLineWidth((y - 72) / 12 + 1);
            break;
        case TOOL_FILL:
        case TOOL_COLOR:
        case TOOL_ZOOM:
        case TOOL_PEN:
            break;
    }
    return 0;
}

LRESULT CToolSettingsWindow::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Invalidate();
    trackbarZoom.ShowWindow((wParam == TOOL_ZOOM) ? SW_SHOW : SW_HIDE);
    return 0;
}

LRESULT CToolSettingsWindow::OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Invalidate();
    return 0;
}

LRESULT CToolSettingsWindow::OnToolsModelZoomChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int tbPos = 0;
    int tempZoom = toolsModel.GetZoom();

    while (tempZoom > MIN_ZOOM)
    {
        tbPos++;
        tempZoom = tempZoom >> 1;
    }
    trackbarZoom.SendMessage(TBM_SETPOS, (WPARAM) TRUE, (LPARAM) tbPos);
    return 0;
}
