/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/dialogs.c
 * PURPOSE:     Window procedures of the dialog windows plus launching functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LRESULT CALLBACK
MRDlgWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            CheckDlgButton(hwnd, IDD_MIRRORROTATERB1, BST_CHECKED);
            CheckDlgButton(hwnd, IDD_MIRRORROTATERB4, BST_CHECKED);
            return TRUE;
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (IsDlgButtonChecked(hwnd, IDD_MIRRORROTATERB1))
                        EndDialog(hwnd, 1);
                    else if (IsDlgButtonChecked(hwnd, IDD_MIRRORROTATERB2))
                        EndDialog(hwnd, 2);
                    else if (IsDlgButtonChecked(hwnd, IDD_MIRRORROTATERB4))
                        EndDialog(hwnd, 3);
                    else if (IsDlgButtonChecked(hwnd, IDD_MIRRORROTATERB5))
                        EndDialog(hwnd, 4);
                    else if (IsDlgButtonChecked(hwnd, IDD_MIRRORROTATERB6))
                        EndDialog(hwnd, 5);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
                case IDD_MIRRORROTATERB3:
                    EnableWindow(GetDlgItem(hwnd, IDD_MIRRORROTATERB4), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDD_MIRRORROTATERB5), TRUE);
                    EnableWindow(GetDlgItem(hwnd, IDD_MIRRORROTATERB6), TRUE);
                    break;
                case IDD_MIRRORROTATERB1:
                case IDD_MIRRORROTATERB2:
                    EnableWindow(GetDlgItem(hwnd, IDD_MIRRORROTATERB4), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDD_MIRRORROTATERB5), FALSE);
                    EnableWindow(GetDlgItem(hwnd, IDD_MIRRORROTATERB6), FALSE);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

int
mirrorRotateDlg()
{
    return DialogBox(hProgInstance, MAKEINTRESOURCE(IDD_MIRRORROTATE), hMainWnd, (DLGPROC) MRDlgWinProc);
}

LRESULT CALLBACK
ATTDlgWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR strrc[100];
            TCHAR res[100];

            CheckDlgButton(hwnd, IDD_ATTRIBUTESRB3, BST_CHECKED);
            CheckDlgButton(hwnd, IDD_ATTRIBUTESRB5, BST_CHECKED);
            SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT1, imgXRes, FALSE);
            SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT2, imgYRes, FALSE);

            if (isAFile)
            {
                TCHAR date[100];
                TCHAR size[100];
                TCHAR temp[100];
                GetDateFormat(LOCALE_USER_DEFAULT, 0, &fileTime, NULL, date, SIZEOF(date));
                GetTimeFormat(LOCALE_USER_DEFAULT, 0, &fileTime, NULL, temp, SIZEOF(temp));
                _tcscat(date, _T(" "));
                _tcscat(date, temp);
                LoadString(hProgInstance, IDS_FILESIZE, strrc, SIZEOF(strrc));
                _stprintf(size, strrc, fileSize);
                SetDlgItemText(hwnd, IDD_ATTRIBUTESTEXT6, date);
                SetDlgItemText(hwnd, IDD_ATTRIBUTESTEXT7, size);
            }
            LoadString(hProgInstance, IDS_PRINTRES, strrc, SIZEOF(strrc));
            _stprintf(res, strrc, fileHPPM, fileVPPM);
            SetDlgItemText(hwnd, IDD_ATTRIBUTESTEXT8, res);
            return TRUE;
        }
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd,
                              GetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT1, NULL,
                                            FALSE) | (GetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT2, NULL,
                                                                    FALSE) << 16));
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
                case IDD_ATTRIBUTESSTANDARD:
                    CheckDlgButton(hwnd, IDD_ATTRIBUTESRB3, BST_CHECKED);
                    CheckDlgButton(hwnd, IDD_ATTRIBUTESRB5, BST_CHECKED);
                    SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT1, imgXRes, FALSE);
                    SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT2, imgYRes, FALSE);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

int
attributesDlg()
{
    return DialogBox(hProgInstance, MAKEINTRESOURCE(IDD_ATTRIBUTES), hMainWnd, (DLGPROC) ATTDlgWinProc);
}

LRESULT CALLBACK
CHSIZEDlgWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            SetDlgItemInt(hwnd, IDD_CHANGESIZEEDIT1, 100, FALSE);
            SetDlgItemInt(hwnd, IDD_CHANGESIZEEDIT2, 100, FALSE);
            return TRUE;
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwnd,
                              GetDlgItemInt(hwnd, IDD_CHANGESIZEEDIT1, NULL,
                                            FALSE) | (GetDlgItemInt(hwnd, IDD_CHANGESIZEEDIT2, NULL,
                                                                    FALSE) << 16));
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

int
changeSizeDlg()
{
    return DialogBox(hProgInstance, MAKEINTRESOURCE(IDD_CHANGESIZE), hMainWnd, (DLGPROC) CHSIZEDlgWinProc);
}
