/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS shutdown/logoff utility
 * FILE:            base/applications/shutdown/gui.c
 * PURPOSE:         Shows a GUI used for managing multiple remote shutdown/restarts
 * PROGRAMMERS:     Lee Schroeder
 */

#include "precomp.h"

INT_PTR CALLBACK ShutdownGuiProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch(LOWORD(wparam))
            {
                case IDC_OK:
                    EndDialog(hwnd, IDC_OK);
                    break;
                case IDC_CANCEL:
                    EndDialog(hwnd, IDC_CANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/*
 * NOTE: Until the ability to shutdown computers remotely, the GUI feature
 * can't be fully implemented.
 */
BOOL ShutdownGuiMain(struct CommandLineOptions opts)
{
    INT_PTR result = DialogBoxW(GetModuleHandle(NULL),
                                MAKEINTRESOURCEW(IDD_GUI),
                                NULL,
                                ShutdownGuiProc);

    switch (result)
    {
        case IDC_OK:
            MessageBoxW(NULL, L"This function is unimplemented.", L"Unimplemented", MB_OK);
            break;

        case IDC_CANCEL:
            /* Exits the program */
            break;

        default:
            MessageBoxW(NULL, L"Dialog Error!", L"Message", MB_OK);
            return FALSE;
    }

    return TRUE;
}

/* EOF */
