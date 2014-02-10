/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/textedit.c
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

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
                    textToolText = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, sizeof(TCHAR) * textToolTextMaxLen);
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
