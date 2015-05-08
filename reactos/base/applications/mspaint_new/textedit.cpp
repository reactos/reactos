/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/textedit.cpp
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include "textedit.h"

/* FUNCTIONS ********************************************************/

void
RegisterWclTextEdit()
{
    WNDCLASSEX wclTextEdit;
    /* initializing and registering the window class used for the text editor */
    wclTextEdit.hInstance         = hProgInstance;
    wclTextEdit.lpszClassName     = _T("TextEdit");
    wclTextEdit.lpfnWndProc       = TextEditWinProc;
    wclTextEdit.style             = CS_DBLCLKS;
    wclTextEdit.cbSize            = sizeof(WNDCLASSEX);
    wclTextEdit.hIcon             = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
    wclTextEdit.hIconSm           = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
    wclTextEdit.hCursor           = LoadCursor(NULL, IDC_ARROW);
    wclTextEdit.lpszMenuName      = NULL;
    wclTextEdit.cbClsExtra        = 0;
    wclTextEdit.cbWndExtra        = 0;
    wclTextEdit.hbrBackground     = GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wclTextEdit);
}

LRESULT CALLBACK
TextEditWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            MoveWindow(hwndEditCtl, clientRect.left, clientRect.top, RECT_WIDTH(clientRect), RECT_HEIGHT(clientRect), TRUE);
            break;
        }
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            break;
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case EN_UPDATE:
                {
                    HeapFree(GetProcessHeap(), 0, textToolText);
                    textToolTextMaxLen = GetWindowTextLength(hwndEditCtl) + 1;
                    textToolText = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(TCHAR) * textToolTextMaxLen);
                    GetWindowText(hwndEditCtl, textToolText, textToolTextMaxLen);
                    ForceRefreshSelectionContents();
                    break;
                }
            }
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
