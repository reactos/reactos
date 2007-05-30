/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/langs.c
 * PURPOSE:         Extra parameters page
 * PROGRAMMERS:     Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* FUNCTIONS ****************************************************************/

/* Languages Parameters page dialog callback */
INT_PTR
CALLBACK
LangsOptsProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    SHELLEXECUTEINFOW shInputDll;

    UNREFERENCED_PARAMETER(wParam);
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
                            MessageBox(NULL, L"Can't start input.dll", L"Error",
                                       MB_OK | MB_ICONERROR);
                        }
                    }

                break;
            }


        break;

        case WM_NOTIFY:
        {
            LPNMHDR Lpnm = (LPNMHDR)lParam;
            /* If push apply button */
            if (Lpnm->code == (UINT)PSN_APPLY)
            {
                // TODO: Implement
            }
        }
        break;
    }
    return FALSE;
}

/* EOF */
