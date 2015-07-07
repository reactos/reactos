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

LRESULT CTextEditWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT clientRect;
    GetClientRect(&clientRect);
    ::MoveWindow(hwndEditCtl, clientRect.left, clientRect.top, RECT_WIDTH(clientRect), RECT_HEIGHT(clientRect), TRUE);
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
            textToolTextMaxLen = ::GetWindowTextLength(hwndEditCtl) + 1;
            textToolText = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(TCHAR) * textToolTextMaxLen);
            ::GetWindowText(hwndEditCtl, textToolText, textToolTextMaxLen);
            ForceRefreshSelectionContents();
            break;
        }
    }
    return 0;
}
