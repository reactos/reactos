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
    start.x = start.y = last.x = last.y = -1;
    selectionModel.ResetPtStack();
    if (selectionModel.m_bShow)
    {
        selectionModel.Landing();
        selectionModel.m_bShow = FALSE;
    }
}

void ToolBase::OnCancelDraw()
{
    reset();
    imageModel.NotifyImageChanged();
}

void ToolBase::OnFinishDraw()
{
    reset();
    imageModel.NotifyImageChanged();
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

void ToolBase::OnDrawSelectionOnCanvas(HDC hdc)
{
    if (!selectionModel.m_bShow)
        return;

    RECT rcSelection = selectionModel.m_rc;
    canvasWindow.ImageToCanvas(rcSelection);

    ::InflateRect(&rcSelection, GRIP_SIZE, GRIP_SIZE);
    drawSizeBoxes(hdc, &rcSelection, TRUE);
}

/* TOOLS ********************************************************/

// TOOL_FREESEL
struct FreeSelTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;

    FreeSelTool() : ToolBase(TOOL_FREESEL)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (!selectionModel.IsLanded())
        {
            selectionModel.DrawBackgroundPoly(hdc, selectionModel.m_rgbBack);
            selectionModel.DrawSelection(hdc, paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());
        }

        if (canvasWindow.m_drawing)
        {
            selectionModel.DrawFramePoly(hdc);
        }
    }

    void OnDrawOverlayOnCanvas(HDC hdc) override
    {
        OnDrawSelectionOnCanvas(hdc);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        selectionModel.Landing();
        if (bLeftButton)
        {
            selectionModel.m_bShow = FALSE;
            selectionModel.ResetPtStack();
            POINT pt = { x, y };
            selectionModel.PushToPtStack(pt);
        }
        m_bLeftButton = bLeftButton;
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        if (bLeftButton)
        {
            POINT pt = { x, y };
            imageModel.Bound(pt);
            selectionModel.PushToPtStack(pt);
            imageModel.NotifyImageChanged();
        }
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        if (bLeftButton)
        {
            if (selectionModel.PtStackSize() > 2)
            {
                selectionModel.BuildMaskFromPtStack();
                selectionModel.m_bShow = TRUE;
            }
            else
            {
                selectionModel.ResetPtStack();
                selectionModel.m_bShow = FALSE;
            }
            imageModel.NotifyImageChanged();
        }
    }

    void OnFinishDraw() override
    {
        m_bLeftButton = FALSE;
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        m_bLeftButton = FALSE;
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RECTSEL
struct RectSelTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;

    RectSelTool() : ToolBase(TOOL_RECTSEL)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (!selectionModel.IsLanded())
        {
            selectionModel.DrawBackgroundRect(hdc, selectionModel.m_rgbBack);
            selectionModel.DrawSelection(hdc, paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());
        }

        if (canvasWindow.m_drawing)
        {
            RECT rc = selectionModel.m_rc;
            if (!::IsRectEmpty(&rc))
                RectSel(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    }

    void OnDrawOverlayOnCanvas(HDC hdc) override
    {
        OnDrawSelectionOnCanvas(hdc);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        selectionModel.Landing();
        if (bLeftButton)
        {
            selectionModel.m_bShow = FALSE;
            ::SetRectEmpty(&selectionModel.m_rc);
        }
        m_bLeftButton = bLeftButton;
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        if (bLeftButton)
        {
            POINT pt = { x, y };
            imageModel.Bound(pt);
            selectionModel.SetRectFromPoints(start, pt);
            imageModel.NotifyImageChanged();
        }
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        if (bLeftButton)
        {
            POINT pt = { x, y };
            imageModel.Bound(pt);
            selectionModel.SetRectFromPoints(start, pt);
            selectionModel.m_bShow = !selectionModel.m_rc.IsRectEmpty();
            imageModel.NotifyImageChanged();
        }
    }

    void OnFinishDraw() override
    {
        m_bLeftButton = FALSE;
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        m_bLeftButton = FALSE;
        selectionModel.m_bShow = FALSE;
        ToolBase::OnCancelDraw();
    }
};

struct TwoPointDrawTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;
    BOOL m_bDrawing = FALSE;

    TwoPointDrawTool(TOOLTYPE type) : ToolBase(type)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;
        m_bDrawing = TRUE;
        start.x = last.x = x;
        start.y = last.y = y;
        imageModel.NotifyImageChanged();
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        last.x = x;
        last.y = y;
        imageModel.NotifyImageChanged();
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        last.x = x;
        last.y = y;
        imageModel.PushImageForUndo();
        OnDrawOverlayOnImage(m_hdc);
        m_bDrawing = FALSE;
        imageModel.NotifyImageChanged();
    }

    void OnFinishDraw() override
    {
        m_bDrawing = FALSE;
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        m_bLeftButton = FALSE;
        m_bDrawing = FALSE;
        ToolBase::OnCancelDraw();
    }
};

struct SmoothDrawTool : ToolBase
{
    SmoothDrawTool(TOOLTYPE type) : ToolBase(type)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y) = 0;

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        imageModel.PushImageForUndo();
        start.x = last.x = x;
        start.y = last.y = y;
        imageModel.NotifyImageChanged();
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        draw(bLeftButton, x, y);
        imageModel.NotifyImageChanged();
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        draw(bLeftButton, x, y);
        OnFinishDraw();
    }

    void OnFinishDraw() override
    {
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        OnButtonUp(FALSE, 0, 0);
        imageModel.Undo(TRUE);
        ToolBase::OnCancelDraw();
    }
};

// TOOL_RUBBER
struct RubberTool : SmoothDrawTool
{
    RubberTool() : SmoothDrawTool(TOOL_RUBBER)
    {
    }

    void draw(BOOL bLeftButton, LONG x, LONG y) override
    {
        if (bLeftButton)
            Erase(m_hdc, last.x, last.y, x, y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(m_hdc, last.x, last.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
        last.x = x;
        last.y = y;
    }
};

// TOOL_FILL
struct FillTool : ToolBase
{
    FillTool() : ToolBase(TOOL_FILL)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        imageModel.PushImageForUndo();
        Fill(m_hdc, x, y, bLeftButton ? m_fg : m_bg);
    }
};

// TOOL_COLOR
struct ColorTool : ToolBase
{
    ColorTool() : ToolBase(TOOL_COLOR)
    {
    }

    void fetchColor(BOOL bLeftButton, LONG x, LONG y)
    {
        COLORREF rgbColor;

        if (0 <= x && x < imageModel.GetWidth() && 0 <= y && y < imageModel.GetHeight())
            rgbColor = GetPixel(m_hdc, x, y);
        else
            rgbColor = RGB(255, 255, 255); // Outside is white

        if (bLeftButton)
            paletteModel.SetFgColor(rgbColor);
        else
            paletteModel.SetBgColor(rgbColor);
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        fetchColor(bLeftButton, x, y);
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        fetchColor(bLeftButton, x, y);
        toolsModel.SetActiveTool(toolsModel.GetOldActiveTool());
    }
};

// TOOL_ZOOM
struct ZoomTool : ToolBase
{
    ZoomTool() : ToolBase(TOOL_ZOOM)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        imageModel.PushImageForUndo();
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
struct PenTool : SmoothDrawTool
{
    PenTool() : SmoothDrawTool(TOOL_PEN)
    {
    }

    void draw(BOOL bLeftButton, LONG x, LONG y) override
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Line(m_hdc, last.x, last.y, x, y, rgb, 1);
        ::SetPixelV(m_hdc, x, y, rgb);
        last.x = x;
        last.y = y;
    }
};

// TOOL_BRUSH
struct BrushTool : SmoothDrawTool
{
    BrushTool() : SmoothDrawTool(TOOL_BRUSH)
    {
    }

    void draw(BOOL bLeftButton, LONG x, LONG y) override
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Brush(m_hdc, last.x, last.y, x, y, rgb, toolsModel.GetBrushStyle());
        last.x = x;
        last.y = y;
    }
};

// TOOL_AIRBRUSH
struct AirBrushTool : SmoothDrawTool
{
    AirBrushTool() : SmoothDrawTool(TOOL_AIRBRUSH)
    {
    }

    void draw(BOOL bLeftButton, LONG x, LONG y) override
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

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (canvasWindow.m_drawing)
        {
            RECT rc = selectionModel.m_rc;
            if (!::IsRectEmpty(&rc))
                RectSel(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    }

    void UpdatePoint(LONG x, LONG y)
    {
        POINT pt = { x, y };
        imageModel.Bound(pt);
        selectionModel.SetRectFromPoints(start, pt);
        imageModel.NotifyImageChanged();
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        if (!textEditWindow.IsWindow())
            textEditWindow.Create(canvasWindow);

        UpdatePoint(x, y);
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        UpdatePoint(x, y);
    }

    void draw(HDC hdc)
    {
        CString szText;
        textEditWindow.GetWindowText(szText);

        RECT rc;
        textEditWindow.InvalidateEditRect();
        textEditWindow.GetEditRect(&rc);
        ::InflateRect(&rc, -GRIP_SIZE / 2, -GRIP_SIZE / 2);

        // Draw the text
        INT style = (toolsModel.IsBackgroundTransparent() ? 0 : 1);
        imageModel.PushImageForUndo();
        Text(hdc, rc.left, rc.top, rc.right, rc.bottom, m_fg, m_bg, szText,
             textEditWindow.GetFont(), style);
    }

    void quit()
    {
        if (textEditWindow.IsWindow())
            textEditWindow.ShowWindow(SW_HIDE);
        ::SetRectEmpty(selectionModel.m_rc);
        ::SetRectEmpty(selectionModel.m_rcOld);
        selectionModel.m_bShow = FALSE;
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        POINT pt = { x, y };
        imageModel.Bound(pt);
        selectionModel.SetRectFromPoints(start, pt);

        BOOL bTextBoxShown = ::IsWindowVisible(textEditWindow);
        if (bTextBoxShown && textEditWindow.GetWindowTextLength() > 0)
        {
            draw(m_hdc);

            if (selectionModel.m_rc.IsRectEmpty())
            {
                textEditWindow.ShowWindow(SW_HIDE);
                textEditWindow.SetWindowText(NULL);
                return;
            }
        }

        if (registrySettings.ShowTextTool)
        {
            if (!fontsDialog.IsWindow())
                fontsDialog.Create(mainWindow);

            fontsDialog.ShowWindow(SW_SHOWNOACTIVATE);
        }

        RECT rc = selectionModel.m_rc;

        // Enlarge if tool small
        INT cxMin = CX_MINTEXTEDIT, cyMin = CY_MINTEXTEDIT;
        if (selectionModel.m_rc.IsRectEmpty())
        {
            SetRect(&rc, x, y, x + cxMin, y + cyMin);
        }
        else
        {
            if (rc.right - rc.left < cxMin)
                rc.right = rc.left + cxMin;
            if (rc.bottom - rc.top < cyMin)
                rc.bottom = rc.top + cyMin;
        }

        if (!textEditWindow.IsWindow())
            textEditWindow.Create(canvasWindow);

        textEditWindow.SetWindowText(NULL);
        textEditWindow.ValidateEditRect(&rc);
        textEditWindow.ShowWindow(SW_SHOWNOACTIVATE);
        textEditWindow.SetFocus();
    }

    void OnFinishDraw() override
    {
        draw(m_hdc);
        quit();
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        quit();
        ToolBase::OnCancelDraw();
    }
};

// TOOL_LINE
struct LineTool : TwoPointDrawTool
{
    LineTool() : TwoPointDrawTool(TOOL_LINE)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (!m_bDrawing)
            return;
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            roundTo8Directions(start.x, start.y, last.x, last.y);
        COLORREF rgb = m_bLeftButton ? m_fg : m_bg;
        Line(hdc, start.x, start.y, last.x, last.y, rgb, toolsModel.GetLineWidth());
    }
};

// TOOL_BEZIER
struct BezierTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;
    BOOL m_bDrawing = FALSE;

    BezierTool() : ToolBase(TOOL_BEZIER)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc)
    {
        if (!m_bDrawing)
            return;

        COLORREF rgb = (m_bLeftButton ? m_fg : m_bg);
        switch (pointSP)
        {
            case 1:
                Line(hdc, pointStack[0].x, pointStack[0].y, pointStack[1].x, pointStack[1].y, rgb,
                     toolsModel.GetLineWidth());
                break;
            case 2:
                Bezier(hdc, pointStack[0], pointStack[2], pointStack[2], pointStack[1], rgb, toolsModel.GetLineWidth());
                break;
            case 3:
                Bezier(hdc, pointStack[0], pointStack[2], pointStack[3], pointStack[1], rgb, toolsModel.GetLineWidth());
                break;
        }
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;

        if (!m_bDrawing)
        {
            m_bDrawing = TRUE;
            pointStack[pointSP].x = pointStack[pointSP + 1].x = x;
            pointStack[pointSP].y = pointStack[pointSP + 1].y = y;
            ++pointSP;
        }
        else
        {
            ++pointSP;
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
        }

        imageModel.NotifyImageChanged();
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        imageModel.NotifyImageChanged();
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;
        if (pointSP >= 3)
        {
            OnFinishDraw();
            return;
        }
        imageModel.NotifyImageChanged();
    }

    void OnCancelDraw() override
    {
        m_bDrawing = FALSE;
        ToolBase::OnCancelDraw();
    }

    void OnFinishDraw() override
    {
        imageModel.PushImageForUndo();
        OnDrawOverlayOnImage(m_hdc);
        m_bDrawing = FALSE;
        ToolBase::OnFinishDraw();
    }
};

// TOOL_RECT
struct RectTool : TwoPointDrawTool
{
    RectTool() : TwoPointDrawTool(TOOL_RECT)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (!m_bDrawing)
            return;
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, last.x, last.y);
        if (m_bLeftButton)
            Rect(hdc, start.x, start.y, last.x, last.y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Rect(hdc, start.x, start.y, last.x, last.y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
};

// TOOL_SHAPE
struct ShapeTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;
    BOOL m_bClosed = FALSE;

    ShapeTool() : ToolBase(TOOL_SHAPE)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc)
    {
        if (pointSP <= 0)
            return;

        if (m_bLeftButton)
            Poly(hdc, pointStack, pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), m_bClosed, FALSE);
        else
            Poly(hdc, pointStack, pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), m_bClosed, FALSE);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;
        m_bClosed = FALSE;

        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;

        if (pointSP && bDoubleClick)
        {
            OnFinishDraw();
            return;
        }

        if (pointSP == 0)
        {
            pointSP++;
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
        }

        imageModel.NotifyImageChanged();
    }

    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y) override
    {
        pointStack[pointSP].x = x;
        pointStack[pointSP].y = y;

        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y, x, y);

        imageModel.NotifyImageChanged();
    }

    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y) override
    {
        if ((pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(pointStack[pointSP - 1].x, pointStack[pointSP - 1].y, x, y);

        m_bClosed = FALSE;
        if (nearlyEqualPoints(x, y, pointStack[0].x, pointStack[0].y))
        {
            OnFinishDraw();
            return;
        }
        else
        {
            pointSP++;
            pointStack[pointSP].x = x;
            pointStack[pointSP].y = y;
        }

        if (pointSP == _countof(pointStack))
            pointSP--;

        imageModel.NotifyImageChanged();
    }

    void OnCancelDraw() override
    {
        ToolBase::OnCancelDraw();
    }

    void OnFinishDraw() override
    {
        if (pointSP)
        {
            --pointSP;
            m_bClosed = TRUE;

            imageModel.PushImageForUndo();
            OnDrawOverlayOnImage(m_hdc);
        }

        m_bClosed = FALSE;
        pointSP = 0;

        ToolBase::OnFinishDraw();
    }
};

// TOOL_ELLIPSE
struct EllipseTool : TwoPointDrawTool
{
    EllipseTool() : TwoPointDrawTool(TOOL_ELLIPSE)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (!m_bDrawing)
            return;
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, last.x, last.y);
        if (m_bLeftButton)
            Ellp(hdc, start.x, start.y, last.x, last.y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Ellp(hdc, start.x, start.y, last.x, last.y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
    }
};

// TOOL_RRECT
struct RRectTool : TwoPointDrawTool
{
    RRectTool() : TwoPointDrawTool(TOOL_RRECT)
    {
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (!m_bDrawing)
            return;
        if (GetAsyncKeyState(VK_SHIFT) < 0)
            regularize(start.x, start.y, last.x, last.y);
        if (m_bLeftButton)
            RRect(hdc, start.x, start.y, last.x, last.y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            RRect(hdc, start.x, start.y, last.x, last.y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
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
