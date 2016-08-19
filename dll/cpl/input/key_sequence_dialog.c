/*
* PROJECT:         input.dll
* FILE:            dll/cpl/input/key_sequence_dialog.c
* PURPOSE:         input.dll
* PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
*/

#include "input.h"


INT_PTR CALLBACK
ChangeKeySeqDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            KEY_SETTINGS *keySettings = (KEY_SETTINGS*) lParam;

            if (keySettings != NULL)
            {
                SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR) keySettings);

                if (keySettings->dwLanguage == 3)
                {
                    CheckDlgButton(hwndDlg, IDC_CTRL_LANG, BST_CHECKED);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CTRL_LANG), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT_ALT_LANG), FALSE);
                }
                else
                {
                    CheckDlgButton(hwndDlg, IDC_SWITCH_INPUT_LANG_CB, BST_CHECKED);

                    if (keySettings->dwLanguage == 1)
                    {
                        CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LANG, BST_CHECKED);
                    }
                    else
                    {
                        CheckDlgButton(hwndDlg, IDC_CTRL_LANG, BST_CHECKED);
                    }
                }

                if (keySettings->dwLayout == 3)
                {
                    CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LAYOUT, BST_CHECKED);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CTRL_LAYOUT), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT_ALT_LAYOUT), FALSE);
                }
                else
                {
                    CheckDlgButton(hwndDlg, IDC_SWITCH_KBLAYOUTS_CB, BST_CHECKED);

                    if (keySettings->dwLayout == 1)
                    {
                        CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LAYOUT, BST_CHECKED);
                    }
                    else
                    {
                        CheckDlgButton(hwndDlg, IDC_CTRL_LAYOUT, BST_CHECKED);
                    }
                }
            }
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_SWITCH_INPUT_LANG_CB:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_SWITCH_INPUT_LANG_CB) == BST_CHECKED)
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_CTRL_LANG), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT_ALT_LANG), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_CTRL_LANG), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT_ALT_LANG), FALSE);
                        }
                    }
                }
                break;

                case IDC_SWITCH_KBLAYOUTS_CB:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_SWITCH_KBLAYOUTS_CB) == BST_CHECKED)
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_CTRL_LAYOUT), TRUE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT_ALT_LAYOUT), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_CTRL_LAYOUT), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_LEFT_ALT_LAYOUT), FALSE);
                        }
                    }
                }
                break;

                case IDC_CTRL_LANG:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LAYOUT, BST_CHECKED);
                        CheckDlgButton(hwndDlg, IDC_CTRL_LAYOUT, BST_UNCHECKED);
                    }
                }
                break;

                case IDC_LEFT_ALT_LANG:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        CheckDlgButton(hwndDlg, IDC_CTRL_LAYOUT, BST_CHECKED);
                        CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LAYOUT, BST_UNCHECKED);
                    }
                }
                break;

                case IDC_CTRL_LAYOUT:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LANG, BST_CHECKED);
                        CheckDlgButton(hwndDlg, IDC_CTRL_LANG, BST_UNCHECKED);
                    }
                }
                break;

                case IDC_LEFT_ALT_LAYOUT:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        CheckDlgButton(hwndDlg, IDC_CTRL_LANG, BST_CHECKED);
                        CheckDlgButton(hwndDlg, IDC_LEFT_ALT_LANG, BST_UNCHECKED);
                    }
                }
                break;

                case IDOK:
                {
                    KEY_SETTINGS *keySettings;

                    keySettings = (KEY_SETTINGS*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

                    if (keySettings != NULL)
                    {
                        if (IsDlgButtonChecked(hwndDlg, IDC_SWITCH_INPUT_LANG_CB) == BST_CHECKED)
                        {
                            if (IsDlgButtonChecked(hwndDlg, IDC_CTRL_LANG) == BST_CHECKED)
                            {
                                keySettings->dwLanguage = 2;
                            }
                            else
                            {
                                keySettings->dwLanguage = 1;
                            }
                        }
                        else
                        {
                            keySettings->dwLanguage = 3;
                        }

                        if (IsDlgButtonChecked(hwndDlg, IDC_SWITCH_KBLAYOUTS_CB) == BST_CHECKED)
                        {
                            if (IsDlgButtonChecked(hwndDlg, IDC_CTRL_LAYOUT) == BST_CHECKED)
                            {
                                keySettings->dwLayout = 2;
                            }
                            else
                            {
                                keySettings->dwLayout = 1;
                            }
                        }
                        else
                        {
                            keySettings->dwLayout = 3;
                        }
                    }

                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
