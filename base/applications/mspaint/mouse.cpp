/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Things which should not be in the mouse event handler itself
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2021-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

INT ToolBase::s_pointSP = 0;
INT ToolBase::s_maxPointSP = 256;
static POINT s_staticPointStack[256];
LPPOINT ToolBase::s_pointStack = s_staticPointStack;
static POINT g_ptStart, g_ptEnd;

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

    if (s_pointStack != s_staticPointStack)
    {
        ::LocalFree(s_pointStack);
        s_pointStack = s_staticPointStack;
        s_maxPointSP = _countof(s_staticPointStack);
    }
}

void ToolBase::OnEndDraw(BOOL bCancel)
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

void ToolBase::pushToPtStack(LONG x, LONG y)
{
    if (s_pointSP >= s_maxPointSP)
    {
        INT newMax = s_maxPointSP + 256;
        SIZE_T cbNew = newMax * sizeof(POINT);

        LPPOINT pptNew;
        if (s_pointStack == s_staticPointStack)
            pptNew = (LPPOINT)::LocalAlloc(LPTR, cbNew);
        else
            pptNew = (LPPOINT)::LocalReAlloc(s_pointStack, cbNew, 0);

        if (!pptNew)
        {
            ATLTRACE("Out of memory!\n");
            return;
        }

        if (s_pointStack == s_staticPointStack)
            CopyMemory(pptNew, s_staticPointStack, s_pointSP * sizeof(POINT));

        s_pointStack = pptNew;
        s_maxPointSP = newMax;
    }

    s_pointStack[s_pointSP++] = { x, y };
}

void ToolBase::getBoundaryOfPtStack(RECT& rcBoundary)
{
    POINT ptMin, ptMax;
    ptMin = ptMax = s_pointStack[0];

    for (INT i = 1; i < s_pointSP; ++i)
    {
        LONG x = s_pointStack[i].x, y = s_pointStack[i].y;
        ptMin.x = min(x, ptMin.x);
        ptMin.y = min(y, ptMin.y);
        ptMax.x = max(x, ptMax.x);
        ptMax.y = max(y, ptMax.y);
    }

    CRect rc(ptMin, ptMax);
    rcBoundary = rc;
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

    void OnEndDraw(BOOL bCancel) override
    {
        if (bCancel)
            selectionModel.HideSelection();
        else
            selectionModel.Landing();
        ToolBase::OnEndDraw(bCancel);
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

    void OnEndDraw(BOOL bCancel) override
    {
        if (bCancel)
            selectionModel.HideSelection();
        else
            selectionModel.Landing();
        ToolBase::OnEndDraw(bCancel);
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
        CRect rcPartial(g_ptStart, g_ptEnd);
        rcPartial.NormalizeRect();
        SIZE size = toolsModel.GetToolSize();
        rcPartial.InflateRect((size.cx + 1) / 2, (size.cy + 1) / 2);
        imageModel.PushImageForUndo(rcPartial);

        OnDrawOverlayOnImage(m_hdc);
        m_bDrawing = FALSE;
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnEndDraw(BOOL bCancel) override
    {
        m_bDrawing = FALSE;
        ToolBase::OnEndDraw(bCancel);
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
    BOOL m_bLeftButton = FALSE;

    SmoothDrawTool(TOOLTYPE type) : ToolBase(type)
    {
    }

    virtual void OnDraw(HDC hdc, BOOL bLeftButton, POINT pt0, POINT pt1) = 0;

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_direction = NO_DIRECTION;
        m_bShiftDown = (::GetKeyState(VK_SHIFT) & 0x8000); // Is Shift key pressed?
        m_bLeftButton = bLeftButton;
        s_pointSP = 0;
        pushToPtStack(x, y);
        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (!m_bShiftDown)
        {
            pushToPtStack(x, y);
            imageModel.NotifyImageChanged();
            return TRUE;
        }

        if (m_direction == NO_DIRECTION)
        {
            m_direction = GetDirection(g_ptStart.x, g_ptStart.y, x, y);
            if (m_direction == NO_DIRECTION)
                return FALSE;
        }

        RestrictDrawDirection(m_direction, g_ptStart.x, g_ptStart.y, x, y);
        pushToPtStack(x, y);
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (m_bShiftDown && m_direction != NO_DIRECTION)
            RestrictDrawDirection(m_direction, g_ptStart.x, g_ptStart.y, x, y);

        pushToPtStack(x, y);

        CRect rcPartial;
        getBoundaryOfPtStack(rcPartial);

        SIZE size = toolsModel.GetToolSize();
        rcPartial.InflateRect((size.cx + 1) / 2, (size.cy + 1) / 2);

        imageModel.PushImageForUndo(rcPartial);

        OnDrawOverlayOnImage(m_hdc);
        imageModel.NotifyImageChanged();
        OnEndDraw(FALSE);
        return TRUE;
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        for (INT i = 1; i < s_pointSP; ++i)
        {
            OnDraw(hdc, m_bLeftButton, s_pointStack[i - 1], s_pointStack[i]);
        }
    }
};

// TOOL_RUBBER
struct RubberTool : SmoothDrawTool
{
    RubberTool() : SmoothDrawTool(TOOL_RUBBER)
    {
    }

    void OnDraw(HDC hdc, BOOL bLeftButton, POINT pt0, POINT pt1) override
    {
        if (bLeftButton)
            Erase(hdc, pt0.x, pt0.y, pt1.x, pt1.y, m_bg, toolsModel.GetRubberRadius());
        else
            Replace(hdc, pt0.x, pt0.y, pt1.x, pt1.y, m_fg, m_bg, toolsModel.GetRubberRadius());
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

    void OnDraw(HDC hdc, BOOL bLeftButton, POINT pt0, POINT pt1) override
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Line(hdc, pt0.x, pt0.y, pt1.x, pt1.y, rgb, toolsModel.GetPenWidth());
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

    void OnDraw(HDC hdc, BOOL bLeftButton, POINT pt0, POINT pt1) override
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Brush(hdc, pt0.x, pt0.y, pt1.x, pt1.y, rgb, toolsModel.GetBrushStyle(),
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
    DWORD m_dwTick = 0;

    AirBrushTool() : SmoothDrawTool(TOOL_AIRBRUSH)
    {
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_dwTick = GetTickCount();
        SmoothDrawTool::OnButtonDown(bLeftButton, x, y, bDoubleClick);
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        srand(m_dwTick);
        SmoothDrawTool::OnDrawOverlayOnImage(hdc);
    }

    void OnDraw(HDC hdc, BOOL bLeftButton, POINT pt0, POINT pt1) override
    {
        COLORREF rgb = bLeftButton ? m_fg : m_bg;
        Airbrush(hdc, pt1.x, pt1.y, rgb, toolsModel.GetAirBrushRadius());
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
        CStringW szText;
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

    void OnEndDraw(BOOL bCancel) override
    {
        if (!bCancel)
        {
            if (::IsWindowVisible(textEditWindow) &&
                textEditWindow.GetWindowTextLength() > 0)
            {
                imageModel.PushImageForUndo();
                draw(m_hdc);
            }
        }
        quit();
        ToolBase::OnEndDraw(bCancel);
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
            OnEndDraw(FALSE);
            return TRUE;
        }
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnEndDraw(BOOL bCancel) override
    {
        if (!bCancel)
        {
            imageModel.PushImageForUndo();
            OnDrawOverlayOnImage(m_hdc);
        }
        m_bDrawing = FALSE;
        ToolBase::OnEndDraw(bCancel);
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
            OnEndDraw(FALSE);
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
            OnEndDraw(FALSE);
            return TRUE;
        }
        else
        {
            s_pointSP++;
            s_pointStack[s_pointSP].x = x;
            s_pointStack[s_pointSP].y = y;
        }

        if (s_pointSP == s_maxPointSP)
            s_pointSP--;

        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnEndDraw(BOOL bCancel) override
    {
        if (!bCancel)
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
        }
        ToolBase::OnEndDraw(bCancel);
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

void ToolsModel::OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
{
    m_pToolObject->beginEvent();
    g_ptStart.x = g_ptEnd.x = x;
    g_ptStart.y = g_ptEnd.y = y;
    m_pToolObject->OnButtonDown(bLeftButton, x, y, bDoubleClick);
    m_pToolObject->endEvent();
}

void ToolsModel::OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    if (m_pToolObject->OnMouseMove(bLeftButton, x, y))
    {
        g_ptEnd.x = x;
        g_ptEnd.y = y;
    }
    m_pToolObject->endEvent();
}

void ToolsModel::OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    if (m_pToolObject->OnButtonUp(bLeftButton, x, y))
    {
        g_ptEnd.x = x;
        g_ptEnd.y = y;
    }
    m_pToolObject->endEvent();
}

void ToolsModel::OnEndDraw(BOOL bCancel)
{
    ATLTRACE("ToolsModel::OnEndDraw(%d)\n", bCancel);
    m_pToolObject->beginEvent();
    m_pToolObject->OnEndDraw(bCancel);
    m_pToolObject->endEvent();
}

void ToolsModel::OnDrawOverlayOnImage(HDC hdc)
{
    m_pToolObject->OnDrawOverlayOnImage(hdc);
}

void ToolsModel::OnDrawOverlayOnCanvas(HDC hdc)
{
    m_pToolObject->OnDrawOverlayOnCanvas(hdc);
}

void ToolsModel::SpecialTweak(BOOL bMinus)
{
    m_pToolObject->OnSpecialTweak(bMinus);
}

void ToolsModel::DrawWithMouseTool(POINT pt, WPARAM wParam)
{
    LONG xRel = pt.x - g_ptStart.x, yRel = pt.y - g_ptStart.y;

    switch (m_activeTool)
    {
        // freesel, rectsel and text tools always show numbers limited to fit into image area
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            if (xRel < 0)
                xRel = (pt.x < 0) ? -g_ptStart.x : xRel;
            else if (pt.x > imageModel.GetWidth())
                xRel = imageModel.GetWidth() - g_ptStart.x;
            if (yRel < 0)
                yRel = (pt.y < 0) ? -g_ptStart.y : yRel;
            else if (pt.y > imageModel.GetHeight())
                yRel = imageModel.GetHeight() - g_ptStart.y;
            break;

        // while drawing, update cursor coordinates only for tools 3, 7, 8, 9, 14
        case TOOL_RUBBER:
        case TOOL_PEN:
        case TOOL_BRUSH:
        case TOOL_AIRBRUSH:
        case TOOL_SHAPE:
        {
            CStringW strCoord;
            strCoord.Format(L"%ld, %ld", pt.x, pt.y);
            ::SendMessageW(g_hStatusBar, SB_SETTEXT, 1, (LPARAM)(LPCWSTR)strCoord);
            break;
        }
        default:
            break;
    }

    // rectsel and shape tools always show non-negative numbers when drawing
    if (m_activeTool == TOOL_RECTSEL || m_activeTool == TOOL_SHAPE)
    {
        xRel = labs(xRel);
        yRel = labs(yRel);
    }

    if (wParam & MK_LBUTTON)
    {
        OnMouseMove(TRUE, pt.x, pt.y);
        canvasWindow.Invalidate(FALSE);
        if ((m_activeTool >= TOOL_TEXT) || IsSelection())
        {
            CStringW strSize;
            if ((m_activeTool >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                yRel = xRel;
            strSize.Format(L"%ld x %ld", xRel, yRel);
            ::SendMessageW(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)(LPCWSTR)strSize);
        }
    }

    if (wParam & MK_RBUTTON)
    {
        OnMouseMove(FALSE, pt.x, pt.y);
        canvasWindow.Invalidate(FALSE);
        if (m_activeTool >= TOOL_TEXT)
        {
            CStringW strSize;
            if ((m_activeTool >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                yRel = xRel;
            strSize.Format(L"%ld x %ld", xRel, yRel);
            ::SendMessageW(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)(LPCWSTR)strSize);
        }
    }
}
