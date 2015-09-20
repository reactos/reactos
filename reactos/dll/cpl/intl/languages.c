#include "intl.h"

#include <shellapi.h>

/* Property page dialog callback */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    SHELLEXECUTEINFOW shInputDll;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                /* If "detail" button pressed */
                case IDC_DETAIL_BUTTON:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        memset(&shInputDll, 0x0, sizeof(SHELLEXECUTEINFOW));
                        shInputDll.cbSize = sizeof(shInputDll);
                        shInputDll.hwnd = hwndDlg;
                        shInputDll.lpVerb = L"open";
                        shInputDll.lpFile = L"RunDll32.exe";
                        shInputDll.lpParameters = L"shell32.dll,Control_RunDLL input.dll";
                        if (ShellExecuteExW(&shInputDll) == 0)
                        {
                            PrintErrorMsgBox(IDS_ERROR_INPUT_DLL);
                        }
                    }
                    break;
            }
            break;
    }
    return FALSE;
}

/* EOF */
