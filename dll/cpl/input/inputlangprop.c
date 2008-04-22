/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/inputlangprop.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 *                  Colin Finck
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

INT_PTR CALLBACK
InputLangPropDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            CreateKeyboardLayoutList();
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    break;

                case IDCANCEL:
                    EndDialog(hDlg,LOWORD(wParam));
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
