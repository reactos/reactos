#include <precomp.h>

INT_PTR CALLBACK
AboutDialogProc(HWND hDlg,
                UINT message,
                WPARAM wParam,
                LPARAM lParam)
{
    HWND  hLicenseEditWnd;
    HICON hIcon = NULL;
    TCHAR strLicense[700];

    switch (message)
    {
    case WM_INITDIALOG:

        hIcon = (HICON) LoadImage(hInstance,
                          MAKEINTRESOURCE(IDI_IMAGESOFTICON),
                          IMAGE_ICON,
                          16,
                          16,
                          0);

        SendMessage(hDlg,
                    WM_SETICON,
                    ICON_SMALL,
                    (LPARAM)hIcon);

        hLicenseEditWnd = GetDlgItem(hDlg,
                                     IDC_LICENSE_EDIT);

        LoadString(hInstance,
                   IDS_LICENSE,
                   strLicense,
                   ARRAYSIZE(strLicense));

        SetWindowText(hLicenseEditWnd,
                      strLicense);
        return TRUE;

    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            DestroyIcon(hIcon);
            EndDialog(hDlg,
                      LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return FALSE;
}
