/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Things which should not be in the mouse event handler itself
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

INT ToolBase::s_pointSP = 0;
POINT ToolBase::s_pointStack[256] = { { 0 } };

/* FUNCTIONS ********************************************************/

void
regularize(LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    if (labs(x1 - x0) >= labs(y1 - y0))
        y1 = y0 + (y1 > y0 ? labs(x1 - x0) : -labs(x1 - x0));
    else
        x1 = x0 + (x1 > x0 ? labs(y1 - y0) : -labs(y1 - y0));
}

void
roundTo8Directions(LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    if (labs(x1 - x0) >= labs(y1 - y0))
    {
        if (labs(y1 - y0) * 5 < labs(x1 - x0) * 2)
            y1 = y0;
        else
            y1 = y0 + (y1 > y0 ? labs(x1 - x0) : -labs(x1 - x0));
    }
    else
    {
        if (labs(x1 - x0) * 5 < labs(y1 - y0) * 2)
            x1 = x0;
        else
            x1 = x0 + (x1 > x0 ? labs(y1 - y0) : -labs(y1 - y0));
    }
}

BOOL nearlyEqualPoints(INT x0, INT y0, INT x1, INT y1)
{
    INT cxThreshold = toolsModel.GetLineWidth() + UnZoomed(GetSystemMetrics(SM_CXDRAG));
    INT cyThreshold = toolsModel.GetLineWidth() + UnZoomed(GetSystemMetrics(SM_CYDRAG));
    return (abs(x1 - x0) <= cxThreshold) && (abs(y1 - y0) <= cyThreshold);
}

void ToolBase::reset()
{
    s_pointSP = 0;
    g_ptStart.x = g_ptStart.y = g_ptEnd.x = g_ptEnd.y = -1;
    selectionModel.ResetPtStack();
    if (selectionModel.m_bShow)
    {
        selectionModel.Landing();
        selectionModel.HideSelection();
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
            selectionModel.DrawSelection(hdc, paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());

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
            selectionModel.HideSelection();
            selectionModel.ResetPtStack();
            POINT pt = { x, y };
            selectionModel.PushToPtStack(pt);
        }
        m_bLeftButton = bLeftButton;
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (bLeftButton)
        {
            POINT pt = { x, y };
            imageModel.Clamp(pt);
            selectionModel.PushToPtStack(pt);
            imageModel.NotifyImageChanged();
        }
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
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
        else
        {
            POINT pt = { x, y };
            canvasWindow.ClientToScreen(&pt);
            mainWindow.TrackPopupMenu(pt, 0);
        }
        return TRUE;
    }

    void OnFinishDraw() override
    {
        selectionModel.Landing();
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        selectionModel.HideSelection();
        ToolBase::OnCancelDraw();
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        selectionModel.StretchSelection(bMinus);
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
            selectionModel.DrawSelection(hdc, paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());

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
            selectionModel.HideSelection();
        }
        m_bLeftButton = bLeftButton;
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (bLeftButton)
        {
            POINT pt = { x, y };
            imageModel.Clamp(pt);
            selectionModel.SetRectFromPoints(g_ptStart, pt);
            imageModel.NotifyImageChanged();
        }
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        POINT pt = { x, y };
        if (bLeftButton)
        {
            imageModel.Clamp(pt);
            selectionModel.SetRectFromPoints(g_ptStart, pt);
            selectionModel.m_bShow = !selectionModel.m_rc.IsRectEmpty();
            imageModel.NotifyImageChanged();
        }
        else
        {
            canvasWindow.ClientToScreen(&pt);
            mainWindow.TrackPopupMenu(pt, 0);
        }
        return TRUE;
    }

    void OnFinishDraw() override
    {
        selectionModel.Landing();
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        selectionModel.HideSelection();
        ToolBase::OnCancelDraw();
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        selectionModel.StretchSelection(bMinus);
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
        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        imageModel.PushImageForUndo();
        OnDrawOverlayOnImage(m_hdc);
        m_bDrawing = FALSE;
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnFinishDraw() override
    {
        m_bDrawing = FALSE;
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        m_bDrawing = FALSE;
        ToolBase::OnCancelDraw();
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakeLineThickerOrThinner(bMinus);
    }
};

typedef enum DIRECTION
{
    NO_DIRECTION = -1,
    DIRECTION_HORIZONTAL,
    DIRECTION_VERTICAL,
    DIRECTION_DIAGONAL_RIGHT_DOWN,
    DIRECTION_DIAGONAL_RIGHT_UP,
} DIRECTION;

#define THRESHOULD_DEG 15

static DIRECTION
GetDirection(LONG x0, LONG y0, LONG x1, LONG y1)
{
    LONG dx = x1 - x0, dy = y1 - y0;

    if (labs(dx) <= 8 && labs(dy) <= 8)
        return NO_DIRECTION;

    double radian = atan2((double)dy, (double)dx);
    if (radian < DEG2RAD(-180 + THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_HORIZONTAL: %ld\n", RAD2DEG(radian));
        return DIRECTION_HORIZONTAL;
    }
    if (radian < DEG2RAD(-90 - THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_DIAGONAL_RIGHT_DOWN: %ld\n", RAD2DEG(radian));
        return DIRECTION_DIAGONAL_RIGHT_DOWN;
    }
    if (radian < DEG2RAD(-90 + THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_VERTICAL: %ld\n", RAD2DEG(radian));
        return DIRECTION_VERTICAL;
    }
    if (radian < DEG2RAD(-THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_DIAGONAL_RIGHT_UP: %ld\n", RAD2DEG(radian));
        return DIRECTION_DIAGONAL_RIGHT_UP;
    }
    if (radian < DEG2RAD(+THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_HORIZONTAL: %ld\n", RAD2DEG(radian));
        return DIRECTION_HORIZONTAL;
    }
    if (radian < DEG2RAD(+90 - THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_DIAGONAL_RIGHT_DOWN: %ld\n", RAD2DEG(radian));
        return DIRECTION_DIAGONAL_RIGHT_DOWN;
    }
    if (radian < DEG2RAD(+90 + THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_VERTICAL: %ld\n", RAD2DEG(radian));
        return DIRECTION_VERTICAL;
    }
    if (radian < DEG2RAD(+180 - THRESHOULD_DEG))
    {
        ATLTRACE("DIRECTION_DIAGONAL_RIGHT_UP: %ld\n", RAD2DEG(radian));
        return DIRECTION_DIAGONAL_RIGHT_UP;
    }
    ATLTRACE("DIRECTION_HORIZONTAL: %ld\n", RAD2DEG(radian));
    return DIRECTION_HORIZONTAL;
}

static void
RestrictDrawDirection(DIRECTION dir, LONG x0, LONG y0, LONG& x1, LONG& y1)
{
    switch (dir)
    {
        case NO_DIRECTION:
        default:
            return;

        case DIRECTION_HORIZONTAL:
            y1 = y0;
            break;

        case DIRECTION_VERTICAL:
            x1 = x0;
            break;

        case DIRECTION_DIAGONAL_RIGHT_DOWN:
            y1 = y0 + (x1 - x0);
            break;

        case DIRECTION_DIAGONAL_RIGHT_UP:
            x1 = x0 - (y1 - y0);
            break;
    }
}

struct SmoothDrawTool : ToolBase
{
    DIRECTION m_direction = NO_DIRECTION;
    BOOL m_bShiftDown = FALSE;

    SmoothDrawTool(TOOLTYPE type) : ToolBase(type)
    {
    }

    virtual void draw(BOOL bLeftButton, LONG x, LONG y) = 0;

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_direction = NO_DIRECTION;
        imageModel.PushImageForUndo();
        imageModel.NotifyImageChanged();
        m_bShiftDown = (::GetKeyState(VK_SHIFT) & 0x8000); // Is Shift key pressed?
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (m_bShiftDown)
        {
            if (m_direction == NO_DIRECTION)
            {
                m_direction = GetDirection(g_ptStart.x, g_ptStart.y, x, y);
                if (m_direction == NO_DIRECTION)
                    return FALSE;
            }

            RestrictDrawDirection(m_direction, g_ptStart.x, g_ptStart.y, x, y);
        }
        else
        {
            draw(bLeftButton, x, y);
            g_ptStart.x = g_ptEnd.x = x;
            g_ptStart.y = g_ptEnd.y = y;
            return TRUE;
        }

        draw(bLeftButton, x, y);
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (m_bShiftDown && m_direction != NO_DIRECTION)
        {
            RestrictDrawDirection(m_direction, g_ptStart.x, g_ptStart.y, x, y);
        }

        draw(bLeftButton, x, y);
        OnFinishDraw();
        return TRUE;
    }

    void OnFinishDraw() override
    {
        ToolBase::OnFinishDraw();
    }

    void OnCancelDraw() override
    {
        LONG x = 0, y = 0;
        OnButtonUp(FALSE, x, y);
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
            Erase(m_hdc, g_ptEnd.x, g_ptEnd.y, x, y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(m_hdc, g_ptEnd.x, g_ptEnd.y, x, y, m_fg, m_bg, toolsModel.GetRubberRadius());
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakeRubberThickerOrThinner(bMinus);
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

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        fetchColor(bLeftButton, x, y);
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        fetchColor(bLeftButton, x, y);
        toolsModel.SetActiveTool(toolsModel.GetOldActiveTool());
        return TRUE;
    }
};

// TOOL_ZOOM
struct ZoomTool : ToolBase
{
    BOOL m_bZoomed = FALSE;

    ZoomTool() : ToolBase(TOOL_ZOOM)
    {
    }

    BOOL getNewZoomRect(CRect& rcView, INT newZoom);

    void OnDrawOverlayOnCanvas(HDC hdc) override
    {
        CRect rcView;
        INT oldZoom = toolsModel.GetZoom();
        if (oldZoom < MAX_ZOOM && getNewZoomRect(rcView, oldZoom * 2))
            DrawXorRect(hdc, &rcView);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        INT newZoom, oldZoom = toolsModel.GetZoom();
        if (bLeftButton)
            newZoom = (oldZoom < MAX_ZOOM) ? (oldZoom * 2) : MIN_ZOOM;
        else
            newZoom = (oldZoom > MIN_ZOOM) ? (oldZoom / 2) : MAX_ZOOM;

        m_bZoomed = FALSE;

        if (oldZoom != newZoom)
        {
            CRect rcView;
            if (getNewZoomRect(rcView, newZoom))
            {
                canvasWindow.zoomTo(newZoom, rcView.left, rcView.top);
                m_bZoomed = TRUE;
            }
        }
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (m_bZoomed)
            toolsModel.SetActiveTool(toolsModel.GetOldActiveTool());

        return TRUE;
    }
};

BOOL ZoomTool::getNewZoomRect(CRect& rcView, INT newZoom)
{
    CPoint pt;
    ::GetCursorPos(&pt);
    canvasWindow.ScreenToClient(&pt);

    canvasWindow.getNewZoomRect(rcView, newZoom, pt);

    CRect rc;
    canvasWindow.GetImageRect(rc);
    canvasWindow.ImageToCanvas(rc);

    return rc.PtInRect(pt);
}

// TOOL_PEN
struct PenTool : SmoothDrawTool
{
    PenTool() : SmoothDrawTool(TOOL_PEN)
    {
    }

    void draw(BOOL bLeftButton, LONG x, LONG y) override
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Line(m_hdc, g_ptEnd.x, g_ptEnd.y, x, y, rgb, toolsModel.GetPenWidth());
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakePenThickerOrThinner(bMinus);
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
        Brush(m_hdc, g_ptEnd.x, g_ptEnd.y, x, y, rgb, toolsModel.GetBrushStyle(),
              toolsModel.GetBrushWidth());
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakeBrushThickerOrThinner(bMinus);
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

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakeAirBrushThickerOrThinner(bMinus);
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
        imageModel.Clamp(pt);
        selectionModel.SetRectFromPoints(g_ptStart, pt);
        imageModel.NotifyImageChanged();
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        if (!textEditWindow.IsWindow())
            textEditWindow.Create(canvasWindow);

        UpdatePoint(x, y);
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        UpdatePoint(x, y);
        return TRUE;
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
        Text(hdc, rc.left, rc.top, rc.right, rc.bottom, m_fg, m_bg, szText,
             textEditWindow.GetFont(), style);
    }

    void quit()
    {
        if (textEditWindow.IsWindow())
            textEditWindow.ShowWindow(SW_HIDE);
        selectionModel.HideSelection();
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        POINT pt = { x, y };
        imageModel.Clamp(pt);
        selectionModel.SetRectFromPoints(g_ptStart, pt);

        BOOL bTextBoxShown = ::IsWindowVisible(textEditWindow);
        if (bTextBoxShown)
        {
            if (textEditWindow.GetWindowTextLength() > 0)
            {
                imageModel.PushImageForUndo();
                draw(m_hdc);
            }
            if (::IsRectEmpty(&selectionModel.m_rc))
            {
                quit();
                return TRUE;
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
        return TRUE;
    }

    void OnFinishDraw() override
    {
        if (::IsWindowVisible(textEditWindow) &&
            textEditWindow.GetWindowTextLength() > 0)
        {
            imageModel.PushImageForUndo();
            draw(m_hdc);
        }
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
            roundTo8Directions(g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y);
        COLORREF rgb = m_bLeftButton ? m_fg : m_bg;
        Line(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, rgb, toolsModel.GetLineWidth());
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
        switch (s_pointSP)
        {
            case 1:
                Line(hdc, s_pointStack[0].x, s_pointStack[0].y, s_pointStack[1].x, s_pointStack[1].y, rgb,
                     toolsModel.GetLineWidth());
                break;
            case 2:
                Bezier(hdc, s_pointStack[0], s_pointStack[2], s_pointStack[2], s_pointStack[1], rgb, toolsModel.GetLineWidth());
                break;
            case 3:
                Bezier(hdc, s_pointStack[0], s_pointStack[2], s_pointStack[3], s_pointStack[1], rgb, toolsModel.GetLineWidth());
                break;
        }
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;

        if (!m_bDrawing)
        {
            m_bDrawing = TRUE;
            s_pointStack[s_pointSP].x = s_pointStack[s_pointSP + 1].x = x;
            s_pointStack[s_pointSP].y = s_pointStack[s_pointSP + 1].y = y;
            ++s_pointSP;
        }
        else
        {
            ++s_pointSP;
            s_pointStack[s_pointSP].x = x;
            s_pointStack[s_pointSP].y = y;
        }

        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        s_pointStack[s_pointSP].x = x;
        s_pointStack[s_pointSP].y = y;
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        s_pointStack[s_pointSP].x = x;
        s_pointStack[s_pointSP].y = y;
        if (s_pointSP >= 3)
        {
            OnFinishDraw();
            return TRUE;
        }
        imageModel.NotifyImageChanged();
        return TRUE;
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

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakeLineThickerOrThinner(bMinus);
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
            regularize(g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y);
        if (m_bLeftButton)
            Rect(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Rect(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
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
        if (s_pointSP <= 0)
            return;

        if (m_bLeftButton)
            Poly(hdc, s_pointStack, s_pointSP + 1, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), m_bClosed, FALSE);
        else
            Poly(hdc, s_pointStack, s_pointSP + 1, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), m_bClosed, FALSE);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;
        m_bClosed = FALSE;

        if ((s_pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(s_pointStack[s_pointSP - 1].x, s_pointStack[s_pointSP - 1].y, x, y);

        s_pointStack[s_pointSP].x = x;
        s_pointStack[s_pointSP].y = y;

        if (s_pointSP && bDoubleClick)
        {
            OnFinishDraw();
            return;
        }

        if (s_pointSP == 0)
        {
            s_pointSP++;
            s_pointStack[s_pointSP].x = x;
            s_pointStack[s_pointSP].y = y;
        }

        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if ((s_pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(s_pointStack[s_pointSP - 1].x, s_pointStack[s_pointSP - 1].y, x, y);

        s_pointStack[s_pointSP].x = x;
        s_pointStack[s_pointSP].y = y;

        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if ((s_pointSP > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(s_pointStack[s_pointSP - 1].x, s_pointStack[s_pointSP - 1].y, x, y);

        m_bClosed = FALSE;
        if (nearlyEqualPoints(x, y, s_pointStack[0].x, s_pointStack[0].y))
        {
            OnFinishDraw();
            return TRUE;
        }
        else
        {
            s_pointSP++;
            s_pointStack[s_pointSP].x = x;
            s_pointStack[s_pointSP].y = y;
        }

        if (s_pointSP == _countof(s_pointStack))
            s_pointSP--;

        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnCancelDraw() override
    {
        ToolBase::OnCancelDraw();
    }

    void OnFinishDraw() override
    {
        if (s_pointSP)
        {
            --s_pointSP;
            m_bClosed = TRUE;

            imageModel.PushImageForUndo();
            OnDrawOverlayOnImage(m_hdc);
        }

        m_bClosed = FALSE;
        s_pointSP = 0;

        ToolBase::OnFinishDraw();
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        toolsModel.MakeLineThickerOrThinner(bMinus);
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
            regularize(g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y);
        if (m_bLeftButton)
            Ellp(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            Ellp(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
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
            regularize(g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y);
        if (m_bLeftButton)
            RRect(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
        else
            RRect(hdc, g_ptStart.x, g_ptStart.y, g_ptEnd.x, g_ptEnd.y, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle());
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
