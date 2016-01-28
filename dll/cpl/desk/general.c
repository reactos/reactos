/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/general.c
 * PURPOSE:         Advanced General settings
 */

#include "desk.h"

static VOID
InitFontSizeList(HWND hWnd)
{
    HINF hInf;
    HKEY hKey;
    HWND hFontSize;
    INFCONTEXT Context;
    int i, ci = 0;
    DWORD dwSize, dwValue, dwType;

    hFontSize = GetDlgItem(hWnd, IDC_FONTSIZE_COMBO);

    hInf = SetupOpenInfFile(_T("font.inf"), NULL,
                            INF_STYLE_WIN4, NULL);

    if (hInf != INVALID_HANDLE_VALUE)
    {
        if (SetupFindFirstLine(hInf, _T("Font Sizes"), NULL, &Context))
        {
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts"),
                             0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                dwSize = MAX_PATH;
                dwType = REG_DWORD;

                if (!RegQueryValueEx(hKey, _T("LogPixels"), NULL,
                                    &dwType, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
                {
                    dwValue = 0;
                }

                RegCloseKey(hKey);
            }

            for (;;)
            {
                TCHAR Buffer[LINE_LEN];
                TCHAR Desc[LINE_LEN];

                if (SetupGetStringField(&Context, 0, Buffer, sizeof(Buffer) / sizeof(TCHAR), NULL) &&
                    SetupGetIntField(&Context, 1, &ci))
                {
                    _stprintf(Desc, _T("%s (%d DPI)"), Buffer, ci);
                    i = SendMessage(hFontSize, CB_ADDSTRING, 0, (LPARAM)Desc);
                    if (i != CB_ERR)
                        SendMessage(hFontSize, CB_SETITEMDATA, (WPARAM)i, (LPARAM)ci);

                    if ((int)dwValue == ci)
                    {
                        SendMessage(hFontSize, CB_SETCURSEL, (WPARAM)i, 0);
                        SetWindowText(GetDlgItem(hWnd, IDC_FONTSIZE_CUSTOM), Desc);
                    }
                    else
                        SendMessage(hFontSize, CB_SETCURSEL, 0, 0);
                }

                if (!SetupFindNextLine(&Context, &Context))
                {
                    break;
                }
            }
        }
    }

    SetupCloseInfFile(hInf);
}

static VOID
InitRadioButtons(HWND hWnd)
{
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\Display"),
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR szBuf[64];
        DWORD dwSize = 64;

        if (RegQueryValueEx(hKey, _T("DynaSettingsChange"), 0, NULL,
                            (LPBYTE)szBuf, &dwSize) == ERROR_SUCCESS)
        {
            switch (_ttoi(szBuf))
            {
                case 0:
                    SendDlgItemMessage(hWnd, IDC_RESTART_RB, BM_SETCHECK, 1, 1);
                    break;
                case 1:
                    SendDlgItemMessage(hWnd, IDC_WITHOUTREBOOT_RB, BM_SETCHECK, 1, 1);
                    break;
                case 3:
                    SendDlgItemMessage(hWnd, IDC_ASKME_RB, BM_SETCHECK, 1, 1);
                    break;
            }
        }
        else
            SendDlgItemMessage(hWnd, IDC_WITHOUTREBOOT_RB, BM_SETCHECK, 1, 1);

        RegCloseKey(hKey);
    }
    else
        SendDlgItemMessage(hWnd, IDC_WITHOUTREBOOT_RB, BM_SETCHECK, 1, 1);
}

INT_PTR CALLBACK
AdvGeneralPageProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PDISPLAY_DEVICE_ENTRY DispDevice = NULL;
    INT_PTR Ret = 0;

    if (uMsg != WM_INITDIALOG)
        DispDevice = (PDISPLAY_DEVICE_ENTRY)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            DispDevice = (PDISPLAY_DEVICE_ENTRY)(((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)DispDevice);

            InitFontSizeList(hwndDlg);
            InitRadioButtons(hwndDlg);

            Ret = TRUE;
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FONTSIZE_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                case IDC_RESTART_RB:
                case IDC_WITHOUTREBOOT_RB:
                case IDC_ASKME_RB:
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                break;
            }
            break;
    }

    return Ret;
}
