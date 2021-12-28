/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/mouse.cpp
 * PURPOSE:     Things which should not be in the mouse event handler itself
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

POINT pointStack[256];
INT pointSP = 0;
BOOL drawing = FALSE;

/* FUNCTIONS ********************************************************/

void
placeSelWin()
{
    selectionWindow.MoveWindow(Zoomed(selectionModel.GetDestRectLeft()), Zoomed(selectionModel.GetDestRectTop()),
        Zoomed(selectionModel.GetDestRectWidth()) + 2 * GRIP_SIZE,
        Zoomed(selectionModel.GetDestRectHeight()) + 2 * GRIP_SIZE, TRUE);
    selectionWindow.BringWindowToTop();
    imageArea.InvalidateRect(NULL, FALSE);
}

void
regularize(LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    if (abs(x1 - x0) >= abs(y1 - y0))
        y1 = y0 + (y1 > y0 ? abs(x1 - x0) : -abs(x1 - x0));
    else
        x1 = x0 + (x1 > x0 ? abs(y1 - y0) : -abs(y1 - y0));
}

void
roundTo8Directions(LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    if (abs(x1 - x0) >= abs(y1 - y0))
    {
        if (abs(y1 - y0) * 5 < abs(x1 - x0) * 2)
            y1 = y0;
        else
            y1 = y0 + (y1 > y0 ? abs(x1 - x0) : -abs(x1 - x0));
    }
    else
    {
        if (abs(x1 - x0) * 5 < abs(y1 - y0) * 2)
            x1 = x0;
        else
            x1 = x0 + (x1 > x0 ? abs(y1 - y0) : -abs(y1 - y0));
    }
}

BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1)
{
    INT cxThreshold = toolsModel.GetLineWidth() + UnZoomed(GetSystemMetrics(SM_CXDRAG));
    INT cyThreshold = toolsModel.GetLineWidth() + UnZoomed(GetSystemMetrics(SM_CYDRAG));
    return (abs(x1 - x0) <= cxThreshold) && (abs(y1 - y0) <= cyThreshold);
}

void ToolBase::OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
{
    imageModel.CopyPrevious();
    start.x = last.x = x;
    start.y = last.y = y;
}

void ToolBase::OnMove(BUTTON_TYPE button, LONG x, LONG y)
{
    last.x = x;
    last.y = y;
}

void ToolBase::OnUp(BUTTON_TYPE button, LONG x, LONG y)
{
}

void ToolBase::OnCancelDraw()
{
    pointSP = 0;
}

void ToolBase::begin()
{
    m_hdc = imageModel.GetDC();
    m_fg = paletteModel.GetFgColor();
    m_bg = paletteModel.GetBgColor();
}

void ToolBase::end()
{
    m_hdc = NULL;
}

// TOOL_FREESEL
struct FreeSelTool : ToolBase
{
    FreeSelTool() : ToolBase(TOOL_FREESEL)
    {
    }
    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        selectionWindow.ShowWindow(SW_HIDE);
        selectionModel.ResetPtStack();
        selectionModel.PushToPtStack(x, y);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        if (selectionModel.PtStackSize() == 1)
            imageModel.CopyPrevious();
        selectionModel.PushToPtStack(max(0, min(x, imageModel.GetWidth())), max(0, min(y, imageModel.GetHeight())));
        imageModel.ResetToPrevious();
        selectionModel.DrawFramePoly(m_hdc);
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        selectionModel.CalculateBoundingBoxAndContents(m_hdc);
        if (selectionModel.PtStackSize() > 1)
        {
            selectionModel.DrawBackgroundPoly(m_hdc, m_bg);
            imageModel.CopyPrevious();

            selectionModel.DrawSelection(m_hdc);

            placeSelWin();
            selectionWindow.ShowWindow(SW_SHOW);
            ForceRefreshSelectionContents();
        }
        selectionModel.ResetPtStack();
    }
    virtual void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RECTSEL
struct RectSelTool : ToolBase
{
    RectSelTool() : ToolBase(TOOL_RECTSEL)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
                imageModel.CopyPrevious();
                selectionWindow.ShowWindow(SW_HIDE);
                selectionModel.SetSrcRectSizeToZero();
                break;
            case BUTTON_RIGHT:
                break;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        POINT temp;
        switch (button)
        {
            case BUTTON_LEFT:
                imageModel.ResetToPrevious();
                temp.x = max(0, min(x, imageModel.GetWidth()));
                temp.y = max(0, min(y, imageModel.GetHeight()));
                selectionModel.SetSrcAndDestRectFromPoints(start, temp);
                RectSel(m_hdc, start.x, start.y, temp.x, temp.y);
                break;
            case BUTTON_RIGHT:
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                imageModel.ResetToPrevious();
                if (selectionModel.IsSrcRectSizeNonzero())
                {
                    selectionModel.CalculateContents(m_hdc);
                    selectionModel.DrawBackgroundRect(m_hdc, m_bg);
                    imageModel.CopyPrevious();

                    selectionModel.DrawSelection(m_hdc);

                    placeSelWin();
                    selectionWindow.ShowWindow(SW_SHOW);
                    ForceRefreshSelectionContents();
                }
                break;
            case BUTTON_RIGHT:
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RUBBER
struct RubberTool : ToolBase
{
    RubberTool() : ToolBase(TOOL_RUBBER)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
                Erase(m_hdc, x, y, x, y, m_bg, toolsModel.GetRubberRadius());
                break;
            case BUTTON_RIGHT:
                Replace(m_hdc, x, y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
                break;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        switch (button)
        {
            case BUTTON_LEFT:
                Erase(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetRubberRadius());
                break;
            case BUTTON_RIGHT:
                Replace(m_hdc, last.x, last.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                Erase(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetRubberRadius());
                break;
            case BUTTON_RIGHT:
                Replace(m_hdc, last.x, last.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        ToolBase::OnCancelDraw();
    }
};

// TOOL_FILL
struct FillTool : ToolBase
{
    FillTool() : ToolBase(TOOL_FILL)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
                Fill(m_hdc, x, y, m_fg);
                break;
            case BUTTON_RIGHT:
                Fill(m_hdc, x, y, m_bg);
                break;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
    }
    virtual void OnCancelDraw()
    {
        ToolBase::OnCancelDraw();
    }
};

// TOOL_COLOR
struct ColorTool : ToolBase
{
    ColorTool() : ToolBase(TOOL_COLOR)
    {
    }
    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        COLORREF tempColor = GetPixel(m_hdc, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                if (tempColor != CLR_INVALID)
                    paletteModel.SetFgColor(tempColor);
                break;
            case BUTTON_RIGHT:
                if (tempColor != CLR_INVALID)
                    paletteModel.SetBgColor(tempColor);
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        COLORREF tempColor = GetPixel(m_hdc, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                if (tempColor != CLR_INVALID)
                    paletteModel.SetFgColor(tempColor);
                break;
            case BUTTON_RIGHT:
                if (tempColor != CLR_INVALID)
                    paletteModel.SetBgColor(tempColor);
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_ZOOM
struct ZoomTool : ToolBase
{
    ZoomTool() : ToolBase(TOOL_ZOOM)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
            {
                if (toolsModel.GetZoom() < MAX_ZOOM)
                    zoomTo(toolsModel.GetZoom() * 2, x, y);
                break;
            }
            case BUTTON_RIGHT:
            {
                if (toolsModel.GetZoom() > MIN_ZOOM)
                    zoomTo(toolsModel.GetZoom() / 2, x, y);
                break;
            }
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
    }
    virtual void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_PEN
struct PenTool : ToolBase
{
    PenTool() : ToolBase(TOOL_PEN)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
            {
                SetPixel(m_hdc, x, y, m_fg);
                break;
            }
            case BUTTON_RIGHT:
            {
                SetPixel(m_hdc, x, y, m_bg);
                break;
            }
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        switch (button)
        {
            case BUTTON_LEFT:
            {
                Line(m_hdc, last.x, last.y, x, y, m_fg, 1);
                break;
            }
            case BUTTON_RIGHT:
            {
                Line(m_hdc, last.x, last.y, x, y, m_bg, 1);
                break;
            }
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
            {
                Line(m_hdc, last.x, last.y, x, y, m_fg, 1);
                SetPixel(m_hdc, x, y, m_fg);
                break;
            }
            case BUTTON_RIGHT:
            {
                Line(m_hdc, last.x, last.y, x, y, m_bg, 1);
                SetPixel(m_hdc, x, y, m_bg);
                break;
            }
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_BRUSH
struct BrushTool : ToolBase
{
    BrushTool() : ToolBase(TOOL_BRUSH)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
                Brush(m_hdc, x, y, x, y, m_fg, toolsModel.GetBrushStyle());
                break;
            case BUTTON_RIGHT:
                Brush(m_hdc, x, y, x, y, m_bg, toolsModel.GetBrushStyle());
                break;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        switch (button)
        {
            case BUTTON_LEFT:
                Brush(m_hdc, last.x, last.y, x, y, m_fg, toolsModel.GetBrushStyle());
                break;
            case BUTTON_RIGHT:
                Brush(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetBrushStyle());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_AIRBRUSH
struct AirBrushTool : ToolBase
{
    AirBrushTool() : ToolBase(TOOL_AIRBRUSH)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        switch (button)
        {
            case BUTTON_LEFT:
                Airbrush(m_hdc, x, y, m_fg, toolsModel.GetAirBrushWidth());
                break;
            case BUTTON_RIGHT:
                Airbrush(m_hdc, x, y, m_bg, toolsModel.GetAirBrushWidth());
                break;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        switch (button)
        {
            case BUTTON_LEFT:
                Airbrush(m_hdc, x, y, m_fg, toolsModel.GetAirBrushWidth());
                break;
            case BUTTON_RIGHT:
                Airbrush(m_hdc, x, y, m_bg, toolsModel.GetAirBrushWidth());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_TEXT
struct TextTool : ToolBase
{
    TextTool() : ToolBase(TOOL_TEXT)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        POINT temp;
        switch (button)
        {
            case BUTTON_LEFT:
                imageModel.ResetToPrevious();
                temp.x = max(0, min(x, imageModel.GetWidth()));
                temp.y = max(0, min(y, imageModel.GetHeight()));
                selectionModel.SetSrcAndDestRectFromPoints(start, temp);
                RectSel(m_hdc, start.x, start.y, temp.x, temp.y);
                break;
            case BUTTON_RIGHT:
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                imageModel.ResetToPrevious();
                if (selectionModel.IsSrcRectSizeNonzero())
                {
                    imageModel.CopyPrevious();

                    placeSelWin();
                    selectionWindow.ShowWindow(SW_SHOW);
                    ForceRefreshSelectionContents();
                }
                break;
            case BUTTON_RIGHT:
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_LINE
struct LineTool : ToolBase
{
    LineTool() : ToolBase(TOOL_LINE)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            roundTo8Directions(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                Line(m_hdc, start.x, start.y, x, y, m_fg, toolsModel.GetLineWidth());
                break;
            case BUTTON_RIGHT:
                Line(m_hdc, start.x, start.y, x, y, m_bg, toolsModel.GetLineWidth());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            roundTo8Directions(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                Line(m_hdc, start.x, start.y, x, y, m_fg, toolsModel.GetLineWidth());
                break;
            case BUTTON_RIGHT:
                Line(m_hdc, start.x, start.y, x, y, m_bg, toolsModel.GetLineWidth());
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_BEZIER
struct BezierTool : ToolBase
{
    BezierTool() : ToolBase(TOOL_BEZIER)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if (pointSP == 0)
        {
            imageModel.CopyPrevious();
            pointSP++;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        switch (button)
        {
            case BUTTON_LEFT:
                switch (pointSP)
                {
                    case 1:
                        Line(m_hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, m_fg,
                             toolsModel.GetLineWidth());
                        break;
                    case 2:
                        Bezier(m_hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], m_fg, toolsModel.GetLineWidth());
                        break;
                    case 3:
                        Bezier(m_hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], m_fg, toolsModel.GetLineWidth());
                        break;
                }
                break;
            case BUTTON_RIGHT:
                switch (pointSP)
                {
                    case 1:
                        Line(m_hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, m_bg,
                             toolsModel.GetLineWidth());
                        break;
                    case 2:
                        Bezier(m_hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], m_bg, toolsModel.GetLineWidth());
                        break;
                    case 3:
                        Bezier(m_hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], m_bg, toolsModel.GetLineWidth());
                        break;
                }
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                pointSP++;
                if (pointSP == 4)
                    pointSP = 0;
                break;
            case BUTTON_RIGHT:
                pointSP++;
                if (pointSP == 4)
                    pointSP = 0;
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RECT
struct RectTool : ToolBase
{
    RectTool() : ToolBase(TOOL_RECT)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                Rect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
            case BUTTON_RIGHT:
                Rect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                Rect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
            case BUTTON_RIGHT:
                Rect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_SHAPE
struct ShapeTool : ToolBase
{
    ShapeTool() : ToolBase(TOOL_SHAPE)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        switch (button)
        {
            case BUTTON_LEFT:
                if (pointSP + 1 >= 2)
                    Poly(m_hdc, pointStack, pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                break;
            case BUTTON_RIGHT:
                if (pointSP + 1 >= 2)
                    Poly(m_hdc, pointStack, pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                break;
        }
        if (pointSP == 0)
        {
            imageModel.CopyPrevious();
            pointSP++;
        }
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                               pointStack[pointSP].x, pointStack[pointSP].y);
        switch (button)
        {
            case BUTTON_LEFT:
                if (pointSP + 1 >= 2)
                    Poly(m_hdc, pointStack, pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                break;
            case BUTTON_RIGHT:
                if (pointSP + 1 >= 2)
                    Poly(m_hdc, pointStack, pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                               pointStack[pointSP].x, pointStack[pointSP].y);
        pointSP++;
        switch (button)
        {
            case BUTTON_LEFT:
                if (pointSP >= 2)
                {
                    if (nearlyEqualPoints(x, y, pointStack[0].x, pointStack[0].y))
                    {
                        Poly(m_hdc, pointStack, pointSP, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), TRUE, FALSE);
                        pointSP = 0;
                    }
                    else
                    {
                        Poly(m_hdc, pointStack, pointSP, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                    }
                }
                if (pointSP == 255)
                    pointSP--;
                break;
            case BUTTON_RIGHT:
                if (pointSP >= 2)
                {
                    if (nearlyEqualPoints(x, y, pointStack[0].x, pointStack[0].y))
                    {
                        Poly(m_hdc, pointStack, pointSP, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), TRUE, FALSE);
                        pointSP = 0;
                    }
                    else
                    {
                        Poly(m_hdc, pointStack, pointSP, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
                    }
                }
                if (pointSP == 255)
                    pointSP--;
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_ELLIPSE
struct EllipseTool : ToolBase
{
    EllipseTool() : ToolBase(TOOL_ELLIPSE)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                Ellp(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
            case BUTTON_RIGHT:
                Ellp(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);

        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);

        switch (button)
        {
            case BUTTON_LEFT:
                Ellp(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
            case BUTTON_RIGHT:
                Ellp(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RRECT
struct RRectTool : ToolBase
{
    RRectTool() : ToolBase(TOOL_RRECT)
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(button, x, y, bDoubleClick);
    }
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                RRect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
            case BUTTON_RIGHT:
                RRect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
        }
        ToolBase::OnMove(button, x, y);
    }
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y)
    {
        ToolBase::OnUp(button, x, y);
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        switch (button)
        {
            case BUTTON_LEFT:
                RRect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
            case BUTTON_RIGHT:
                RRect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
                break;
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(BUTTON_LEFT, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

/*static*/ ToolBase*
ToolBase::createToolObject(TOOLTYPE type)
{
    switch (type)
    {
        case TOOL_FREESEL:  return new FreeSelTool();
        case TOOL_RECTSEL:  return new RectSelTool();
        case TOOL_RUBBER:   return new RubberTool();
        case TOOL_FILL:     return new FillTool();
        case TOOL_COLOR:    return new ColorTool();
        case TOOL_ZOOM:     return new ZoomTool();
        case TOOL_PEN:      return new PenTool();
        case TOOL_BRUSH:    return new BrushTool();
        case TOOL_AIRBRUSH: return new AirBrushTool();
        case TOOL_TEXT:     return new TextTool();
        case TOOL_LINE:     return new LineTool();
        case TOOL_BEZIER:   return new BezierTool();
        case TOOL_RECT:     return new RectTool();
        case TOOL_SHAPE:    return new ShapeTool();
        case TOOL_ELLIPSE:  return new EllipseTool();
        case TOOL_RRECT:    return new RRectTool();
    }
    UNREACHABLE;
    return NULL;
}
