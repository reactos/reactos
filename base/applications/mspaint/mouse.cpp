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

void ToolBase::OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
{
    start.x = last.x = x;
    start.y = last.y = y;
}

void ToolBase::OnMove(BOOL bLeftButton, LONG x, LONG y)
{
    last.x = x;
    last.y = y;
}

void ToolBase::OnUp(BOOL bLeftButton, LONG x, LONG y)
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
    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        selectionWindow.ShowWindow(SW_HIDE);
        selectionModel.ResetPtStack();
        selectionModel.PushToPtStack(x, y);
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        if (selectionModel.PtStackSize() == 1)
            imageModel.CopyPrevious();
        selectionModel.PushToPtStack(max(0, min(x, imageModel.GetWidth())), max(0, min(y, imageModel.GetHeight())));
        imageModel.ResetToPrevious();
        selectionModel.DrawFramePoly(m_hdc);
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
        {
            imageModel.CopyPrevious();
            selectionWindow.ShowWindow(SW_HIDE);
            selectionModel.SetSrcRectSizeToZero();
        }
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        POINT temp;
        if (bLeftButton)
        {
            imageModel.ResetToPrevious();
            temp.x = max(0, min(x, imageModel.GetWidth()));
            temp.y = max(0, min(y, imageModel.GetHeight()));
            selectionModel.SetSrcAndDestRectFromPoints(start, temp);
            RectSel(m_hdc, start.x, start.y, temp.x, temp.y);
        }
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        if (bLeftButton)
        {
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
            Erase(m_hdc, x, y, x, y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(m_hdc, x, y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        if (bLeftButton)
            Erase(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(m_hdc, last.x, last.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        if (bLeftButton)
            Erase(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(m_hdc, last.x, last.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
            Fill(m_hdc, x, y, m_fg);
        else
            Fill(m_hdc, x, y, m_bg);
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
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
    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        COLORREF tempColor = GetPixel(m_hdc, x, y);
        if (bLeftButton)
        {
            if (tempColor != CLR_INVALID)
                paletteModel.SetFgColor(tempColor);
        }
        else
        {
            if (tempColor != CLR_INVALID)
                paletteModel.SetBgColor(tempColor);
        }
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        COLORREF tempColor = GetPixel(m_hdc, x, y);
        if (bLeftButton)
        {
            if (tempColor != CLR_INVALID)
                paletteModel.SetFgColor(tempColor);
        }
        else
        {
            if (tempColor != CLR_INVALID)
                paletteModel.SetBgColor(tempColor);
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
        {
            if (toolsModel.GetZoom() < MAX_ZOOM)
                zoomTo(toolsModel.GetZoom() * 2, x, y);
        }
        else
        {
            if (toolsModel.GetZoom() > MIN_ZOOM)
                zoomTo(toolsModel.GetZoom() / 2, x, y);
        }
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
            SetPixel(m_hdc, x, y, m_fg);
        else
            SetPixel(m_hdc, x, y, m_bg);
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        if (bLeftButton)
            Line(m_hdc, last.x, last.y, x, y, m_fg, 1);
        else
            Line(m_hdc, last.x, last.y, x, y, m_bg, 1);
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        if (bLeftButton)
        {
            Line(m_hdc, last.x, last.y, x, y, m_fg, 1);
            SetPixel(m_hdc, x, y, m_fg);
        }
        else
        {
            Line(m_hdc, last.x, last.y, x, y, m_bg, 1);
            SetPixel(m_hdc, x, y, m_bg);
        }
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
            Brush(m_hdc, x, y, x, y, m_fg, toolsModel.GetBrushStyle());
        else
            Brush(m_hdc, x, y, x, y, m_bg, toolsModel.GetBrushStyle());
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        if (bLeftButton)
            Brush(m_hdc, last.x, last.y, x, y, m_fg, toolsModel.GetBrushStyle());
        else
            Brush(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetBrushStyle());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
        if (bLeftButton)
            Airbrush(m_hdc, x, y, m_fg, toolsModel.GetAirBrushWidth());
        else
            Airbrush(m_hdc, x, y, m_bg, toolsModel.GetAirBrushWidth());
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        if (bLeftButton)
            Airbrush(m_hdc, x, y, m_fg, toolsModel.GetAirBrushWidth());
        else
            Airbrush(m_hdc, x, y, m_bg, toolsModel.GetAirBrushWidth());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        POINT temp;
        if (bLeftButton)
        {
            imageModel.ResetToPrevious();
            temp.x = max(0, min(x, imageModel.GetWidth()));
            temp.y = max(0, min(y, imageModel.GetHeight()));
            selectionModel.SetSrcAndDestRectFromPoints(start, temp);
            RectSel(m_hdc, start.x, start.y, temp.x, temp.y);
        }
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        if (bLeftButton)
        {
            imageModel.ResetToPrevious();
            if (selectionModel.IsSrcRectSizeNonzero())
            {
                imageModel.CopyPrevious();

                placeSelWin();
                selectionWindow.ShowWindow(SW_SHOW);
                ForceRefreshSelectionContents();
            }
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            roundTo8Directions(start.x, start.y, x, y);
        if (bLeftButton)
            Line(m_hdc, start.x, start.y, x, y, m_fg, toolsModel.GetLineWidth());
        else
            Line(m_hdc, start.x, start.y, x, y, m_bg, toolsModel.GetLineWidth());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            roundTo8Directions(start.x, start.y, x, y);
        if (bLeftButton)
            Line(m_hdc, start.x, start.y, x, y, m_fg, toolsModel.GetLineWidth());
        else
            Line(m_hdc, start.x, start.y, x, y, m_bg, toolsModel.GetLineWidth());
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if (pointSP == 0)
        {
            imageModel.CopyPrevious();
            pointSP++;
        }
    }
    void drawLine(COLORREF rgb)
    {
        switch (pointSP)
        {
            case 1:
                Line(m_hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, rgb,
                     toolsModel.GetLineWidth());
                break;
            case 2:
                Bezier(m_hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], rgb, toolsModel.GetLineWidth());
                break;
            case 3:
                Bezier(m_hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], rgb, toolsModel.GetLineWidth());
                break;
        }
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if (bLeftButton)
            drawLine(m_fg);
        else
            drawLine(m_bg);
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        imageModel.ResetToPrevious();
        if (bLeftButton)
            drawLine(m_fg);
        else
            drawLine(m_bg);
        pointSP++;
        if (pointSP == 4)
            pointSP = 0;
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            Rect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Rect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            Rect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Rect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if (bLeftButton)
        {
            if (pointSP + 1 >= 2)
                Poly(m_hdc, pointStack, pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
        }
        else
        {
            if (pointSP + 1 >= 2)
                Poly(m_hdc, pointStack, pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
        }
        if (pointSP == 0)
        {
            imageModel.CopyPrevious();
            pointSP++;
        }
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                               pointStack[pointSP].x, pointStack[pointSP].y);
        if (bLeftButton)
        {
            if (pointSP + 1 >= 2)
                Poly(m_hdc, pointStack, pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
        }
        else
        {
            if (pointSP + 1 >= 2)
                Poly(m_hdc, pointStack, pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), FALSE, FALSE);
        }
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y,
                               pointStack[pointSP].x, pointStack[pointSP].y);
        pointSP++;
        if (bLeftButton)
        {
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
        }
        else
        {
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
        }
        if (pointSP == 255)
            pointSP--;
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            Ellp(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Ellp(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);

        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);

        if (bLeftButton)
            Ellp(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Ellp(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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

    virtual void OnDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        ToolBase::OnDown(bLeftButton, x, y, bDoubleClick);
        imageModel.CopyPrevious();
    }
    virtual void OnMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            RRect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            RRect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        ToolBase::OnMove(bLeftButton, x, y);
    }
    virtual void OnUp(BOOL bLeftButton, LONG x, LONG y)
    {
        ToolBase::OnUp(bLeftButton, x, y);
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            RRect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            RRect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
    virtual void OnCancelDraw()
    {
        OnUp(FALSE, 0, 0);
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
