#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

/* Property page dialog callback */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    SHELLEXECUTEINFO shInputDll;

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
                        memset(&shInputDll, 0x0, sizeof(SHELLEXECUTEINFO));
                        shInputDll.cbSize = sizeof(shInputDll);
                        shInputDll.hwnd = hwndDlg;
                        shInputDll.lpVerb = _T("open");
                        shInputDll.lpFile = _T("RunDll32.exe");
                        shInputDll.lpParameters = _T("shell32.dll,Control_RunDLL input.dll");
                        if (ShellExecuteEx(&shInputDll) == 0)
                        {
                            MessageBox(NULL,
                                       _T("Can't start input.dll"),
                                       _T("Error"),
                                       MB_OK | MB_ICONERROR);
                        }
                    }
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
