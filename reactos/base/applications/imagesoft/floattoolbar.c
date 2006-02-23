#include "imagesoft.h"

HWND hFloatTool;


BOOL ShowHideToolbar(HWND hwnd)
{
    static BOOL Hidden = FALSE;

    ShowWindow(hwnd, Hidden ? SW_SHOW : SW_HIDE);
    Hidden = ~Hidden;

    return Hidden;
}


BOOL CALLBACK ToolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    hFloatTool = hwnd;

    switch(Message)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
                ShowHideToolbar(hwnd);

            switch(LOWORD(wParam))
            {
                case IDC_PRESS:
                    MessageBox(hwnd, _T("Kapow!"), _T("Batman says"),
                    MB_OK | MB_ICONEXCLAMATION);
                break;
            }
        break;

        default:
        return FALSE;
    }
    return TRUE;
}
