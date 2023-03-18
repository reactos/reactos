/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/toolsettings.cpp
 * PURPOSE:     Window procedure of the tool settings window
 * PROGRAMMERS: Benedikt Freisen
 *              Stanislav Motylkov
 *              Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#define CX_TRANS_ICON 40
#define CY_TRANS_ICON 30
#define BOX_MARGIN 2

static const INT s_AirRadius[4] = { 5, 8, 3, 12 };

/* FUNCTIONS ********************************************************/

static INT
getSplitRects(RECT *arc, INT cColumns, INT cRows, LPCRECT prc, LPPOINT ppt)
{
    INT cx = prc->right - prc->left, cy = prc->bottom - prc->top;
    for (INT i = 0, iRow = 0; iRow < cRows; ++iRow)
    {
        for (INT iColumn = 0; iColumn < cColumns; ++iColumn)
        {
            RECT& rc = arc[i];
            rc.left = prc->left + (iColumn * cx / cColumns);
            rc.top = prc->top + (iRow * cy / cRows);
            rc.right = prc->left + ((iColumn + 1) * cx / cColumns);
            rc.bottom = prc->top + ((iRow + 1) * cy / cRows);
            if (ppt && ::PtInRect(&rc, pt))
                return i;
            ++i;
        }
    }
    return -1;
}

static inline INT getTransRects(RECT arc[2], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(arc, 1, 2, prc, ppt);
}

VOID CToolSettingsWindow::drawTrans(HDC hdc, LPCRECT prc)
{
    RECT rc[2];
    getTransRects(rc, prc);

    ::FillRect(hdc, &rc[toolsModel.IsBackgroundTransparent()], ::GetSysColorBrush(COLOR_HIGHLIGHT));
    ::DrawIconEx(hdc, rc[0].left, rc[0].top, m_hNontranspIcon,
                 CX_TRANS_ICON, CY_TRANS_ICON, 0, NULL, DI_NORMAL);
    ::DrawIconEx(hdc, rc[1].left, rc[1].top, m_hTranspIcon,
                 CX_TRANS_ICON, CY_TRANS_ICON, 0, NULL, DI_NORMAL);
}

static inline INT getRubberRects(RECT arc[4], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(arc, 1, 4, prc, ppt);
}

VOID CToolSettingsWindow::drawRubber(HDC hdc, LPCRECT prc)
{
    RECT arc[4], rcRubber;
    getRubberRects(arc, prc);
    INT xCenter = (prc->left + prc->right) / 2;
    for (INT i = 0; i < 4; i++)
    {
        INT iColor, radius = i + 2;
        if (toolsModel.GetRubberRadius() == radius)
        {
            ::FillRect(hdc, &arc[i], ::GetSysColorBrush(COLOR_HIGHLIGHT));
            iColor = COLOR_HIGHLIGHTTEXT;
        }
        else
        {
            iColor = COLOR_WINDOWTEXT;
        }

        INT yCenter = (arc[i].top + arc[i].bottom) / 2;
        rcRubber.left = xCenter - radius;
        rcRubber.top = yCenter - radius;
        rcRubber.right = rcRubber.left + radius * 2;
        rcRubber.bottom = rcRubber.top + radius * 2;
        ::FillRect(hdc, &rcRubber, GetSysColorBrush(iColor));
    }
}

static inline INT getBrushRects(RECT arc[12], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(arc, 3, 4, prc, ppt);
}

VOID CToolSettingsWindow::drawBrush(HDC hdc, LPCRECT prc)
{
    RECT arc[12];
    getBrushRects(arc, prc);

    ::FillRect(hdc, &arc[toolsModel.GetBrushStyle()], ::GetSysColorBrush(COLOR_HIGHLIGHT));

    for (INT i = 0; i < 12; i++)
    {
        RECT rcItem = arc[i];
        INT x = (rcItem.left + rcItem.right) / 2, y = (rcItem.top + rcItem.bottom) / 2;
        INT iColor;
        if (i == toolsModel.GetBrushStyle())
            iColor = COLOR_HIGHLIGHTTEXT;
        else
            iColor = COLOR_WINDOWTEXT;
        Brush(hdc, x, y, x, y, ::GetSysColor(iColor), i);
    }
}

static inline INT getLineRects(RECT arc[5], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(arc, 1, 5, prc, ppt);
}

VOID CToolSettingsWindow::drawLine(HDC hdc, LPCRECT prc)
{
    RECT arc[5];
    getLineRects(arc, prc);

    for (INT i = 0; i < 5; i++)
    {
        INT penWidth = i + 1;
        RECT rcLine = arc[i];
        ::InflateRect(&rcLine, -2, 0);
        rcLine.top = (rcLine.top + rcLine.bottom - penWidth) / 2;
        rcLine.bottom = rcLine.top + penWidth;
        if (toolsModel.GetLineWidth() == penWidth)
        {
            ::FillRect(hdc, &arc[i], ::GetSysColorBrush(COLOR_HIGHLIGHT));
            ::FillRect(hdc, &rcLine, ::GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            ::FillRect(hdc, &rcLine, ::GetSysColorBrush(COLOR_WINDOWTEXT));
        }
    }
}

static VOID getAirBrushRects(RECT arc[4], LPCRECT prc)
{
    INT cx = (prc->right - prc->left), cy = (prc->bottom - prc->top);

    arc[0] = arc[1] = arc[2] = arc[3] = *prc;

    arc[0].right = arc[1].left = prc->left + cx * 3 / 8;
    arc[0].bottom = arc[1].bottom = prc->top + cy / 2;

    arc[2].top = arc[3].top = prc->top + cy / 2;
    arc[2].right = arc[3].left = prc->left + cx * 2 / 8;
}

VOID CToolSettingsWindow::drawAirBrush(HDC hdc, LPCRECT prc)
{
    RECT arc[4];
    getAirBrushRects(arc, prc);

    srand(0);
    for (INT i = 0; i < 4; ++i)
    {
        RECT& rc = arc[i];
        INT x = (rc.left + rc.right) / 2;
        INT y = (rc.top + rc.bottom) / 2;
        BOOL bHigh = (s_AirRadius[i] == toolsModel.GetAirBrushWidth());
        if (bHigh)
        {
            ::FillRect(hdc, &rc, ::GetSysColorBrush(COLOR_HIGHLIGHT));
            Airbrush(hdc, x, y, ::GetSysColor(COLOR_HIGHLIGHTTEXT), s_AirRadius[i]);
        }
        else
        {
            Airbrush(hdc, x, y, ::GetSysColor(COLOR_WINDOWTEXT), s_AirRadius[i]);
        }
    }
}

static inline INT getBoxRects(RECT arc[3], LPCRECT prc, LPPOINT ppt = NULL)
{
    return getSplitRects(arc, 1, 3, prc, ppt);
}

VOID CToolSettingsWindow::drawBox(HDC hdc, LPCRECT prc)
{
    RECT arc[3];
    getBoxRects(arc, prc);

    for (INT iItem = 0; iItem < 3; ++iItem)
    {
        RECT& rcItem = arc[iItem];

        if (toolsModel.GetShapeStyle() == iItem)
            ::FillRect(hdc, &rcItem, ::GetSysColorBrush(COLOR_HIGHLIGHT));

        ::InflateRect(&rcItem, -5, -5);

        if (iItem <= 1)
        {
            INT iPenColor;
            if (toolsModel.GetShapeStyle() == iItem)
                iPenColor = COLOR_HIGHLIGHTTEXT;
            else
                iPenColor = COLOR_WINDOWTEXT;
            HGDIOBJ hOldBrush;
            if (iItem == 0)
                hOldBrush = ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
            else
                hOldBrush = ::SelectObject(hdc, ::GetSysColorBrush(COLOR_APPWORKSPACE));
            HGDIOBJ hOldPen = ::SelectObject(hdc, ::CreatePen(PS_SOLID, 1, GetSysColor(iPenColor)));
            ::Rectangle(hdc, rcItem.left, rcItem.top, rcItem.right, rcItem.bottom);
            ::SelectObject(hdc, hOldBrush);
            ::DeleteObject(::SelectObject(hdc, hOldPen));
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

LRESULT CToolSettingsWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, WINBOOL& bHandled)
{
    /* preloading the draw transparent/nontransparent icons for later use */
    m_hNontranspIcon = (HICON)LoadImage(hProgInstance, MAKEINTRESOURCE(IDI_NONTRANSPARENT),
                                        IMAGE_ICON, CX_TRANS_ICON, CY_TRANS_ICON, LR_DEFAULTCOLOR);
    m_hTranspIcon = (HICON)LoadImage(hProgInstance, MAKEINTRESOURCE(IDI_TRANSPARENT),
                                     IMAGE_ICON, CX_TRANS_ICON, CY_TRANS_ICON, LR_DEFAULTCOLOR);

    RECT trackbarZoomPos = {1, 1, 1 + 40, 1 + 64};
    trackbarZoom.Create(TRACKBAR_CLASS, m_hWnd, trackbarZoomPos, NULL, WS_CHILD | TBS_VERT | TBS_AUTOTICKS);
    trackbarZoom.SendMessage(TBM_SETRANGE, (WPARAM) TRUE, MAKELPARAM(0, 6));
    trackbarZoom.SendMessage(TBM_SETPOS, (WPARAM) TRUE, (LPARAM) 3);
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
    if (!zoomTo(125 << trackbarZoom.SendMessage(TBM_GETPOS, 0, 0), 0, 0))
    {
        OnToolsModelZoomChanged(nMsg, wParam, lParam, bHandled);
    }
    return 0;
}

LRESULT CToolSettingsWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rcClient;
    GetClientRect(&rcClient);
    RECT rect1 = { 0, 0, rcClient.right, rcClient.bottom / 2 };
    RECT rect2 = { 0, rcClient.bottom / 2, rcClient.right, rcClient.bottom };
    ::InflateRect(&rect1, -BOX_MARGIN, -BOX_MARGIN);
    ::InflateRect(&rect2, -BOX_MARGIN, -BOX_MARGIN);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        ::DrawEdge(hdc, &rect1, BDR_SUNKENOUTER, BF_RECT);
    else
        ::DrawEdge(hdc, &rect1, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);

    if (toolsModel.GetActiveTool() >= TOOL_RECT)
        ::DrawEdge(hdc, &rect2, BDR_SUNKENOUTER, BF_RECT | BF_MIDDLE);

    ::InflateRect(&rect1, -BOX_MARGIN, -BOX_MARGIN);
    ::InflateRect(&rect2, -BOX_MARGIN, -BOX_MARGIN);
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

    RECT rcClient, arc[12];
    GetClientRect(&rcClient);

    RECT rect1 = { 0, 0, rcClient.right, rcClient.bottom / 2 };
    RECT rect2 = { 0, rcClient.bottom / 2, rcClient.right, rcClient.bottom };
    ::InflateRect(&rect1, -BOX_MARGIN * 2, -BOX_MARGIN * 2);
    ::InflateRect(&rect2, -BOX_MARGIN * 2, -BOX_MARGIN * 2);

    INT iItem;
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_TEXT:
            iItem = getTransRects(arc, &rect1);
            if (iItem != -1)
                toolsModel.SetBackgroundTransparent(iItem);
            break;
        case TOOL_RUBBER:
            iItem = getRubberRects(arc, &rect1);
            if (iItem != -1)
                toolsModel.SetRubberRadius(iItem + 2);
            break;
        case TOOL_BRUSH:
            iItem = getBrushRects(arc, &rect1);
            if (iItem != -1)
                toolsModel.SetBrushStyle(iItem);
            break;
        case TOOL_AIRBRUSH:
            iItem = getAirBrushRects(arc, &rect1);
            if (iItem != -1)
                toolsModel.SetAirBrushWidth(s_AirRadius[iItem]);
            break;
        case TOOL_LINE:
        case TOOL_BEZIER:
            iItem = getLineRects(arc, &rect1);
            if (iItem != -1)
                toolsModel.SetLineWidth(iItem + 1);
            break;
        case TOOL_RECT:
        case TOOL_SHAPE:
        case TOOL_ELLIPSE:
        case TOOL_RRECT:
            iItem = getBoxRects(arc, &rect1);
            if (iItem != -1)
                toolsModel.SetShapeStyle(iItem);

            iItem = getLineRects(arc, &rect2);
            if (iItem != -1)
                toolsModel.SetLineWidth(i + 1);
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
