/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/scrollbox.cpp
 * PURPOSE:     Functionality surrounding the scroll box window class
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void
UpdateScrollbox()
{
    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    SCROLLINFO si;
    scrollboxWindow.GetClientRect(&clientRectScrollbox);
    imageArea.GetClientRect(&clientRectImageArea);
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_PAGE | SIF_RANGE;
    si.nMax   = clientRectImageArea.right + 6 - 1;
    si.nMin   = 0;
    si.nPage  = clientRectScrollbox.right;
    scrollboxWindow.SetScrollInfo(SB_HORZ, &si);
    scrollboxWindow.GetClientRect(&clientRectScrollbox);
    si.nMax   = clientRectImageArea.bottom + 6 - 1;
    si.nPage  = clientRectScrollbox.bottom;
    scrollboxWindow.SetScrollInfo(SB_VERT, &si);
    scrlClientWindow.MoveWindow(
        -scrollboxWindow.GetScrollPos(SB_HORZ), -scrollboxWindow.GetScrollPos(SB_VERT),
        max(clientRectImageArea.right + 6, clientRectScrollbox.right),
        max(clientRectImageArea.bottom + 6, clientRectScrollbox.bottom), TRUE);
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
                   -scrollboxWindow.GetScrollPos(SB_VERT), imageModel.GetWidth() * toolsModel.GetZoom() / 1000 + 6,
                   imageModel.GetHeight() * toolsModel.GetZoom() / 1000 + 6, TRUE);
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
                   -scrollboxWindow.GetScrollPos(SB_VERT), imageModel.GetWidth() * toolsModel.GetZoom() / 1000 + 6,
                   imageModel.GetHeight() * toolsModel.GetZoom() / 1000 + 6, TRUE);
    }
    return 0;
}
