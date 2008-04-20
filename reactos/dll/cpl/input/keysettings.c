/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/keysettings.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 *                  Colin Finck
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

INT_PTR CALLBACK
KeySettingsDlgProc(HWND hDlg,
                   UINT message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CHANGE_KEY_SEQ_BTN:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_CHANGE_KEY_SEQ),
                              hDlg,
                              ChangeKeySeqDlgProc);
                    break;

                case IDOK:
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
