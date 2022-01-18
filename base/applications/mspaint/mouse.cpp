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

INT ToolBase::pointSP = 0;
POINT ToolBase::pointStack[256] = { { 0 } };

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

void updateStartAndLast(LONG x, LONG y)
{
    start.x = last.x = x;
    start.y = last.y = y;
}

void updateLast(LONG x, LONG y)
{
    last.x = x;
    last.y = y;
}

void ToolBase::reset()
{
    pointSP = 0;
}

void ToolBase::OnCancelDraw()
{
    reset();
}

void ToolBase::beginEvent()
{
    m_hdc = imageModel.GetDC();
    m_fg = paletteModel.GetFgColor();
    m_bg = paletteModel.GetBgColor();
}

void ToolBase::endEvent()
{
    m_hdc = NULL;
}

/* TOOLS ********************************************************/

// TOOL_FREESEL
struct FreeSelTool : ToolBase
{
    FreeSelTool() : ToolBase(TOOL_FREESEL)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        imageModel.CopyPrevious();
        selectionWindow.ShowWindow(SW_HIDE);
        selectionModel.ResetPtStack();
        selectionModel.PushToPtStack(x, y);
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
    {
        if (selectionModel.PtStackSize() == 1)
            imageModel.CopyPrevious();
        selectionModel.PushToPtStack(max(0, min(x, imageModel.GetWidth())), max(0, min(y, imageModel.GetHeight())));
        imageModel.ResetToPrevious();
        selectionModel.DrawFramePoly(m_hdc);
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
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

    void OnCancelDraw()
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

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        imageModel.CopyPrevious();
        if (bLeftButton)
        {
            imageModel.CopyPrevious();
            selectionWindow.ShowWindow(SW_HIDE);
            selectionModel.SetSrcRectSizeToZero();
        }
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
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
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
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

    void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

struct GenericDrawTool : ToolBase
{
    GenericDrawTool(TOOLTYPE type) : ToolBase(type)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y) = 0;

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        imageModel.CopyPrevious();
        draw(bLeftButton, x, y);
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
    {
        draw(bLeftButton, x, y);
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
        draw(bLeftButton, x, y);
    }

    void OnCancelDraw()
    {
        OnButtonUp(FALSE, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RUBBER
struct RubberTool : GenericDrawTool
{
    RubberTool() : GenericDrawTool(TOOL_RUBBER)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        if (bLeftButton)
            Erase(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(m_hdc, last.x, last.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
    }
};

// TOOL_FILL
struct FillTool : ToolBase
{
    FillTool() : ToolBase(TOOL_FILL)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        imageModel.CopyPrevious();
        Fill(m_hdc, x, y, bLeftButton ? m_fg : m_bg);
    }
};

// TOOL_COLOR
struct ColorTool : ToolBase
{
    ColorTool() : ToolBase(TOOL_COLOR)
    {
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
        COLORREF tempColor;

        if (0 <= x && x < imageModel.GetWidth() && 0 <= y && y < imageModel.GetHeight())
            tempColor = GetPixel(m_hdc, x, y);
        else
            tempColor = RGB(255, 255, 255); // Outside is white

        if (bLeftButton)
            paletteModel.SetFgColor(tempColor);
        else
            paletteModel.SetBgColor(tempColor);

        toolsModel.SetActiveTool(toolsModel.GetOldActiveTool());
    }
};

// TOOL_ZOOM
struct ZoomTool : ToolBase
{
    ZoomTool() : ToolBase(TOOL_ZOOM)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
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
};

// TOOL_PEN
struct PenTool : GenericDrawTool
{
    PenTool() : GenericDrawTool(TOOL_PEN)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Line(m_hdc, last.x, last.y, x, y, rgb, 1);
        SetPixel(m_hdc, x, y, rgb);
    }
};

// TOOL_BRUSH
struct BrushTool : GenericDrawTool
{
    BrushTool() : GenericDrawTool(TOOL_BRUSH)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Brush(m_hdc, last.x, last.y, x, y, rgb, toolsModel.GetBrushStyle());
    }
};

// TOOL_AIRBRUSH
struct AirBrushTool : GenericDrawTool
{
    AirBrushTool() : GenericDrawTool(TOOL_AIRBRUSH)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Airbrush(m_hdc, x, y, rgb, toolsModel.GetAirBrushWidth());
    }
};

// TOOL_TEXT
struct TextTool : ToolBase
{
    TextTool() : ToolBase(TOOL_TEXT)
    {
    }

    void UpdatePoint(LONG x, LONG y)
    {
        POINT temp;
        imageModel.ResetToPrevious();
        temp.x = max(0, min(x, imageModel.GetWidth()));
        temp.y = max(0, min(y, imageModel.GetHeight()));
        selectionModel.SetSrcAndDestRectFromPoints(start, temp);
        RectSel(m_hdc, start.x, start.y, temp.x, temp.y);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        if (!textEditWindow.IsWindow())
            textEditWindow.Create(imageArea);

        imageModel.CopyPrevious();
        UpdatePoint(x, y);
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
    {
        UpdatePoint(x, y);
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();

        BOOL bTextBoxShown = textEditWindow.IsWindowVisible();
        if (bTextBoxShown && textEditWindow.GetWindowTextLength() > 0)
        {
            CString szText;
            textEditWindow.GetWindowText(szText);

            RECT rc;
            textEditWindow.InvalidateEditRect();
            textEditWindow.GetEditRect(&rc);

            INT style = (toolsModel.IsBackgroundTransparent() ? 0 : 1);
            Text(m_hdc, rc.left, rc.top, rc.right, rc.bottom, m_fg, m_bg, szText,
                 textEditWindow.GetFont(), style);
        }

        if (registrySettings.ShowTextTool)
        {
            if (!fontsDialog.IsWindow())
                fontsDialog.Create(mainWindow);

            fontsDialog.ShowWindow(SW_SHOWNOACTIVATE);
        }

        if (!bTextBoxShown || selectionModel.IsSrcRectSizeNonzero())
        {
            RECT rc;
            selectionModel.GetRect(&rc);

            // Enlarge if tool small
            INT cxMin = CX_MINTEXTEDIT, cyMin = CY_MINTEXTEDIT;
            if (selectionModel.IsSrcRectSizeNonzero())
            {
                if (rc.right - rc.left < cxMin)
                    rc.right = rc.left + cxMin;
                if (rc.bottom - rc.top < cyMin)
                    rc.bottom = rc.top + cyMin;
            }
            else
            {
                SetRect(&rc, x, y, x + cxMin, y + cyMin);
            }

            if (!textEditWindow.IsWindow())
                textEditWindow.Create(imageArea);

            textEditWindow.SetWindowText(NULL);
            textEditWindow.ValidateEditRect(&rc);
            textEditWindow.ShowWindow(SW_SHOWNOACTIVATE);
            textEditWindow.SetFocus();
        }
        else
        {
            textEditWindow.ShowWindow(SW_HIDE);
            textEditWindow.SetWindowText(NULL);
        }
    }

    void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        textEditWindow.SetWindowText(NULL);
        textEditWindow.ShowWindow(SW_HIDE);
        ToolBase::OnCancelDraw();
    }
};

// TOOL_LINE
struct LineTool : GenericDrawTool
{
    LineTool() : GenericDrawTool(TOOL_LINE)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            roundTo8Directions(start.x, start.y, x, y);
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Line(m_hdc, start.x, start.y, x, y, rgb, toolsModel.GetLineWidth());
    }
};

// TOOL_BEZIER
struct BezierTool : ToolBase
{
    BezierTool() : ToolBase(TOOL_BEZIER)
    {
    }

    void draw(BOOL bLeftButton)
    {
        COLORREF rgb = (bLeftButton ? m_fg : m_bg);
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

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;

        if (pointSP == 0)
        {
            imageModel.CopyPrevious();
            pointSP++;
        }
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        draw(bLeftButton);
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        draw(bLeftButton);
        pointSP++;
        if (pointSP == 4)
            pointSP = 0;
    }

    void OnCancelDraw()
    {
        OnButtonUp(FALSE, 0, 0);
        imageModel.Undo();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RECT
struct RectTool : GenericDrawTool
{
    RectTool() : GenericDrawTool(TOOL_RECT)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            Rect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Rect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
};

// TOOL_SHAPE
struct ShapeTool : ToolBase
{
    ShapeTool() : ToolBase(TOOL_SHAPE)
    {
    }

    void draw(BOOL bLeftButton, LONG x, LONG y, BOOL bClosed = FALSE)
    {
        if (pointSP + 1 >= 2)
        {
            if (bLeftButton)
                Poly(m_hdc, pointStack, pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), bClosed, FALSE);
            else
                Poly(m_hdc, pointStack, pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), bClosed, FALSE);
        }
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
    {
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;

        if (pointSP == 0 && !bDoubleClick)
        {
            imageModel.CopyPrevious();
            draw(bLeftButton, x, y);
            pointSP++;
        }
        else
        {
            draw(bLeftButton, x, y, bDoubleClick);
        }
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y, x, y);
        draw(bLeftButton, x, y, FALSE);
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y, x, y);

        if (nearlyEqualPoints(x, y, pointStack[0].x, pointStack[0].y))
        {
            pointSP--;
            draw(bLeftButton, x, y, TRUE);
            pointSP = 0;
        }
        else
        {
            pointSP++;
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
            draw(bLeftButton, x, y, FALSE);
        }

        if (pointSP == _countof(pointStack))
            pointSP--;
    }

    void OnCancelDraw()
    {
        imageModel.ResetToPrevious();
        selectionModel.ResetPtStack();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_ELLIPSE
struct EllipseTool : GenericDrawTool
{
    EllipseTool() : GenericDrawTool(TOOL_ELLIPSE)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            Ellp(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Ellp(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
};

// TOOL_RRECT
struct RRectTool : GenericDrawTool
{
    RRectTool() : GenericDrawTool(TOOL_RRECT)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y)
    {
        imageModel.ResetToPrevious();
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, x, y);
        if (bLeftButton)
            RRect(m_hdc, start.x, start.y, x, y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            RRect(m_hdc, start.x, start.y, x, y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
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
