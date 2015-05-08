/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/dialogs.cpp
 * PURPOSE:     Window procedures of the dialog windows plus launching functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#include <winnls.h>

/* FUNCTIONS ********************************************************/

INT_PTR CALLBACK
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
    return DialogBox(hProgInstance, MAKEINTRESOURCE(IDD_MIRRORROTATE), hMainWnd, MRDlgWinProc);
}

INT_PTR CALLBACK
ATTDlgWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            TCHAR strrc[100];
            TCHAR res[100];

            widthSetInDlg = imgXRes;
            heightSetInDlg = imgYRes;

            CheckDlgButton(hwnd, IDD_ATTRIBUTESRB3, BST_CHECKED);
            CheckDlgButton(hwnd, IDD_ATTRIBUTESRB5, BST_CHECKED);
            SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT1, widthSetInDlg, FALSE);
            SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT2, heightSetInDlg, FALSE);

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
                    EndDialog(hwnd, 1);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
                case IDD_ATTRIBUTESSTANDARD:
                    widthSetInDlg = imgXRes;
                    heightSetInDlg = imgYRes;
                    CheckDlgButton(hwnd, IDD_ATTRIBUTESRB3, BST_CHECKED);
                    CheckDlgButton(hwnd, IDD_ATTRIBUTESRB5, BST_CHECKED);
                    SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT1, widthSetInDlg, FALSE);
                    SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT2, heightSetInDlg, FALSE);
                    break;
                case IDD_ATTRIBUTESRB1:
                {
                    TCHAR number[100];
                    _stprintf(number, _T("%.3lf"), widthSetInDlg / (0.0254 * fileHPPM));
                    SetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT1, number);
                    _stprintf(number, _T("%.3lf"), heightSetInDlg / (0.0254 * fileVPPM));
                    SetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT2, number);
                    break;
                }
                case IDD_ATTRIBUTESRB2:
                {
                    TCHAR number[100];
                    _stprintf(number, _T("%.3lf"), widthSetInDlg * 100.0 / fileHPPM);
                    SetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT1, number);
                    _stprintf(number, _T("%.3lf"), heightSetInDlg * 100.0 / fileVPPM);
                    SetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT2, number);
                    break;
                }
                case IDD_ATTRIBUTESRB3:
                    SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT1, widthSetInDlg, FALSE);
                    SetDlgItemInt(hwnd, IDD_ATTRIBUTESEDIT2, heightSetInDlg, FALSE);
                    break;
                case IDD_ATTRIBUTESEDIT1:
                    if (Edit_GetModify((HWND)lParam))
                    {
                        TCHAR tempS[100];
                        if (IsDlgButtonChecked(hwnd, IDD_ATTRIBUTESRB1))
                        {
                            GetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT1, tempS, SIZEOF(tempS));
                            widthSetInDlg = max(1, (int) (_tcstod(tempS, NULL) * fileHPPM * 0.0254));
                        }
                        else if (IsDlgButtonChecked(hwnd, IDD_ATTRIBUTESRB2))
                        {
                            GetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT1, tempS, SIZEOF(tempS));
                            widthSetInDlg = max(1, (int) (_tcstod(tempS, NULL) * fileHPPM / 100));
                        }
                        else if (IsDlgButtonChecked(hwnd, IDD_ATTRIBUTESRB3))
                        {
                            GetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT1, tempS, SIZEOF(tempS));
                            widthSetInDlg = max(1, _tstoi(tempS));
                        }
                        Edit_SetModify((HWND)lParam, FALSE);
                    }
                    break;
                case IDD_ATTRIBUTESEDIT2:
                    if (Edit_GetModify((HWND)lParam))
                    {
                        TCHAR tempS[100];
                        if (IsDlgButtonChecked(hwnd, IDD_ATTRIBUTESRB1))
                        {
                            GetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT2, tempS, SIZEOF(tempS));
                            heightSetInDlg = max(1, (int) (_tcstod(tempS, NULL) * fileVPPM * 0.0254));
                        }
                        else if (IsDlgButtonChecked(hwnd, IDD_ATTRIBUTESRB2))
                        {
                            GetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT2, tempS, SIZEOF(tempS));
                            heightSetInDlg = max(1, (int) (_tcstod(tempS, NULL) * fileVPPM / 100));
                        }
                        else if (IsDlgButtonChecked(hwnd, IDD_ATTRIBUTESRB3))
                        {
                            GetDlgItemText(hwnd, IDD_ATTRIBUTESEDIT2, tempS, SIZEOF(tempS));
                            heightSetInDlg = max(1, _tstoi(tempS));
                        }
                        Edit_SetModify((HWND)lParam, FALSE);
                    }
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
    return DialogBox(hProgInstance, MAKEINTRESOURCE(IDD_ATTRIBUTES), hMainWnd, ATTDlgWinProc);
}

INT_PTR CALLBACK
CHSIZEDlgWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            SetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITHSTRETCH, 100, FALSE);
            SetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITVSTRETCH, 100, FALSE);
            SetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITHSKEW, 0, FALSE);
            SetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITVSKEW, 0, FALSE);
            return TRUE;
        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    TCHAR strrcIntNumbers[100];
                    TCHAR strrcPercentage[100];
                    TCHAR strrcAngle[100];
                    BOOL tr1, tr2, tr3, tr4;

                    LoadString(hProgInstance, IDS_INTNUMBERS, strrcIntNumbers, SIZEOF(strrcIntNumbers));
                    LoadString(hProgInstance, IDS_PERCENTAGE, strrcPercentage, SIZEOF(strrcPercentage));
                    LoadString(hProgInstance, IDS_ANGLE, strrcAngle, SIZEOF(strrcAngle));

                    stretchSkew.percentage.x = GetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITHSTRETCH, &tr1, FALSE);
                    stretchSkew.percentage.y = GetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITVSTRETCH, &tr2, FALSE);
                    stretchSkew.angle.x = GetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITHSKEW, &tr3, TRUE);
                    stretchSkew.angle.y = GetDlgItemInt(hwnd, IDD_STRETCHSKEWEDITVSKEW, &tr4, TRUE);

                    if (!(tr1 && tr2 && tr3 && tr4))
                        MessageBox(hwnd, strrcIntNumbers, NULL, MB_ICONEXCLAMATION);
                    else if (stretchSkew.percentage.x < 1 || stretchSkew.percentage.x > 500
                        || stretchSkew.percentage.y < 1 || stretchSkew.percentage.y > 500)
                        MessageBox(hwnd, strrcPercentage, NULL, MB_ICONEXCLAMATION);
                    else if (stretchSkew.angle.x < -89 || stretchSkew.angle.x > 89
                        || stretchSkew.angle.y < -89 || stretchSkew.angle.y > 89)
                        MessageBox(hwnd, strrcAngle, NULL, MB_ICONEXCLAMATION);
                    else
                        EndDialog(hwnd, 1);

                    break;
                }
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
    return DialogBox(hProgInstance, MAKEINTRESOURCE(IDD_STRETCHSKEW), hMainWnd, CHSIZEDlgWinProc);
}
