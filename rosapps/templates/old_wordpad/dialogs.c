#include "precomp.h"

INT_PTR CALLBACK
NewDocSelDlgProc(HWND hDlg,
                 UINT message,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (message)
    {
        static HWND hList;

        case WM_INITDIALOG:
        {
            LPTSTR lpDocType;
            INT i;

            hList = GetDlgItem(hDlg,
                               IDC_LIST);

            for (i = IDS_DOC_TYPE_RICH_TEXT; i <= IDS_DOC_TYPE_TEXT; i++)
            {
                if (AllocAndLoadString(&lpDocType,
                                       hInstance,
                                       i))
                {
                    (void)ListBox_AddString(hList,
                                            lpDocType);
                    LocalFree((HLOCAL)lpDocType);
                }
            }

            SendMessage(hList,
                        LB_SETCURSEL,
                        0,
                        0);

            return TRUE;
        }

        case WM_COMMAND:
        {
            INT LbSel;

            if (HIWORD(wParam) == LBN_DBLCLK)
            {
                LbSel = (INT)SendMessage(hList,
                                         LB_GETCURSEL,
                                         0,
                                         0);
                EndDialog(hDlg,
                          LbSel);
                return TRUE;
            }

            switch (LOWORD(wParam))
            {
                case IDOK:
                    LbSel = (INT)SendMessage(hList,
                                             LB_GETCURSEL,
                                             0,
                                             0);
                    EndDialog(hDlg,
                              LbSel);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, -1);
                    return TRUE;
            }
        }
    }

    return FALSE;
}



INT_PTR CALLBACK
AboutDialogProc(HWND hDlg,
                UINT message,
                WPARAM wParam,
                LPARAM lParam)
{
    HWND  hLicenseEditWnd;
    static HICON hIcon = NULL;
    static LPTSTR lpLicense = NULL;

    switch (message)
    {
    case WM_INITDIALOG:

        hIcon = LoadImage(hInstance,
                          MAKEINTRESOURCE(IDI_ICON),
                          IMAGE_ICON,
                          16,
                          16,
                          0);
        if (hIcon != NULL)
        {
            SendMessage(hDlg,
                        WM_SETICON,
                        ICON_SMALL,
                        (LPARAM)hIcon);
        }

        hLicenseEditWnd = GetDlgItem(hDlg,
                                     IDC_LICENSE_EDIT);

        if (AllocAndLoadString(&lpLicense,
                               hInstance,
                               IDS_LICENSE))
        {
            SetWindowText(hLicenseEditWnd,
                          lpLicense);
        }
        return TRUE;

    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            LocalFree((HLOCAL)lpLicense);
            DestroyIcon(hIcon);
            EndDialog(hDlg,
                      LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return FALSE;
}

