#include <precomp.h>


static INT_PTR CALLBACK
AboutDialogProc(HWND hDlg,
                UINT message,
                WPARAM wParam,
                LPARAM lParam)
{
    static HICON hIcon = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            HWND  hLicenseEditWnd;
            TCHAR strLicense[700];

            hIcon = LoadImage(hInstance,
                              MAKEINTRESOURCE(IDI_ICON),
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
                       sizeof(strLicense) / sizeof(TCHAR));

            SetWindowText(hLicenseEditWnd,
                          strLicense);
            return TRUE;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
            {
                DestroyIcon(hIcon);
                EndDialog(hDlg,
                          LOWORD(wParam));
                return TRUE;
            }

            break;
        }
    }

    return FALSE;
}


VOID
ShowAboutDlg(HWND hWndParent)
{
    DialogBox(hInstance,
              MAKEINTRESOURCE(IDD_ABOUTBOX),
              hWndParent,
              AboutDialogProc);
}
