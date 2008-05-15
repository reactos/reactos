/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/changekeyseq.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

BOOL
GetHotkeys(LPTSTR szHotkey, LPTSTR szLangHotkey, LPTSTR szLayoutHotkey)
{
    HKEY hKey;
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Toggle"),
                     0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szHotkey);
        if (RegQueryValueEx(hKey, _T("Hotkey"), NULL, NULL,
                            (LPBYTE)szHotkey, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        dwSize = sizeof(szLangHotkey);
        if (RegQueryValueEx(hKey, _T("Language Hotkey"), NULL, NULL,
                            (LPBYTE)szLangHotkey, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        dwSize = sizeof(szLayoutHotkey);
        if (RegQueryValueEx(hKey, _T("Layout Hotkey"), NULL, NULL,
                            (LPBYTE)szLayoutHotkey, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey(hKey);
    }
    else return FALSE;

    return TRUE;
}

static VOID
InitChangeKeySeqDlg(HWND hDlg)
{
    TCHAR szHotkey[1 + 1], szLangHotkey[1 + 1], szLayoutHotkey[1 + 1];

    if (!GetHotkeys(szHotkey, szLangHotkey, szLayoutHotkey))
        return;

    if (_tcscmp(szLangHotkey, _T("3")) == 0)
    {
        SendDlgItemMessage(hDlg, IDC_CTRL_LANG, BM_SETCHECK, 1, 1);
        EnableWindow(GetDlgItem(hDlg, IDC_CTRL_LANG), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LEFT_ALT_LANG), FALSE);
    }
    else
    {
        SendDlgItemMessage(hDlg, IDC_SWITCH_INPUT_LANG_CB, BM_SETCHECK, 1, 1);

        if (_tcscmp(szLangHotkey, _T("1")) == 0)
            SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LANG, BM_SETCHECK, 1, 1);
        else
            SendDlgItemMessage(hDlg, IDC_CTRL_LANG, BM_SETCHECK, 1, 1);
    }

    if (_tcscmp(szLayoutHotkey, _T("3")) == 0)
    {
        SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LAYOUT, BM_SETCHECK, 1, 1);
        EnableWindow(GetDlgItem(hDlg, IDC_CTRL_LAYOUT), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_LEFT_ALT_LAYOUT), FALSE);
    }
    else
    {
        SendDlgItemMessage(hDlg, IDC_SWITCH_KBLAYOUTS_CB, BM_SETCHECK, 1, 1);

        if (_tcscmp(szLayoutHotkey, _T("1")) == 0)
            SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LAYOUT, BM_SETCHECK, 1, 1);
        else
            SendDlgItemMessage(hDlg, IDC_CTRL_LAYOUT, BM_SETCHECK, 1, 1);
    }
}

INT_PTR CALLBACK
ChangeKeySeqDlgProc(HWND hDlg,
                    UINT message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            InitChangeKeySeqDlg(hDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SWITCH_INPUT_LANG_CB:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (SendDlgItemMessage(hDlg, IDC_SWITCH_INPUT_LANG_CB, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_CTRL_LANG), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_LEFT_ALT_LANG), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_CTRL_LANG), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_LEFT_ALT_LANG), FALSE);
                        }
                    }
                    break;
                
                case IDC_SWITCH_KBLAYOUTS_CB:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (SendDlgItemMessage(hDlg, IDC_SWITCH_KBLAYOUTS_CB, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_CTRL_LAYOUT), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_LEFT_ALT_LAYOUT), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_CTRL_LAYOUT), FALSE);
                            EnableWindow(GetDlgItem(hDlg, IDC_LEFT_ALT_LAYOUT), FALSE);
                        }
                    }
                    break;

                case IDC_CTRL_LANG:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LAYOUT, BM_SETCHECK, 1, 1);
                        SendDlgItemMessage(hDlg, IDC_CTRL_LAYOUT, BM_SETCHECK, 0, 0);
                    }
                    break;

                case IDC_LEFT_ALT_LANG:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        SendDlgItemMessage(hDlg, IDC_CTRL_LAYOUT, BM_SETCHECK, 1, 1);
                        SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LAYOUT, BM_SETCHECK, 0, 0);
                    }
                    break;

                case IDC_CTRL_LAYOUT:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LANG, BM_SETCHECK, 1, 1);
                        SendDlgItemMessage(hDlg, IDC_CTRL_LANG, BM_SETCHECK, 0, 0);
                    }
                    break;

                case IDC_LEFT_ALT_LAYOUT:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        SendDlgItemMessage(hDlg, IDC_CTRL_LANG, BM_SETCHECK, 1, 1);
                        SendDlgItemMessage(hDlg, IDC_LEFT_ALT_LANG, BM_SETCHECK, 0, 0);
                    }
                    break;

                case IDOK:
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
