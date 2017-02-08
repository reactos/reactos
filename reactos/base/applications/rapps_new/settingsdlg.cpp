/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps_new/settingsdlg.cpp
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
    BOOL bRet = FALSE;
    BROWSEINFO bi;
    WCHAR szPath[MAX_PATH], szBuf[MAX_STR_LEN];

    LoadStringW(hInst, IDS_CHOOSE_FOLDER_TEXT, szBuf, sizeof(szBuf) / sizeof(TCHAR));

    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hwnd;
    bi.pidlRoot  = NULL;
    bi.lpszTitle = szBuf;
    bi.ulFlags   = BIF_USENEWUI | BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | /* BIF_BROWSEFILEJUNCTIONS | */ BIF_VALIDATE;

    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        LPITEMIDLIST lpItemList = SHBrowseForFolder(&bi);
        if (lpItemList && SHGetPathFromIDList(lpItemList, szPath))
        {
            if (szPath[0] != 0)
            {
                SetDlgItemTextW(hwnd, IDC_DOWNLOAD_DIR_EDIT, szPath);
                bRet = TRUE;
            }
        }

        CoTaskMemFree(lpItemList);
        CoUninitialize();
    }

    return bRet;
}

static VOID
InitSettingsControls(HWND hDlg, PSETTINGS_INFO Info)
{
    SendDlgItemMessage(hDlg, IDC_SAVE_WINDOW_POS, BM_SETCHECK, Info->bSaveWndPos, 0);
    SendDlgItemMessage(hDlg, IDC_UPDATE_AVLIST, BM_SETCHECK, Info->bUpdateAtStart, 0);
    SendDlgItemMessage(hDlg, IDC_LOG_ENABLED, BM_SETCHECK, Info->bLogEnabled, 0);
    SendDlgItemMessage(hDlg, IDC_DEL_AFTER_INSTALL, BM_SETCHECK, Info->bDelInstaller, 0);

    SetWindowTextW(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT),
                   Info->szDownloadDir);

    CheckRadioButton(hDlg, IDC_PROXY_DEFAULT, IDC_USE_PROXY, IDC_PROXY_DEFAULT+Info->Proxy);

    if(IDC_PROXY_DEFAULT + Info->Proxy == IDC_USE_PROXY)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PROXY_SERVER), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), TRUE);
    }

    SetWindowTextW(GetDlgItem(hDlg, IDC_PROXY_SERVER), Info->szProxyServer);
    SetWindowTextW(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), Info->szNoProxyFor);
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
            InitSettingsControls(hDlg, &SettingsInfo);
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

                case IDC_PROXY_DEFAULT:
                    NewSettingsInfo.Proxy = 0;
                    EnableWindow(GetDlgItem(hDlg, IDC_PROXY_SERVER), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), FALSE);
                    break;

                case IDC_NO_PROXY:
                    NewSettingsInfo.Proxy = 1;
                    EnableWindow(GetDlgItem(hDlg, IDC_PROXY_SERVER), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), FALSE);
                    break;

                case IDC_USE_PROXY:
                    NewSettingsInfo.Proxy = 2;
                    EnableWindow(GetDlgItem(hDlg, IDC_PROXY_SERVER), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), TRUE);
                    break;

                case IDC_DEFAULT_SETTINGS:
                    FillDefaultSettings(&NewSettingsInfo);
                    InitSettingsControls(hDlg, &NewSettingsInfo);
                    break;

                case IDOK:
                {
                    WCHAR szDir[MAX_PATH];
                    WCHAR szProxy[MAX_PATH];
                    WCHAR szNoProxy[MAX_PATH];
                    DWORD dwAttr;

                    GetWindowTextW(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT),
                                   szDir, MAX_PATH);

                    GetWindowTextW(GetDlgItem(hDlg, IDC_PROXY_SERVER),
                                   szProxy, MAX_PATH);
                    StringCbCopyW(NewSettingsInfo.szProxyServer, sizeof(NewSettingsInfo.szProxyServer), szProxy);

                    GetWindowTextW(GetDlgItem(hDlg, IDC_NO_PROXY_FOR),
                                   szNoProxy, MAX_PATH);
                    StringCbCopyW(NewSettingsInfo.szNoProxyFor, sizeof(NewSettingsInfo.szNoProxyFor), szNoProxy);

                    dwAttr = GetFileAttributesW(szDir);
                    if (dwAttr != INVALID_FILE_ATTRIBUTES &&
                        (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        StringCbCopyW(NewSettingsInfo.szDownloadDir,
                                      sizeof(NewSettingsInfo.szDownloadDir),
                                      szDir);
                    }
                    else
                    {
                        WCHAR szMsgText[MAX_STR_LEN];

                        LoadStringW(hInst,
                                    IDS_CHOOSE_FOLDER_ERROR,
                                    szMsgText, sizeof(szMsgText) / sizeof(WCHAR));

                        if (MessageBoxW(hDlg, szMsgText, NULL, MB_YESNO) == IDYES)
                        {
                            if (CreateDirectoryW(szDir, NULL))
                            {
                                EndDialog(hDlg, LOWORD(wParam));
                            }
                        }
                        else
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT));
                            break;
                        }
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
