#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* Property page dialog callback */
INT_PTR CALLBACK
LanguagesPageProc(HWND hwndDlg,
	     UINT uMsg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  SHELLEXECUTEINFOW shInputDll;
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            /* If "detail" button pressed */
            case IDC_DETAIL_BUTTON:
                if(HIWORD(wParam)==BN_CLICKED)
                {
                    memset(&shInputDll, 0x0, sizeof(SHELLEXECUTEINFOW));
                    shInputDll.cbSize = sizeof(shInputDll);
                    shInputDll.hwnd = hwndDlg;
                    shInputDll.lpVerb = L"open";
                    shInputDll.lpFile = L"RunDll32.exe";
                    shInputDll.lpParameters = L"shell32.dll,Control_RunDLL input.dll";
                    if(ShellExecuteExW(&shInputDll)==0)
                    {
                        MessageBox(NULL, L"Can't start input.dll", L"Error", MB_OK | MB_ICONERROR);
                    }
                }

                break;
        }


        break;
  }
  return FALSE;
}

/* EOF */
