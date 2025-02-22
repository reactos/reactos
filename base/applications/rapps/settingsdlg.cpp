/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Settings Dialog
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 */
#include "rapps.h"

SETTINGS_INFO NewSettingsInfo;

static int CALLBACK
BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
        case BFFM_INITIALIZED:
            SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
            break;
        case BFFM_VALIDATEFAILED:
            return TRUE;
    }
    return 0;
}

BOOL
ChooseFolder(HWND hwnd)
{
    BOOL bRet = FALSE;
    BROWSEINFOW bi;
    CStringW szChooseFolderText;

    szChooseFolderText.LoadStringW(IDS_CHOOSE_FOLDER_TEXT);

    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hwnd;
    bi.pidlRoot = NULL;
    bi.lpszTitle = szChooseFolderText;
    bi.ulFlags =
        BIF_USENEWUI | BIF_DONTGOBELOWDOMAIN | BIF_RETURNONLYFSDIRS | /* BIF_BROWSEFILEJUNCTIONS | */ BIF_VALIDATE;

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        WCHAR szDir[MAX_PATH];
        if (GetWindowTextW(GetDlgItem(hwnd, IDC_DOWNLOAD_DIR_EDIT), szDir, _countof(szDir)))
        {
            bi.lpfn = BrowseFolderCallback;
            bi.lParam = (LPARAM)szDir;
        }

        LPITEMIDLIST lpItemList = SHBrowseForFolderW(&bi);
        if (lpItemList && SHGetPathFromIDListW(lpItemList, szDir))
        {
            if (*szDir)
            {
                SetDlgItemTextW(hwnd, IDC_DOWNLOAD_DIR_EDIT, szDir);
                bRet = TRUE;
            }
        }

        CoTaskMemFree(lpItemList);
        CoUninitialize();
    }

    return bRet;
}

BOOL
IsUrlValid(const WCHAR *Url)
{
    URL_COMPONENTSW UrlComponmentInfo = {0};
    UrlComponmentInfo.dwStructSize = sizeof(UrlComponmentInfo);
    UrlComponmentInfo.dwSchemeLength = 1;

    BOOL bSuccess = InternetCrackUrlW(Url, wcslen(Url), 0, &UrlComponmentInfo);
    if (!bSuccess)
    {
        return FALSE;
    }

    switch (UrlComponmentInfo.nScheme)
    {
        case INTERNET_SCHEME_HTTP:
        case INTERNET_SCHEME_HTTPS:
        case INTERNET_SCHEME_FTP:
        case INTERNET_SCHEME_FILE:
            // supported
            return TRUE;

        default:
            return FALSE;
    }
}

namespace
{
inline BOOL
IsCheckedDlgItem(HWND hDlg, INT nIDDlgItem)
{
    return (SendDlgItemMessageW(hDlg, nIDDlgItem, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;
}

VOID
InitSettingsControls(HWND hDlg, PSETTINGS_INFO Info)
{
    SendDlgItemMessageW(hDlg, IDC_SAVE_WINDOW_POS, BM_SETCHECK, Info->bSaveWndPos, 0);
    SendDlgItemMessageW(hDlg, IDC_UPDATE_AVLIST, BM_SETCHECK, Info->bUpdateAtStart, 0);
    SendDlgItemMessageW(hDlg, IDC_LOG_ENABLED, BM_SETCHECK, Info->bLogEnabled, 0);
    SendDlgItemMessageW(hDlg, IDC_DEL_AFTER_INSTALL, BM_SETCHECK, Info->bDelInstaller, 0);

    HWND hCtl = GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT);
    SetWindowTextW(hCtl, Info->szDownloadDir);
    SendMessageW(hCtl, EM_LIMITTEXT, MAX_PATH - 1, 0);

    CheckRadioButton(hDlg, IDC_PROXY_DEFAULT, IDC_USE_PROXY, IDC_PROXY_DEFAULT + Info->Proxy);

    if (Info->Proxy == 2)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PROXY_SERVER), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hDlg, IDC_PROXY_SERVER), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), FALSE);
    }

    CheckRadioButton(hDlg, IDC_SOURCE_DEFAULT, IDC_USE_SOURCE, Info->bUseSource ? IDC_USE_SOURCE : IDC_SOURCE_DEFAULT);

    EnableWindow(GetDlgItem(hDlg, IDC_SOURCE_URL), Info->bUseSource);

    SetWindowTextW(GetDlgItem(hDlg, IDC_SOURCE_URL), Info->szSourceURL);
    SetWindowTextW(GetDlgItem(hDlg, IDC_PROXY_SERVER), Info->szProxyServer);
    SetWindowTextW(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), Info->szNoProxyFor);
}

INT_PTR CALLBACK
SettingsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_INITDIALOG:
            NewSettingsInfo = SettingsInfo;
            InitSettingsControls(hDlg, &SettingsInfo);
            return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHOOSE:
                    ChooseFolder(hDlg);
                    break;

                case IDC_SAVE_WINDOW_POS:
                    NewSettingsInfo.bSaveWndPos = IsCheckedDlgItem(hDlg, IDC_SAVE_WINDOW_POS);
                    break;

                case IDC_UPDATE_AVLIST:
                    NewSettingsInfo.bUpdateAtStart = IsCheckedDlgItem(hDlg, IDC_UPDATE_AVLIST);
                    break;

                case IDC_LOG_ENABLED:
                    NewSettingsInfo.bLogEnabled = IsCheckedDlgItem(hDlg, IDC_LOG_ENABLED);
                    break;

                case IDC_DEL_AFTER_INSTALL:
                    NewSettingsInfo.bDelInstaller = IsCheckedDlgItem(hDlg, IDC_DEL_AFTER_INSTALL);
                    break;

                case IDC_SOURCE_DEFAULT:
                    NewSettingsInfo.bUseSource = FALSE;
                    EnableWindow(GetDlgItem(hDlg, IDC_SOURCE_URL), NewSettingsInfo.bUseSource);
                    break;

                case IDC_USE_SOURCE:
                    NewSettingsInfo.bUseSource = TRUE;
                    EnableWindow(GetDlgItem(hDlg, IDC_SOURCE_URL), NewSettingsInfo.bUseSource);
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
                    CStringW szDir;
                    CStringW szSource;
                    CStringW szProxy;
                    CStringW szNoProxy;
                    DWORD dwAttr;

                    GetWindowTextW(GetDlgItem(hDlg, IDC_DOWNLOAD_DIR_EDIT), szDir.GetBuffer(MAX_PATH), MAX_PATH);
                    szDir.ReleaseBuffer();

                    GetWindowTextW(
                        GetDlgItem(hDlg, IDC_SOURCE_URL), szSource.GetBuffer(INTERNET_MAX_URL_LENGTH),
                        INTERNET_MAX_URL_LENGTH);
                    szSource.ReleaseBuffer();

                    GetWindowTextW(GetDlgItem(hDlg, IDC_PROXY_SERVER), szProxy.GetBuffer(MAX_PATH), MAX_PATH);
                    szProxy.ReleaseBuffer();
                    CStringW::CopyChars(
                        NewSettingsInfo.szProxyServer, _countof(NewSettingsInfo.szProxyServer), szProxy,
                        szProxy.GetLength() + 1);

                    GetWindowTextW(GetDlgItem(hDlg, IDC_NO_PROXY_FOR), szNoProxy.GetBuffer(MAX_PATH), MAX_PATH);
                    szNoProxy.ReleaseBuffer();
                    CStringW::CopyChars(
                        NewSettingsInfo.szNoProxyFor, _countof(NewSettingsInfo.szNoProxyFor), szNoProxy,
                        szNoProxy.GetLength() + 1);

                    dwAttr = GetFileAttributesW(szDir);
                    if (dwAttr != INVALID_FILE_ATTRIBUTES && (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        CStringW::CopyChars(
                            NewSettingsInfo.szDownloadDir, _countof(NewSettingsInfo.szDownloadDir), szDir,
                            szDir.GetLength() + 1);
                    }
                    else
                    {
                        CStringW szMsgText;
                        szMsgText.LoadStringW(IDS_CHOOSE_FOLDER_ERROR);

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

                    if (NewSettingsInfo.bUseSource && !IsUrlValid(szSource))
                    {
                        CStringW szMsgText;
                        szMsgText.LoadStringW(IDS_URL_INVALID);

                        MessageBoxW(hDlg, szMsgText, NULL, MB_OK);
                        SetFocus(GetDlgItem(hDlg, IDC_SOURCE_URL));
                        break;
                    }
                    else
                    {
                        CStringW::CopyChars(
                            NewSettingsInfo.szSourceURL, _countof(NewSettingsInfo.szSourceURL), szSource,
                            szSource.GetLength() + 1);
                    }

                    SettingsInfo = NewSettingsInfo;
                    SaveSettings(GetParent(hDlg), &SettingsInfo);
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
} // namespace

VOID
CreateSettingsDlg(HWND hwnd)
{
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG), hwnd, SettingsDlgProc);
}
