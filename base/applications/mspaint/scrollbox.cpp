/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/scrollbox.cpp
 * PURPOSE:     Functionality surrounding the scroll box window class
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include <atltypes.h>

/*
 * Scrollbar functional modes:
 * 0  view < canvas
 * 1  view < canvas + scroll width
 * 2  view >= canvas + scroll width
 *
 * Matrix of scrollbar presence (VERTICAL,HORIZONTAL) given by
 * vertical & horizontal scrollbar modes (view:canvas ratio):
 *
 *           horizontal mode
 *             |      0      |      1      |      2
 * vertical ---+-------------+-------------+------------
 *   mode    0 |  TRUE,TRUE  |  TRUE,TRUE  |  TRUE,FALSE
 *          ---+-------------+-------------+------------
 *           1 |  TRUE,TRUE  | FALSE,FALSE | FALSE,FALSE
 *          ---+-------------+-------------+------------
 *           2 | FALSE,TRUE  | FALSE,FALSE | FALSE,FALSE
 */

struct ScrollbarPresence
{
    BOOL bVert;
    BOOL bHoriz;
};

CONST ScrollbarPresence sp_mx[3][3] =
{
    { {  TRUE,TRUE  }, {  TRUE,TRUE  }, {  TRUE,FALSE } },
    { {  TRUE,TRUE  }, { FALSE,FALSE }, { FALSE,FALSE } },
    { { FALSE,TRUE  }, { FALSE,FALSE }, { FALSE,FALSE } }
};

CONST INT HSCROLL_WIDTH = ::GetSystemMetrics(SM_CYHSCROLL);
CONST INT VSCROLL_WIDTH = ::GetSystemMetrics(SM_CXVSCROLL);


/* FUNCTIONS ********************************************************/

void
UpdateScrollbox()
{
    CRect tempRect;
    CSize sizeImageArea;
    CSize sizeScrollBox;
    INT vmode, hmode;
    SCROLLINFO si;

    scrollboxWindow.GetWindowRect(&tempRect);
    sizeScrollBox = CSize(tempRect.Width(), tempRect.Height());

    imageArea.GetClientRect(&tempRect);
    sizeImageArea = CSize(tempRect.Width(), tempRect.Height());
    sizeImageArea += CSize(GRIP_SIZE * 2, GRIP_SIZE * 2);

    /* show/hide the scrollbars */
    vmode = (sizeScrollBox.cy < sizeImageArea.cy ? 0 :
                (sizeScrollBox.cy < sizeImageArea.cy + HSCROLL_WIDTH ? 1 : 2));
    hmode = (sizeScrollBox.cx < sizeImageArea.cx ? 0 :
                (sizeScrollBox.cx < sizeImageArea.cx + VSCROLL_WIDTH ? 1 : 2));
    scrollboxWindow.ShowScrollBar(SB_VERT, sp_mx[vmode][hmode].bVert);
    scrollboxWindow.ShowScrollBar(SB_HORZ, sp_mx[vmode][hmode].bHoriz);

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_PAGE | SIF_RANGE;
    si.nMin   = 0;

    si.nMax   = sizeImageArea.cx +
                    (sp_mx[vmode][hmode].bVert == TRUE ? HSCROLL_WIDTH : 0);
    si.nPage  = sizeScrollBox.cx;
    scrollboxWindow.SetScrollInfo(SB_HORZ, &si);

    si.nMax   = sizeImageArea.cy +
                    (sp_mx[vmode][hmode].bHoriz == TRUE ? VSCROLL_WIDTH : 0);
    si.nPage  = sizeScrollBox.cy;
    scrollboxWindow.SetScrollInfo(SB_VERT, &si);

    scrlClientWindow.MoveWindow(-scrollboxWindow.GetScrollPos(SB_HORZ),
                                -scrollboxWindow.GetScrollPos(SB_VERT),
                                sizeImageArea.cx, sizeImageArea.cy, TRUE);
}

LRESULT CScrollboxWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd == scrollboxWindow.m_hWnd)
    {
        UpdateScrollbox();
    }
    return 0;
}

LRESULT CScrollboxWindow::OnHScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd == scrollboxWindow.m_hWnd)
    {
        SCROLLINFO si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL;
        scrollboxWindow.GetScrollInfo(SB_HORZ, &si);
        switch (LOWORD(wParam))
        {
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                si.nPos = HIWORD(wParam);
                break;
            case SB_LINELEFT:
                si.nPos -= 5;
                break;
            case SB_LINERIGHT:
                si.nPos += 5;
                break;
            case SB_PAGELEFT:
                si.nPos -= si.nPage;
                break;
            case SB_PAGERIGHT:
                si.nPos += si.nPage;
                break;
        }
        scrollboxWindow.SetScrollInfo(SB_HORZ, &si);
        scrlClientWindow.MoveWindow(-scrollboxWindow.GetScrollPos(SB_HORZ),
                   -scrollboxWindow.GetScrollPos(SB_VERT),
                   Zoomed(imageModel.GetWidth()) + 2 * GRIP_SIZE,
                   Zoomed(imageModel.GetHeight()) + 2 * GRIP_SIZE, TRUE);
    }
    return 0;
}

LRESULT CScrollboxWindow::OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd == scrollboxWindow.m_hWnd)
    {
        SCROLLINFO si;
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL;
        scrollboxWindow.GetScrollInfo(SB_VERT, &si);
        switch (LOWORD(wParam))
        {
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                si.nPos = HIWORD(wParam);
                break;
            case SB_LINEUP:
                si.nPos -= 5;
                break;
            case SB_LINEDOWN:
                si.nPos += 5;
                break;
            case SB_PAGEUP:
                si.nPos -= si.nPage;
                break;
            case SB_PAGEDOWN:
                si.nPos += si.nPage;
                break;
        }
        scrollboxWindow.SetScrollInfo(SB_VERT, &si);
        scrlClientWindow.MoveWindow(-scrollboxWindow.GetScrollPos(SB_HORZ),
                   -scrollboxWindow.GetScrollPos(SB_VERT),
                   Zoomed(imageModel.GetWidth()) + 2 * GRIP_SIZE,
                   Zoomed(imageModel.GetHeight()) + 2 * GRIP_SIZE, TRUE);
    }
    return 0;
}

LRESULT CScrollboxWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    selectionWindow.ShowWindow(SW_HIDE);

    switch (toolsModel.GetActiveTool())
    {
        case TOOL_BEZIER:
        case TOOL_SHAPE:
            if (ToolBase::pointSP != 0)
            {
                toolsModel.OnCancelDraw();
                imageArea.Invalidate();
            }
            break;

        default:
            break;
    }

    toolsModel.resetTool();  // resets the point-buffer of the polygon and bezier functions
    return 0;
}

LRESULT CScrollboxWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}
