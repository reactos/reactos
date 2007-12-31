#include "calc.h"

#define MAX_LICENSE_SIZE 1000 // it's enought!

INT_PTR CALLBACK AboutDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TCHAR *license;

    switch (msg) {
    case WM_INITDIALOG:
        license = (TCHAR *)alloca(MAX_LICENSE_SIZE*sizeof(TCHAR));
        if (LoadString(calc.hInstance, IDS_STRING_LICENSE, license, MAX_LICENSE_SIZE))
            SendDlgItemMessage(hWnd, IDC_EDIT_LICENSE, WM_SETTEXT, 0, (LPARAM)license);
        /* Update software version */
        SendDlgItemMessage(hWnd, IDC_TEXT_VERSION, WM_GETTEXT, (WPARAM)MAX_LICENSE_SIZE, (LPARAM)license);
        _tcscat(license, CALC_VERSION);
        SendDlgItemMessage(hWnd, IDC_TEXT_VERSION, WM_SETTEXT, 0, (LPARAM)license);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDOK:
            EndDialog(hWnd, 0);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return 0;
    }
    return FALSE;
}

