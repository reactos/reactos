/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the tool settings window
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2018 Stanislav Motylkov <x86corez@gmail.com>
 *             Copyright 2021-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#define X_TOOLSETTINGS  0
#define Y_TOOLSETTINGS  (CY_TOOLBAR + 3)
#define CX_TOOLSETTINGS CX_TOOLBAR
#define CY_TOOLSETTINGS 140

#define CX_TRANS_ICON 40
#define CY_TRANS_ICON 30
#define MARGIN1 3
#define MARGIN2 2

#define MAX_ZOOM_TRACK 6
#define MIN_ZOOM_TRACK 0
#define DEFAULT_ZOOM_TRACK 3

static const BYTE s_AirRadius[4] = { 5, 8, 3, 12 };

CToolSettingsWindow toolSettingsWindow;

/* FUNCTIONS ********************************************************/

BOOL CToolSettingsWindow::DoCreate(HWND hwndParent)
{
    RECT toolSettingsWindowPos =
    {
        X_TOOLSETTINGS, Y_TOOLSETTINGS,
        X_TOOLSETTINGS + CX_TOOLSETTINGS, Y_TOOLSETTINGS + CY_TOOLSETTINGS
    };
    return !!Create(toolBoxContainer, toolSettingsWindowPos, NULL, WS_CHILD | WS_VISIBLE);
}

static INT
getSplitRects(RECT *rects, INT cColumns, INT cRows, LPCRECT prc, LPPOINT ppt)
{
    INT cx = prc->right - prc->left, cy = prc->bottom - prc->top;
    for (INT i = 0, iRow = 0; iRow < cRows; ++iRow)
    {
        for (INT iColumn = 0; iColumn < cColumns; ++iColumn)
        {
            RECT& rc = rects[i];
            rc.left = prc->left + (iColumn * cx / cColumns);
            rc.top = prc->top + (iRow * cy / cRows);
            rc.right = prc->left + ((iColumn + 1) * cx / cColumns);
            rc.bottom = prc->top + ((iRow + 1) * cy / cRows);
            if (ppt && ::PtInRect(&rc, *ppt))
                return i;
            ++i;
        }
    }
    return -1;
}

static inline INT getTransRects(RECT rects[2], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(rects, 1, 2, prc, ppt);
}

VOID CToolSettingsWindow::drawTrans(HDC hdc, LPCRECT prc)
{
    RECT rc[2];
    getTransRects(rc, prc);

    ::FillRect(hdc, &rc[toolsModel.IsBackgroundTransparent()], (HBRUSH)(COLOR_HIGHLIGHT + 1));
    ::DrawIconEx(hdc, rc[0].left, rc[0].top, m_hNontranspIcon,
                 CX_TRANS_ICON, CY_TRANS_ICON, 0, NULL, DI_NORMAL);
    ::DrawIconEx(hdc, rc[1].left, rc[1].top, m_hTranspIcon,
                 CX_TRANS_ICON, CY_TRANS_ICON, 0, NULL, DI_NORMAL);
}

static inline INT getRubberRects(RECT rects[4], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(rects, 1, 4, prc, ppt);
}

VOID CToolSettingsWindow::drawRubber(HDC hdc, LPCRECT prc)
{
    RECT rects[4], rcRubber;
    getRubberRects(rects, prc);
    INT xCenter = (prc->left + prc->right) / 2;
    for (INT i = 0; i < 4; i++)
    {
        INT iColor, radius = i + 2;
        if (toolsModel.GetRubberRadius() == radius)
        {
            ::FillRect(hdc, &rects[i], ::GetSysColorBrush(COLOR_HIGHLIGHT));
            iColor = COLOR_HIGHLIGHTTEXT;
        }
        else
        {
            iColor = COLOR_WINDOWTEXT;
        }

        INT yCenter = (rects[i].top + rects[i].bottom) / 2;
        rcRubber.left = xCenter - radius;
        rcRubber.top = yCenter - radius;
        rcRubber.right = rcRubber.left + radius * 2;
        rcRubber.bottom = rcRubber.top + radius * 2;
        ::FillRect(hdc, &rcRubber, GetSysColorBrush(iColor));
    }
}

static inline INT getBrushRects(RECT rects[12], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(rects, 3, 4, prc, ppt);
}

struct BrushStyleAndWidth
{
    BrushStyle style;
    INT width;
};

static const BrushStyleAndWidth c_BrushPresets[] =
{
    { BrushStyleRound,     7 }, { BrushStyleRound,     4 }, { BrushStyleRound,     1 },
    { BrushStyleSquare,    8 }, { BrushStyleSquare,    5 }, { BrushStyleSquare,    2 },
    { BrushStyleForeSlash, 8 }, { BrushStyleForeSlash, 5 }, { BrushStyleForeSlash, 2 },
    { BrushStyleBackSlash, 8 }, { BrushStyleBackSlash, 5 }, { BrushStyleBackSlash, 2 },
};

VOID CToolSettingsWindow::drawBrush(HDC hdc, LPCRECT prc)
{
    RECT rects[12];
    getBrushRects(rects, prc);

    for (INT i = 0; i < 12; i++)
    {
        RECT rcItem = rects[i];
        INT x = (rcItem.left + rcItem.right) / 2, y = (rcItem.top + rcItem.bottom) / 2;

        INT iColor;
        const BrushStyleAndWidth& data = c_BrushPresets[i];
        if (data.width == toolsModel.GetBrushWidth() && data.style == toolsModel.GetBrushStyle())
        {
            iColor = COLOR_HIGHLIGHTTEXT;
            ::FillRect(hdc, &rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
        }
        else
        {
            iColor = COLOR_WINDOWTEXT;
        }

        Brush(hdc, x, y, x, y, ::GetSysColor(iColor), data.style, data.width);
    }
}

static inline INT getLineRects(RECT rects[5], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(rects, 1, 5, prc, ppt);
}

VOID CToolSettingsWindow::drawLine(HDC hdc, LPCRECT prc)
{
    RECT rects[5];
    getLineRects(rects, prc);

    for (INT i = 0; i < 5; i++)
    {
        INT penWidth = i + 1;
        CRect rcLine = rects[i];
        rcLine.InflateRect(-2, 0);
        rcLine.top = (rcLine.top + rcLine.bottom - penWidth) / 2;
        rcLine.bottom = rcLine.top + penWidth;
        if (toolsModel.GetLineWidth() == penWidth)
        {
            ::FillRect(hdc, &rects[i], ::GetSysColorBrush(COLOR_HIGHLIGHT));
            ::FillRect(hdc, &rcLine, ::GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            ::FillRect(hdc, &rcLine, ::GetSysColorBrush(COLOR_WINDOWTEXT));
        }
    }
}

static INT getAirBrushRects(RECT rects[4], LPCRECT prc, LPPOINT ppt = NULL)
{
    INT cx = (prc->right - prc->left), cy = (prc->bottom - prc->top);

    rects[0] = rects[1] = rects[2] = rects[3] = *prc;

    rects[0].right = rects[1].left = prc->left + cx * 3 / 8;
    rects[0].bottom = rects[1].bottom = prc->top + cy / 2;

    rects[2].top = rects[3].top = prc->top + cy / 2;
    rects[2].right = rects[3].left = prc->left + cx * 2 / 8;

    if (ppt)
    {
        for (INT i = 0; i < 4; ++i)
        {
            if (::PtInRect(&rects[i], *ppt))
                return i;
        }
    }
    return -1;
}

VOID CToolSettingsWindow::drawAirBrush(HDC hdc, LPCRECT prc)
{
    RECT rects[4];
    getAirBrushRects(rects, prc);

    srand(0);
    for (size_t i = 0; i < 4; ++i)
    {
        RECT& rc = rects[i];
        INT x = (rc.left + rc.right) / 2;
        INT y = (rc.top + rc.bottom) / 2;
        BOOL bHigh = (s_AirRadius[i] == toolsModel.GetAirBrushRadius());
        if (bHigh)
        {
            ::FillRect(hdc, &rc, ::GetSysColorBrush(COLOR_HIGHLIGHT));

            for (int k = 0; k < 3; ++k)
                Airbrush(hdc, x, y, ::GetSysColor(COLOR_HIGHLIGHTTEXT), s_AirRadius[i]);
        }
        else
        {
            for (int k = 0; k < 3; ++k)
                Airbrush(hdc, x, y, ::GetSysColor(COLOR_WINDOWTEXT), s_AirRadius[i]);
        }
    }
}

static inline INT getBoxRects(RECT rects[3], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(rects, 1, 3, prc, ppt);
}

VOID CToolSettingsWindow::drawBox(HDC hdc, LPCRECT prc)
{
    CRect rects[3];
    getBoxRects(rects, prc);

    for (INT iItem = 0; iItem < 3; ++iItem)
    {
        CRect& rcItem = rects[iItem];

        if (toolsModel.GetShapeStyle() == iItem)
            ::FillRect(hdc, &rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHT));

        rcItem.InflateRect(-5, -5);

        if (iItem <= 1)
        {
            COLORREF rgbPen;
            if (toolsModel.GetShapeStyle() == iItem)
                rgbPen = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
            else
                rgbPen = ::GetSysColor(COLOR_WINDOWTEXT);
            HGDIOBJ hOldBrush;
            if (iItem == 0)
                hOldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
            else
                hOldBrush = ::SelectObject(hdc, ::GetSysColorBrush(COLOR_APPWORKSPACE));
            HGDIOBJ hOldPen = ::SelectObject(hdc, ::CreatePen(PS_SOLID, 1, rgbPen));
            ::Rectangle(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
            ::DeleteObject(::SelectObject(hdc, hOldPen));
            ::SelectObject(hdc, hOldBrush);
        }
        else
        {
            if (toolsModel.GetShapeStyle() == iItem)
                ::FillRect(hdc, &rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
            else
                ::FillRect(hdc, &rcItem, ::GetSysColorBrush(COLOR_WINDOWTEXT));
        }
    }
}

LRESULT CToolSettingsWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* preloading the draw transparent/nontransparent icons for later use */
    m_hNontranspIcon = (HICON)LoadImageW(g_hinstExe, MAKEINTRESOURCEW(IDI_NONTRANSPARENT),
                                         IMAGE_ICON, CX_TRANS_ICON, CY_TRANS_ICON, LR_DEFAULTCOLOR);
    m_hTranspIcon = (HICON)LoadImageW(g_hinstExe, MAKEINTRESOURCEW(IDI_TRANSPARENT),
                                      IMAGE_ICON, CX_TRANS_ICON, CY_TRANS_ICON, LR_DEFAULTCOLOR);

    CRect trackbarZoomPos, rect2;
    calculateTwoBoxes(trackbarZoomPos, rect2);
    trackbarZoomPos.InflateRect(-1, -1);

    trackbarZoom.Create(TRACKBAR_CLASS, m_hWnd, trackbarZoomPos, NULL, WS_CHILD | TBS_VERT | TBS_AUTOTICKS);
    trackbarZoom.SendMessage(TBM_SETRANGE, TRUE, MAKELPARAM(MIN_ZOOM_TRACK, MAX_ZOOM_TRACK));
    trackbarZoom.SendMessage(TBM_SETPOS, TRUE, DEFAULT_ZOOM_TRACK);
    return 0;
}

LRESULT CToolSettingsWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::DestroyIcon(m_hNontranspIcon);
    ::DestroyIcon(m_hTranspIcon);
    return 0;
}

LRESULT CToolSettingsWindow::OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT trackPos = MAX_ZOOM_TRACK - (INT)trackbarZoom.SendMessage(TBM_GETPOS, 0, 0);
    canvasWindow.zoomTo(MIN_ZOOM << trackPos);

    INT zoomRate = toolsModel.GetZoom();

    CStringW strZoom;
    if (zoomRate % 10 == 0)
        strZoom.Format(L"%d%%", zoomRate / 10);
    else
        strZoom.Format(L"%d.%d%%", zoomRate / 10, zoomRate % 10);

    ::SendMessageW(g_hStatusBar, SB_SETTEXT, 1, (LPARAM)(LPCWSTR)strZoom);

    OnToolsModelZoomChanged(nMsg, wParam, lParam, bHandled);
    return 0;
}

LRESULT CToolSettingsWindow::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR *pnmhdr = (NMHDR*)lParam;
    if (pnmhdr->code == NM_CUSTOMDRAW)
    {
        NMCUSTOMDRAW *pCustomDraw = (NMCUSTOMDRAW*)pnmhdr;
        pCustomDraw->uItemState &= ~CDIS_FOCUS; // Do not draw the focus
    }
    return 0;
}

VOID CToolSettingsWindow::calculateTwoBoxes(CRect& rect1, CRect& rect2)
{
    CRect rcClient;
    GetClientRect(&rcClient);
    rcClient.InflateRect(-MARGIN1, -MARGIN1);

    INT yCenter = (rcClient.top + rcClient.bottom) / 2;
    rect1.SetRect(rcClient.left, rcClient.top, rcClient.right, yCenter);
    rect2.SetRect(rcClient.left, yCenter, rcClient.right, rcClient.bottom);

    rect1.InflateRect(-MARGIN2, -MARGIN2);
    rect2.InflateRect(-MARGIN2, -MARGIN2);
}

LRESULT CToolSettingsWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CRect rect1, rect2;
    calculateTwoBoxes(rect1, rect2);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    if (toolsModel.GetActiveTool() != TOOL_ZOOM)
        ::DrawEdge(hdc, &rect1, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);

    if (toolsModel.GetActiveTool() >= TOOL_RECT)
        ::DrawEdge(hdc, &rect2, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);

    rect1.InflateRect(-MARGIN2, -MARGIN2);
    rect2.InflateRect(-MARGIN2, -MARGIN2);
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            drawTrans(hdc, &rect1);
            break;
        case TOOL_RUBBER:
            drawRubber(hdc, &rect1);
            break;
        case TOOL_BRUSH:
            drawBrush(hdc, &rect1);
            break;
        case TOOL_AIRBRUSH:
            drawAirBrush(hdc, &rect1);
            break;
        case TOOL_LINE:
        case TOOL_BEZIER:
            drawLine(hdc, &rect1);
            break;
        case TOOL_RECT:
        case TOOL_SHAPE:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            drawBox(hdc, &rect1);
            drawLine(hdc, &rect2);
            break;
        case TOOL_FILL:
        case TOOL_COLOR:
        case TOOL_ZOOM:
        case TOOL_PEN:
            break;
    }
    EndPaint(&ps);
    return 0;
}

LRESULT CToolSettingsWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    CRect rect1, rect2;
    calculateTwoBoxes(rect1, rect2);
    RECT rects[12];

    INT iItem;
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            iItem = getTransRects(rects, &rect1, &pt);
            if (iItem != -1)
                toolsModel.SetBackgroundTransparent(iItem);
            break;
        case TOOL_RUBBER:
            iItem = getRubberRects(rects, &rect1, &pt);
            if (iItem != -1)
                toolsModel.SetRubberRadius(iItem + 2);
            break;
        case TOOL_BRUSH:
            iItem = getBrushRects(rects, &rect1, &pt);
            if (iItem != -1)
            {
                const BrushStyleAndWidth& data = c_BrushPresets[iItem];
                toolsModel.SetBrushStyle(data.style);
                toolsModel.SetBrushWidth(data.width);
            }
            break;
        case TOOL_AIRBRUSH:
            iItem = getAirBrushRects(rects, &rect1, &pt);
            if (iItem != -1)
                toolsModel.SetAirBrushRadius(s_AirRadius[iItem]);
            break;
        case TOOL_LINE:
        case TOOL_BEZIER:
            iItem = getLineRects(rects, &rect1, &pt);
            if (iItem != -1)
                toolsModel.SetLineWidth(iItem + 1);
            break;
        case TOOL_RECT:
        case TOOL_SHAPE:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            iItem = getBoxRects(rects, &rect1, &pt);
            if (iItem != -1)
                toolsModel.SetShapeStyle(iItem);

            iItem = getLineRects(rects, &rect2, &pt);
            if (iItem != -1)
                toolsModel.SetLineWidth(iItem + 1);
            break;
        case TOOL_FILL:
        case TOOL_COLOR:
        case TOOL_ZOOM:
        case TOOL_PEN:
            break;
    }

    ::SetCapture(::GetParent(m_hWnd));
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
    int tbPos = MIN_ZOOM_TRACK;
    int tempZoom = toolsModel.GetZoom();

    while (tempZoom > MIN_ZOOM)
    {
        tbPos++;
        tempZoom = tempZoom >> 1;
    }

    trackbarZoom.SendMessage(TBM_SETPOS, TRUE, MAX_ZOOM_TRACK - tbPos);
    return 0;
}
