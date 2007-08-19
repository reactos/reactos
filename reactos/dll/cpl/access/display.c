/* $Id$
 *
 * PROJECT:         ReactOS Accessibility Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/display.c
 * PURPOSE:         Display-related accessibility settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 *                  Copyright 2007 Eric Kohl
 */

#include <windows.h>
#include <stdlib.h>
#include <commctrl.h>
#include <prsht.h>
#include <tchar.h>
#include "resource.h"
#include "access.h"

typedef struct _GLOBAL_DATA
{
    HIGHCONTRAST highContrast;
} GLOBAL_DATA, *PGLOBAL_DATA;


static VOID
FillColorSchemeComboBox(HWND hwnd)
{
    TCHAR szValue[128];
    DWORD dwDisposition;
    DWORD dwLength;
    HKEY hKey;
    LONG lError;
    INT i;

    lError = RegCreateKeyEx(HKEY_CURRENT_USER,
                            _T("Control Panel\\Appearance\\Schemes"),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
                            NULL,
                            &hKey,
                            &dwDisposition);
    if (lError != ERROR_SUCCESS)
        return;

    for (i = 0; ; i++)
    {
        dwLength = 128;
        lError = RegEnumValue(hKey,
                              i,
                              szValue,
                              &dwLength, NULL, NULL, NULL, NULL);
        if (lError == ERROR_NO_MORE_ITEMS)
            break;

        SendMessage(hwnd,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)szValue);
    }

    RegCloseKey(hKey);
}


INT_PTR CALLBACK
HighContrastDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            CheckDlgButton(hwndDlg,
                           IDC_CONTRAST_ACTIVATE_CHECK,
                           pGlobalData->highContrast.dwFlags & HCF_HOTKEYACTIVE ? BST_CHECKED : BST_UNCHECKED);

            FillColorSchemeComboBox(GetDlgItem(hwndDlg, IDC_CONTRAST_COMBO));

            SendDlgItemMessage(hwndDlg,
                               IDC_CONTRAST_COMBO,
                               CB_SELECTSTRING,
                               -1,
                               (LPARAM)pGlobalData->highContrast.lpszDefaultScheme);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CONTRAST_ACTIVATE_CHECK:
                    pGlobalData->highContrast.dwFlags ^= HCF_HOTKEYACTIVE;
                    break;

                case IDC_CONTRAST_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        INT nSel;

                        nSel = SendDlgItemMessage(hwndDlg, IDC_CONTRAST_COMBO,
                                                  CB_GETCURSEL, 0, 0);
                        SendDlgItemMessage(hwndDlg, IDC_CONTRAST_COMBO,
                                           CB_GETLBTEXT, nSel,
                                           (LPARAM)pGlobalData->highContrast.lpszDefaultScheme);
                    }
                    break;

                case IDOK:
                    EndDialog(hwndDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, FALSE);
                    break;

                default:
                    break;
            }
            break;
    }

    return FALSE;
}


/* Property page dialog callback */
INT_PTR CALLBACK
DisplayPageProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;
    LPPSHNOTIFY lppsn;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            if (pGlobalData == NULL)
                return FALSE;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            /* Get sticky keys information */
            pGlobalData->highContrast.cbSize = sizeof(HIGHCONTRAST);
            SystemParametersInfo(SPI_GETHIGHCONTRAST,
                                 sizeof(HIGHCONTRAST),
                                 &pGlobalData->highContrast,
                                 0);

            /* Set the checkbox */
            CheckDlgButton(hwndDlg,
                           IDC_CONTRAST_BOX,
                           pGlobalData->highContrast.dwFlags & HCF_HIGHCONTRASTON ? BST_CHECKED : BST_UNCHECKED);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CONTRAST_BOX:
                    pGlobalData->highContrast.dwFlags ^= HCF_HIGHCONTRASTON;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_CONTRAST_BUTTON:
                    if (DialogBoxParam(hApplet,
                                       MAKEINTRESOURCE(IDD_CONTRASTOPTIONS),
                                       hwndDlg,
                                       (DLGPROC)HighContrastDlgProc,
                                       (LPARAM)pGlobalData))
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                SystemParametersInfo(SPI_SETHIGHCONTRAST,
                                     sizeof(HIGHCONTRAST),
                                     &pGlobalData->highContrast,
                                     SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);
                return TRUE;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
    }

    return FALSE;
}
