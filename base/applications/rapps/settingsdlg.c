/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/settingsdlg.c
 * PURPOSE:         Settings Dialog
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

SETTINGS_INFO NewSettingsInfo;

#define IS_CHECKED(a, b) \
    a = (SendDlgItemMessage(hDlg, b, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE

BOOL
ChooseFolder(HWND hwnd)
{
    BROWSEINFO fi;
    LPCITEMIDLIST lpItemList;
    WCHAR szPath[MAX_PATH], szBuf[MAX_STR_LEN];

    LoadStringW(hInst, IDS_CHOOSE_FOLDER_TEXT, szBuf, sizeof(szBuf) / sizeof(TCHAR));

    ZeroMemory(&fi, sizeof(BROWSEINFO));
    fi.hwndOwner = hwnd;
    fi.lpszTitle = szBuf;
    fi.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | BIF_BROWSEFORCOMPUTER | BIF_NEWDIALOGSTYLE;
    fi.lpfn = NULL;
    fi.lParam = -1;
    fi.iImage = 0;

    if (!(lpItemList = SHBrowseForFolder(&fi))) return FALSE;
    SHGetPathFromIDList(lpItemList, szPath);

    if (wcslen(szPath) == 0) return FALSE;
    SetDlgItemTextW(hwnd, IDC_DOWNLOAD_DIR_EDIT, szPath);

    return TRUE;
}

static VOID
InitSettingsControls(HWND hDlg, SETTINGS_INFO Info)
{
    SendDlgItemMessage(hDlg, IDC_SAVE_WINDOW_POS, BM_SETCHECK, Info.bSaveWndPos, 0);
    SendDlgItemMessage(hDlg, IDC_UPDATE_AVLIST, BM_SETCHECK, Info.bUpdateAtStart, 0);
    SendDlgItemMessage(hDlg, IDC_LOG_ENABLED, BM_SETCHECK, Info.bLogEnabled, 0);
    SendDlgItemMessage(hDlg, IDC_DEL_AFTER_INSTALL, BM_SETCHECK, Info.bDelInstaller, 0);

    SetWindowTextW(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT),
                   Info.szDownloadDir);
}

static
INT_PTR CALLBACK
SettingsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
        {
            NewSettingsInfo = SettingsInfo;
            InitSettingsControls(hDlg, SettingsInfo);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHOOSE:
                    ChooseFolder(hDlg);
                    break;

                case IDC_SAVE_WINDOW_POS:
                    IS_CHECKED(NewSettingsInfo.bSaveWndPos, IDC_SAVE_WINDOW_POS);
                    break;

                case IDC_UPDATE_AVLIST:
                    IS_CHECKED(NewSettingsInfo.bUpdateAtStart, IDC_UPDATE_AVLIST);
                    break;

                case IDC_LOG_ENABLED:
                    IS_CHECKED(NewSettingsInfo.bLogEnabled, IDC_LOG_ENABLED);
                    break;

                case IDC_DEL_AFTER_INSTALL:
                    IS_CHECKED(NewSettingsInfo.bDelInstaller, IDC_DEL_AFTER_INSTALL);
                    break;

                case IDC_DEFAULT_SETTINGS:
                    FillDafaultSettings(&NewSettingsInfo);
                    InitSettingsControls(hDlg, NewSettingsInfo);
                    break;

                case IDOK:
                {
                    WCHAR szDir[MAX_PATH];
                    DWORD dwAttr;

                    GetWindowTextW(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT),
                                   szDir, MAX_PATH);

                    dwAttr = GetFileAttributesW(szDir);
                    if (dwAttr != INVALID_FILE_ATTRIBUTES &&
                        (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        wcscpy(NewSettingsInfo.szDownloadDir, szDir);
                    }
                    else
                    {
                        WCHAR szMsgText[MAX_STR_LEN];

                        LoadStringW(hInst,
                                    IDS_CHOOSE_FOLDER_ERROR,
                                    szMsgText, sizeof(szMsgText) / sizeof(WCHAR));

                        MessageBoxW(hDlg, szMsgText, NULL, MB_OK | MB_ICONERROR);
                        SetFocus(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT));
                        break;
                    }

                    SettingsInfo = NewSettingsInfo;
                    SaveSettings(GetParent(hDlg));
                    EndDialog(hDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    break;
            }
        }
        break;
    }

    return FALSE;
}

VOID
CreateSettingsDlg(HWND hwnd)
{
    DialogBoxW(hInst,
               MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG),
               hwnd,
               SettingsDlgProc);
}
