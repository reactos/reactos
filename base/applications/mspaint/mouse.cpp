/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Things which should not be in the mouse event handler itself
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2021-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <atlalloc.h>

static SIZE_T s_cPoints = 0;
static CHeapPtr<POINT, CLocalAllocator> s_dynamicPoints;
static POINT s_staticPoints[512]; // 512 is enough
static SIZE_T s_maxPoints = _countof(s_staticPoints);
static LPPOINT s_pPoints = s_staticPoints;
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

void getBoundaryOfPoints(RECT& rcBoundary, SIZE_T cPoints, const POINT *pPoints)
{
    POINT ptMin = { MAXLONG, MAXLONG }, ptMax = { (LONG)MINLONG, (LONG)MINLONG };
    while (cPoints-- > 0)
    {
        LONG x = pPoints->x, y = pPoints->y;
        ptMin = { min(x, ptMin.x), min(y, ptMin.y) };
        ptMax = { max(x, ptMax.x), max(y, ptMax.y) };
        ++pPoints;
    }

    ptMax.x += 1;
    ptMax.y += 1;

    CRect rc(ptMin, ptMax);
    rcBoundary = rc;
}

void ShiftPoints(INT dx, INT dy)
{
    for (SIZE_T i = 0; i < s_cPoints; ++i)
    {
        POINT& pt = s_pPoints[i];
        pt.x += dx;
        pt.y += dy;
    }
}

void BuildMaskFromPoints()
{
    CRect rc;
    getBoundaryOfPoints(rc, s_cPoints, s_pPoints);

    ShiftPoints(-rc.left, -rc.top);

    HDC hdcMem = ::CreateCompatibleDC(NULL);
    HBITMAP hbmMask = ::CreateBitmap(rc.Width(), rc.Height(), 1, 1, NULL);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbmMask);
    ::FillRect(hdcMem, &rc, (HBRUSH)::GetStockObject(BLACK_BRUSH));
    HGDIOBJ hPenOld = ::SelectObject(hdcMem, GetStockObject(NULL_PEN));
    HGDIOBJ hbrOld = ::SelectObject(hdcMem, GetStockObject(WHITE_BRUSH));
    ::Polygon(hdcMem, s_pPoints, (INT)s_cPoints);
    ::SelectObject(hdcMem, hbrOld);
    ::SelectObject(hdcMem, hPenOld);
    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);

    selectionModel.setMask(rc, hbmMask);
}

void ToolBase::reset()
{
    if (s_pPoints != s_staticPoints)
    {
        s_dynamicPoints.Free();
        s_pPoints = s_staticPoints;
        s_maxPoints = _countof(s_staticPoints);
    }

    s_cPoints = 0;
    g_ptEnd = g_ptStart = { -1, -1 };

    if (selectionModel.m_bShow)
    {
        selectionModel.Landing();
        selectionModel.HideSelection();
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

static void pushToPoints(LONG x, LONG y)
{
    if (s_cPoints + 1 >= s_maxPoints)
    {
        SIZE_T newMax = s_maxPoints + 512;
        SIZE_T cbNew = newMax * sizeof(POINT);
        if (!s_dynamicPoints.ReallocateBytes(cbNew))
        {
            ATLTRACE("%d, %d, %d\n", (INT)s_cPoints, (INT)s_maxPoints, (INT)cbNew);
            return;
        }

        if (s_pPoints == s_staticPoints)
            CopyMemory(s_dynamicPoints, s_staticPoints, s_cPoints * sizeof(POINT));

        s_pPoints = s_dynamicPoints;
        s_maxPoints = newMax;
    }

    s_pPoints[s_cPoints++] = { x, y };
}

/* TOOLS ********************************************************/

struct TwoPointDrawTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;
    BOOL m_bDrawing = FALSE;

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

    virtual void OnDraw(HDC hdc, BOOL bLeftButton, POINT pt0, POINT pt1) = 0;

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_direction = NO_DIRECTION;
        m_bShiftDown = (::GetKeyState(VK_SHIFT) & 0x8000); // Is Shift key pressed?
        m_bLeftButton = bLeftButton;
        s_cPoints = 0;
        pushToPoints(x, y);
        pushToPoints(x, y); // We have to draw the first point
        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (!m_bShiftDown)
        {
            pushToPoints(x, y);
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
        pushToPoints(x, y);
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (m_bShiftDown && m_direction != NO_DIRECTION)
            RestrictDrawDirection(m_direction, g_ptStart.x, g_ptStart.y, x, y);

        pushToPoints(x, y);

        CRect rcPartial;
        getBoundaryOfPoints(rcPartial, s_cPoints, s_pPoints);

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
        for (SIZE_T i = 1; i < s_cPoints; ++i)
        {
            OnDraw(hdc, m_bLeftButton, s_pPoints[i - 1], s_pPoints[i]);
        }
    }
};

struct SelectionBaseTool : ToolBase
{
    BOOL m_bLeftButton = FALSE;
    BOOL m_bCtrlKey = FALSE;
    BOOL m_bShiftKey = FALSE;
    BOOL m_bDrawing = FALSE;
    BOOL m_bNoDrawBack = FALSE;
    HITTEST m_hitSelection = HIT_NONE;

    BOOL isRectSelect() const
    {
        return (toolsModel.GetActiveTool() == TOOL_RECTSEL);
    }

    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (selectionModel.IsLanded() || !selectionModel.m_bShow)
            return;

        if (!m_bNoDrawBack)
            selectionModel.DrawBackground(hdc, selectionModel.m_rgbBack);

        selectionModel.DrawSelection(hdc, paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());
    }

    void OnDrawOverlayOnCanvas(HDC hdc) override
    {
        if (m_bDrawing || selectionModel.m_bShow)
            selectionModel.drawFrameOnCanvas(hdc);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;
        m_bCtrlKey = (::GetKeyState(VK_CONTROL) < 0);
        m_bShiftKey = (::GetKeyState(VK_SHIFT) < 0);
        m_bDrawing = FALSE;
        m_hitSelection = HIT_NONE;

        POINT pt = { x, y };
        if (!m_bLeftButton) // Show context menu on Right-click
        {
            canvasWindow.ImageToCanvas(pt);
            canvasWindow.ClientToScreen(&pt);
            mainWindow.TrackPopupMenu(pt, 0);
            return;
        }

        POINT ptCanvas = pt;
        canvasWindow.ImageToCanvas(ptCanvas);
        HITTEST hit = selectionModel.hitTest(ptCanvas);
        if (hit != HIT_NONE) // Dragging of selection started?
        {
            if (m_bCtrlKey || m_bShiftKey)
            {
                imageModel.PushImageForUndo();
                toolsModel.OnDrawOverlayOnImage(imageModel.GetDC());
            }
            m_hitSelection = hit;
            selectionModel.m_ptHit = pt;
            selectionModel.TakeOff();
            m_bNoDrawBack |= (m_bCtrlKey || m_bShiftKey);
            imageModel.NotifyImageChanged();
            return;
        }

        selectionModel.Landing();
        m_bDrawing = TRUE;

        imageModel.Clamp(pt);
        if (isRectSelect())
        {
            selectionModel.SetRectFromPoints(g_ptStart, pt);
        }
        else
        {
            s_cPoints = 0;
            pushToPoints(pt.x, pt.y);
        }

        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        POINT pt = { x, y };

        if (!m_bLeftButton)
            return TRUE;

        if (m_hitSelection != HIT_NONE) // Now dragging selection?
        {
            if (m_bShiftKey)
                toolsModel.OnDrawOverlayOnImage(imageModel.GetDC());

            selectionModel.Dragging(m_hitSelection, pt);
            imageModel.NotifyImageChanged();
            return TRUE;
        }

        if (isRectSelect() && ::GetKeyState(VK_SHIFT) < 0)
            regularize(g_ptStart.x, g_ptStart.y, pt.x, pt.y);

        imageModel.Clamp(pt);

        if (isRectSelect())
            selectionModel.SetRectFromPoints(g_ptStart, pt);
        else
            pushToPoints(pt.x, pt.y);

        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        POINT pt = { x, y };
        m_bDrawing = FALSE;

        if (!m_bLeftButton)
            return TRUE;

        if (m_hitSelection != HIT_NONE) // Dragging of selection ended?
        {
            if (m_bShiftKey)
                toolsModel.OnDrawOverlayOnImage(imageModel.GetDC());

            selectionModel.Dragging(m_hitSelection, pt);
            m_hitSelection = HIT_NONE;
            imageModel.NotifyImageChanged();
            return TRUE;
        }

        if (isRectSelect() && ::GetKeyState(VK_SHIFT) < 0)
            regularize(g_ptStart.x, g_ptStart.y, pt.x, pt.y);

        imageModel.Clamp(pt);

        if (isRectSelect())
        {
            selectionModel.SetRectFromPoints(g_ptStart, pt);
            selectionModel.m_bShow = !selectionModel.m_rc.IsRectEmpty();
        }
        else
        {
            if (s_cPoints > 2)
            {
                BuildMaskFromPoints();
                selectionModel.m_bShow = TRUE;
            }
            else
            {
                s_cPoints = 0;
                selectionModel.m_bShow = FALSE;
            }
        }

        m_bNoDrawBack = FALSE;
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnEndDraw(BOOL bCancel) override
    {
        if (bCancel)
            selectionModel.HideSelection();
        else
            selectionModel.Landing();

        m_bDrawing = FALSE;
        m_hitSelection = HIT_NONE;
        ToolBase::OnEndDraw(bCancel);
    }

    void OnSpecialTweak(BOOL bMinus) override
    {
        selectionModel.StretchSelection(bMinus);
    }
};

// TOOL_FREESEL
struct FreeSelTool : SelectionBaseTool
{
    void OnDrawOverlayOnImage(HDC hdc) override
    {
        SelectionBaseTool::OnDrawOverlayOnImage(hdc);

        if (!selectionModel.m_bShow && m_bDrawing)
        {
            /* Draw the freehand selection inverted/xored */
            Poly(hdc, s_pPoints, (INT)s_cPoints, 0, 0, 2, 0, FALSE, TRUE);
        }
    }
};

// TOOL_RECTSEL
struct RectSelTool : SelectionBaseTool
{
    void OnDrawOverlayOnImage(HDC hdc) override
    {
        SelectionBaseTool::OnDrawOverlayOnImage(hdc);

        if (!selectionModel.m_bShow && m_bDrawing)
        {
            CRect& rc = selectionModel.m_rc;
            if (!rc.IsRectEmpty())
                RectSel(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    }
};

// TOOL_RUBBER
struct RubberTool : SmoothDrawTool
{
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
    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        imageModel.PushImageForUndo();
        Fill(m_hdc, x, y, bLeftButton ? m_fg : m_bg);
    }
};

// TOOL_COLOR
struct ColorTool : ToolBase
{
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
    void OnDrawOverlayOnImage(HDC hdc) override
    {
        if (canvasWindow.m_drawing)
        {
            CRect& rc = selectionModel.m_rc;
            if (!rc.IsRectEmpty())
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

        CRect rc;
        textEditWindow.InvalidateEditRect();
        textEditWindow.GetEditRect(&rc);
        rc.InflateRect(-GRIP_SIZE / 2, -GRIP_SIZE / 2);

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
            if (selectionModel.m_rc.IsRectEmpty())
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

        CRect rc = selectionModel.m_rc;

        // Enlarge if tool small
        INT cxMin = CX_MINTEXTEDIT, cyMin = CY_MINTEXTEDIT;
        if (selectionModel.m_rc.IsRectEmpty())
        {
            rc.SetRect(x, y, x + cxMin, y + cyMin);
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

    void OnDrawOverlayOnImage(HDC hdc)
    {
        COLORREF rgb = (m_bLeftButton ? m_fg : m_bg);
        switch (s_cPoints)
        {
            case 2:
                Line(hdc, s_pPoints[0].x, s_pPoints[0].y, s_pPoints[1].x, s_pPoints[1].y, rgb,
                     toolsModel.GetLineWidth());
                break;
            case 3:
                Bezier(hdc, s_pPoints[0], s_pPoints[2], s_pPoints[2], s_pPoints[1], rgb, toolsModel.GetLineWidth());
                break;
            case 4:
                Bezier(hdc, s_pPoints[0], s_pPoints[2], s_pPoints[3], s_pPoints[1], rgb, toolsModel.GetLineWidth());
                break;
        }
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;

        if (s_cPoints == 0)
        {
            pushToPoints(x, y);
            pushToPoints(x, y);
        }
        else
        {
            s_pPoints[s_cPoints - 1] = { x, y };
        }

        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (s_cPoints > 0)
            s_pPoints[s_cPoints - 1] = { x, y };
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (s_cPoints >= 4)
        {
            OnEndDraw(FALSE);
            return TRUE;
        }
        pushToPoints(x, y);
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnEndDraw(BOOL bCancel) override
    {
        if (!bCancel && s_cPoints > 1)
        {
            // FIXME: I couldn't calculate boundary rectangle from Bezier curve
            imageModel.PushImageForUndo();
            OnDrawOverlayOnImage(m_hdc);
        }
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

    void OnDrawOverlayOnImage(HDC hdc)
    {
        if (s_cPoints <= 0)
            return;

        if (m_bLeftButton)
            Poly(hdc, s_pPoints, (INT)s_cPoints, m_fg, m_bg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), m_bClosed, FALSE);
        else
            Poly(hdc, s_pPoints, (INT)s_cPoints, m_bg, m_fg, toolsModel.GetLineWidth(), toolsModel.GetShapeStyle(), m_bClosed, FALSE);
    }

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) override
    {
        m_bLeftButton = bLeftButton;
        m_bClosed = FALSE;

        if ((s_cPoints > 0) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(s_pPoints[s_cPoints - 1].x, s_pPoints[s_cPoints - 1].y, x, y);

        pushToPoints(x, y);

        if (s_cPoints > 1 && bDoubleClick)
        {
            OnEndDraw(FALSE);
            return;
        }

        if (s_cPoints == 1)
            pushToPoints(x, y); // We have to draw the first point

        imageModel.NotifyImageChanged();
    }

    BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if (s_cPoints > 1)
        {
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                roundTo8Directions(s_pPoints[s_cPoints - 2].x, s_pPoints[s_cPoints - 2].y, x, y);

            s_pPoints[s_cPoints - 1] = { x, y };
        }

        imageModel.NotifyImageChanged();
        return TRUE;
    }

    BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) override
    {
        if ((s_cPoints > 1) && (GetAsyncKeyState(VK_SHIFT) < 0))
            roundTo8Directions(s_pPoints[s_cPoints - 2].x, s_pPoints[s_cPoints - 2].y, x, y);

        m_bClosed = FALSE;
        if (nearlyEqualPoints(x, y, s_pPoints[0].x, s_pPoints[0].y))
        {
            OnEndDraw(FALSE);
            return TRUE;
        }

        pushToPoints(x, y);
        imageModel.NotifyImageChanged();
        return TRUE;
    }

    void OnEndDraw(BOOL bCancel) override
    {
        if (!bCancel && s_cPoints > 1)
        {
            CRect rcPartial;
            getBoundaryOfPoints(rcPartial, s_cPoints, s_pPoints);

            SIZE size = toolsModel.GetToolSize();
            rcPartial.InflateRect((size.cx + 1) / 2, (size.cy + 1) / 2);

            imageModel.PushImageForUndo(rcPartial);

            m_bClosed = TRUE;
            OnDrawOverlayOnImage(m_hdc);
        }
        m_bClosed = FALSE;
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
    g_ptEnd = g_ptStart = { x, y };
    m_pToolObject->OnButtonDown(bLeftButton, x, y, bDoubleClick);
    m_pToolObject->endEvent();
}

void ToolsModel::OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    if (m_pToolObject->OnMouseMove(bLeftButton, x, y))
        g_ptEnd = { x, y };

    m_pToolObject->endEvent();
}

void ToolsModel::OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    if (m_pToolObject->OnButtonUp(bLeftButton, x, y))
        g_ptEnd = { x, y };

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
