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
    TCHAR szActivePort[MAX_PATH];
    TCHAR szPort[MAX_PATH];
    BOOL bWarningSounds;
    BOOL bSoundOnActivation;
} GLOBAL_DATA, *PGLOBAL_DATA;


#define BAUDTICKS 6
static INT nBaudArray[BAUDTICKS] = {300, 1200, 2400, 4800, 9600, 19200};


INT_PTR CALLBACK
SerialKeysDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;
    INT i, n;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobalData = (PGLOBAL_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            /* Add the COM-Ports 1 - 4 to the list */
            for (i = 0; i < 4; i++)
            {
                TCHAR szBuffer[256];

                _stprintf(szBuffer, _T("COM%d"), i + 1);
                SendDlgItemMessage(hwndDlg, IDC_SERIAL_PORT_COMBO, CB_ADDSTRING, 0, (LPARAM)szBuffer);
            }

            /* Determine the current port */
            if (pGlobalData->serialKeys.lpszActivePort && pGlobalData->serialKeys.lpszActivePort[0])
            {
                i = pGlobalData->serialKeys.lpszActivePort[3] - '1';
                if (i < 0 || i > 3)
                    i = 0;
            }
            else
            {
                /* Make COM1 the default port */
                i = 0;
                _tcscpy(pGlobalData->serialKeys.lpszActivePort, _T("COM1"));
            }

            /* Set the current port */
            SendDlgItemMessage(hwndDlg, IDC_SERIAL_PORT_COMBO, CB_SETCURSEL, i, 0);

            /* Determine the current baud rate */
            n = 0;
            for (i = 0; i < BAUDTICKS; i++)
            {
                TCHAR szBuffer[256];

                _stprintf(szBuffer, _T("%d Baud"), nBaudArray[i]);
                SendDlgItemMessage(hwndDlg, IDC_SERIAL_BAUD_COMBO, CB_ADDSTRING, 0, (LPARAM)szBuffer);

                if (pGlobalData->serialKeys.iBaudRate == nBaudArray[i])
                    n = i;
            }

            /* Set the current baud rate */
            SendDlgItemMessage(hwndDlg, IDC_SERIAL_BAUD_COMBO, CB_SETCURSEL, n, 0);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    i = SendDlgItemMessage(hwndDlg, IDC_SERIAL_PORT_COMBO, CB_GETCURSEL, 0, 0) + 1;
                    _stprintf(pGlobalData->serialKeys.lpszActivePort, _T("COM%d"), i);

                    i = SendDlgItemMessage(hwndDlg, IDC_SERIAL_BAUD_COMBO, CB_GETCURSEL, 0, 0);
                    pGlobalData->serialKeys.iBaudRate = nBaudArray[i];

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


static VOID
FillResetComboBox(HWND hwnd)
{
    TCHAR szBuffer[32];
    TCHAR szMinutes[20];
    INT i;

    if (LoadString(hApplet, IDS_MINUTES, szMinutes, 20) == 0)
        lstrcpy(szMinutes, L"Minutes");

    for (i = 0; i < 6; i++)
    {
        _stprintf(szBuffer, _T("%u %s"), (i + 1) * 5, szMinutes);
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
    pGlobalData->serialKeys.lpszActivePort = pGlobalData->szActivePort;
    pGlobalData->serialKeys.lpszPort = pGlobalData->szPort;
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
                    if (DialogBoxParam(hApplet,
                                       MAKEINTRESOURCE(IDD_SERIALKEYSOPTIONS),
                                       hwndDlg,
                                       (DLGPROC)SerialKeysDlgProc,
                                       (LPARAM)pGlobalData))
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
