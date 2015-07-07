/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/textedit.cpp
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/
LRESULT CTextEditWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* creating the edit control within the editor window */
    RECT editControlPos = {0, 0, 0 + 100, 0 + 100};
    hwndEditCtl = editControl.Create(_T("EDIT"), m_hWnd, editControlPos, NULL,
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                                     WS_EX_CLIENTEDGE);
    return 0;
}

LRESULT CTextEditWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT clientRect;
    GetClientRect(&clientRect);
    editControl.MoveWindow(clientRect.left, clientRect.top, RECT_WIDTH(clientRect), RECT_HEIGHT(clientRect), TRUE);
    return 0;
}

LRESULT CTextEditWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow(SW_HIDE);
    return 0;
}

LRESULT CTextEditWindow::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch(HIWORD(wParam))
    {
        case EN_UPDATE:
        {
            HeapFree(GetProcessHeap(), 0, textToolText);
            textToolTextMaxLen = editControl.GetWindowTextLength() + 1;
            textToolText = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(TCHAR) * textToolTextMaxLen);
            editControl.GetWindowText(textToolText, textToolTextMaxLen);
            ForceRefreshSelectionContents();
            break;
        }
    }
    return 0;
}

LRESULT CTextEditWindow::OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow((wParam == TOOL_TEXT) ? SW_SHOW : SW_HIDE);
    return 0;
}
