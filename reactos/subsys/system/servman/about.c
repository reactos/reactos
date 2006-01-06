#include "servman.h"

extern HINSTANCE hInstance;

BOOL CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hLicenseEditWnd;
    TCHAR    strLicense[0x1000];

    switch (message)
    {
    case WM_INITDIALOG:

        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE_EDIT);

        LoadString(hInstance, IDS_LICENSE, strLicense, 0x1000);

        SetWindowText(hLicenseEditWnd, strLicense);

        return TRUE;

    case WM_COMMAND:

        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}
