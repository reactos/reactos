/* $Id$
 *
 * PROJECT:         ReactOS Accessibility Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/general.c
 * PURPOSE:         General accessibility settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 *                  Copyright 2007 Eric Kohl
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <commctrl.h>
#include <prsht.h>
#include <tchar.h>
#include "resource.h"
#include "access.h"


typedef struct _GLOBAL_DATA
{
    ACCESSTIMEOUT accessTimeout;
    SERIALKEYS serialKeys;
    BOOL bWarningSounds;
    BOOL bSoundOnActivation;
} GLOBAL_DATA, *PGLOBAL_DATA;


static VOID
FillResetComboBox(HWND hwnd)
{
    TCHAR szBuffer[16];
    INT i;

    for (i = 0; i < 6; i++)
    {
        _stprintf(szBuffer, _T("%u"), (i + 1) * 5);
        SendMessage(hwnd,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)szBuffer);
    }
}


static VOID
ReadGlobalData(PGLOBAL_DATA pGlobalData)
{
    DWORD dwDisposition;
    DWORD dwLength;
    HKEY hKey;
    LONG lError;

    /* Get access timeout information */
    pGlobalData->accessTimeout.cbSize = sizeof(ACCESSTIMEOUT);
    SystemParametersInfo(SPI_GETACCESSTIMEOUT,
                         sizeof(ACCESSTIMEOUT),
                         &pGlobalData->accessTimeout,
                         0);

    /* Get serial keys information */
    pGlobalData->serialKeys.cbSize = sizeof(SERIALKEYS);
    SystemParametersInfo(SPI_GETSERIALKEYS,
                         sizeof(SERIALKEYS),
                         &pGlobalData->serialKeys,
                         0);

    pGlobalData->bWarningSounds = TRUE;
    pGlobalData->bSoundOnActivation = TRUE;

    lError = RegCreateKeyEx(HKEY_CURRENT_USER,
                            _T("Control Panel\\Accessibility"),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_EXECUTE | KEY_QUERY_VALUE,
                            NULL,
                            &hKey,
                            &dwDisposition);
    if (lError != ERROR_SUCCESS)
        return;

    dwLength = sizeof(BOOL);
    lError = RegQueryValueEx(hKey,
                             _T("Warning Sounds"),
                             NULL,
                             NULL,
                             (LPBYTE)&pGlobalData->bWarningSounds,
                             &dwLength);
    if (lError != ERROR_SUCCESS)
        pGlobalData->bWarningSounds = TRUE;

    dwLength = sizeof(BOOL);
    lError = RegQueryValueEx(hKey,
                             _T("Sound On Activation"),
                             NULL,
                             NULL,
                             (LPBYTE)&pGlobalData->bSoundOnActivation,
                             &dwLength);
    if (lError != ERROR_SUCCESS)
        pGlobalData->bSoundOnActivation = TRUE;


    RegCloseKey(hKey);
}


static VOID
WriteGlobalData(PGLOBAL_DATA pGlobalData)
{
    DWORD dwDisposition;
    HKEY hKey;
    LONG lError;

    SystemParametersInfo(SPI_SETACCESSTIMEOUT,
                         sizeof(ACCESSTIMEOUT),
                         &pGlobalData->accessTimeout,
                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);

    SystemParametersInfo(SPI_SETSERIALKEYS,
                         sizeof(SERIALKEYS),
                         &pGlobalData->serialKeys,
                         SPIF_UPDATEINIFILE | SPIF_SENDCHANGE /*0*/);

    lError = RegCreateKeyEx(HKEY_CURRENT_USER,
                            _T("Control Panel\\Accessibility"),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &hKey,
                            &dwDisposition);
    if (lError != ERROR_SUCCESS)
        return;

    RegSetValueEx(hKey,
                  _T("Warning Sounds"),
                  0,
                  REG_DWORD,
                  (LPBYTE)&pGlobalData->bWarningSounds,
                  sizeof(BOOL));

    RegSetValueEx(hKey,
                  _T("Sound On Activation"),
                  0,
                  REG_DWORD,
                  (LPBYTE)pGlobalData->bSoundOnActivation,
                  sizeof(BOOL));

    RegCloseKey(hKey);
}


/* Property page dialog callback */
INT_PTR CALLBACK
GeneralPageProc(HWND hwndDlg,
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

            ReadGlobalData(pGlobalData);

            /* Set access timeout info */
            CheckDlgButton(hwndDlg,
                           IDC_RESET_BOX,
                           pGlobalData->accessTimeout.dwFlags & ATF_TIMEOUTON ? BST_CHECKED : BST_UNCHECKED);
            FillResetComboBox(GetDlgItem(hwndDlg, IDC_RESET_COMBO));
            SendDlgItemMessage(hwndDlg, IDC_RESET_COMBO, CB_SETCURSEL,
                               (pGlobalData->accessTimeout.iTimeOutMSec / 300000) - 1, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_COMBO),
                         pGlobalData->accessTimeout.dwFlags & ATF_TIMEOUTON ? TRUE : FALSE);

            CheckDlgButton(hwndDlg,
                           IDC_NOTIFICATION_MESSAGE,
                           pGlobalData->bWarningSounds ? BST_CHECKED : BST_UNCHECKED);

            CheckDlgButton(hwndDlg,
                           IDC_NOTIFICATION_SOUND,
                           pGlobalData->bSoundOnActivation ? BST_CHECKED : BST_UNCHECKED);

            /* Set serial keys info */
            CheckDlgButton(hwndDlg,
                           IDC_SERIAL_BOX,
                           pGlobalData->serialKeys.dwFlags & SERKF_SERIALKEYSON ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SERIAL_BOX),
                         pGlobalData->serialKeys.dwFlags & SERKF_AVAILABLE ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SERIAL_BUTTON),
                         pGlobalData->serialKeys.dwFlags & SERKF_AVAILABLE ? TRUE : FALSE);

            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RESET_BOX:
                    pGlobalData->accessTimeout.dwFlags ^= ATF_TIMEOUTON;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_COMBO),
                                 pGlobalData->accessTimeout.dwFlags & ATF_TIMEOUTON ? TRUE : FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_RESET_COMBO:
                    if (HIWORD(wParam) == CBN_CLOSEUP)
                    {
                        INT nSel;
                        nSel = SendDlgItemMessage(hwndDlg, IDC_RESET_COMBO, CB_GETCURSEL, 0, 0);
                        pGlobalData->accessTimeout.iTimeOutMSec = (ULONG)((nSel + 1) * 300000);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_NOTIFICATION_MESSAGE:
                    pGlobalData->bWarningSounds = !pGlobalData->bWarningSounds;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_NOTIFICATION_SOUND:
                    pGlobalData->bSoundOnActivation = !pGlobalData->bSoundOnActivation;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_SERIAL_BOX:
                    pGlobalData->serialKeys.dwFlags ^= SERKF_SERIALKEYSON;
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_SERIAL_BUTTON:
                    break;

                case IDC_ADMIN_LOGON_BOX:
                    break;

                case IDC_ADMIN_USERS_BOX:
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                WriteGlobalData(pGlobalData);
                return TRUE;
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;    }

    return FALSE;
}
