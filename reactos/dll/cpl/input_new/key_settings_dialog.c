/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/key_settings_dialog.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*/

#include "input.h"


INT_PTR CALLBACK
ChangeKeySeqDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {

        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK
KeySettingsDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHANGE_KEY_SEQ_BTN:
                {
                    if (DialogBoxW(hApplet,
                                   MAKEINTRESOURCEW(IDD_CHANGE_KEY_SEQ),
                                   hwndDlg,
                                   ChangeKeySeqDialogProc) == IDOK)
                    {

                    }
                }
                break;

                case IDOK:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
