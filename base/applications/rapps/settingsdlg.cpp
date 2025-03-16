/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Settings Dialog
 * COPYRIGHT:   Copyright 2009 Dmitry Chapyshev (dmitry@reactos.org)
 *              Copyright 2017 Alexander Shaposhnikov (sanchaez@reactos.org)
 */
#include "rapps.h"

SETTINGS_INFO *g_pNewSettingsInfo;

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

static BOOL
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

static BOOL
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
static inline BOOL
IsCheckedDlgItem(HWND hDlg, INT nIDDlgItem)
{
    return SendDlgItemMessageW(hDlg, nIDDlgItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

static inline void
AdjustListViewHeader(HWND hWndList)
{
    ListView_SetColumnWidth(hWndList, 0, LVSCW_AUTOSIZE_USEHEADER);
}

static void
HandleGeneralListItems(HWND hWndList, PSETTINGS_INFO Load, PSETTINGS_INFO Save)
{
    PSETTINGS_INFO Info = Load ? Load : Save;
    const struct {
        WORD Id;
        BOOL *Setting;
    } Map[] = {
        { IDS_CFG_SAVE_WINDOW_POS, &Info->bSaveWndPos },
        { IDS_CFG_UPDATE_AVLIST, &Info->bUpdateAtStart },
        { IDS_CFG_LOG_ENABLED, &Info->bLogEnabled },
        { IDS_CFG_SMALL_ICONS, &Info->bSmallIcons },
    };

    if (Load)
    {
        UINT ExStyle = LVS_EX_CHECKBOXES | LVS_EX_LABELTIP;
        ListView_SetExtendedListViewStyleEx(hWndList, ExStyle, ExStyle);
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_SUBITEM;
        lvc.iSubItem = 0;
        lvc.pszText = const_cast<PWSTR>(L"");
        ListView_InsertColumn(hWndList, 0, &lvc);

        CStringW Name;
        for (SIZE_T i = 0; i < _countof(Map); ++i)
        {
            LVITEMW Item;
            Item.mask = LVIF_TEXT | LVIF_PARAM;
            Item.iItem = 0x7fff;
            Item.iSubItem = 0;
            Item.lParam = Map[i].Id;
            Name.LoadStringW(Map[i].Id);
            Item.pszText = const_cast<PWSTR>(Name.GetString());
            Item.iItem = ListView_InsertItem(hWndList, &Item);
            ListView_SetCheckState(hWndList, Item.iItem, *Map[i].Setting);
        }
        ListView_SetItemState(hWndList, 0, -1, LVIS_FOCUSED | LVIS_SELECTED);
        AdjustListViewHeader(hWndList);
    }
    else
    {
        for (SIZE_T i = 0; i < _countof(Map); ++i)
        {
            LVFINDINFOW FindInfo = { LVFI_PARAM, NULL, Map[i].Id };
            int Idx = ListView_FindItem(hWndList, -1, &FindInfo);
            if (Idx >= 0)
                *Map[i].Setting = ListView_GetCheckState(hWndList, Idx);
        }
    }
}

static VOID
InitSettingsControls(HWND hDlg, PSETTINGS_INFO Info)
{
    HandleGeneralListItems(GetDlgItem(hDlg, IDC_GENERALLIST), Info, NULL);
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

static INT_PTR CALLBACK
SettingsDlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    SETTINGS_INFO &NewSettingsInfo = *g_pNewSettingsInfo;

    switch (Msg)
    {
        case WM_INITDIALOG:
            InitSettingsControls(hDlg, &SettingsInfo);
            return TRUE;

        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
        case WM_SYSCOLORCHANGE:
            SendMessage(GetDlgItem(hDlg, IDC_GENERALLIST), Msg, wParam, lParam);
            AdjustListViewHeader(GetDlgItem(hDlg, IDC_GENERALLIST));
            break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CHOOSE:
                    ChooseFolder(hDlg);
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
                    HandleGeneralListItems(GetDlgItem(hDlg, IDC_GENERALLIST), NULL, &NewSettingsInfo);

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

                    CStringW::CopyChars(
                        NewSettingsInfo.szDownloadDir, _countof(NewSettingsInfo.szDownloadDir), szDir,
                        szDir.GetLength() + 1);
                    dwAttr = GetFileAttributesW(szDir);
                    if (dwAttr == INVALID_FILE_ATTRIBUTES || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        CStringW szMsgText;
                        szMsgText.LoadStringW(IDS_CHOOSE_FOLDER_ERROR);

                        if (MessageBoxW(hDlg, szMsgText, NULL, MB_YESNO) == IDYES)
                        {
                            if (!CreateDirectoryW(szDir, NULL))
                            {
                                ErrorBox(hDlg);
                                break;
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

                    if (SettingsInfo.bSmallIcons != NewSettingsInfo.bSmallIcons)
                    {
                        SendMessageW(hMainWnd, WM_SETTINGCHANGE, SPI_SETICONMETRICS, 0); // Note: WM_SETTINGCHANGE cannot be posted
                        PostMessageW(hMainWnd, WM_COMMAND, ID_REFRESH, 0);
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
            break;
        }

        case WM_NOTIFY:
        {
            NMITEMACTIVATE &nmia = *(NMITEMACTIVATE*)lParam;
            if (wParam == IDC_GENERALLIST && nmia.hdr.code == NM_CLICK)
            {
                LVHITTESTINFO lvhti;
                lvhti.pt = nmia.ptAction;
                if (nmia.iItem != -1 && ListView_HitTest(nmia.hdr.hwndFrom, &lvhti) != -1)
                {
                    if (lvhti.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL))
                        ListView_SetCheckState(nmia.hdr.hwndFrom, nmia.iItem,
                                               !ListView_GetCheckState(nmia.hdr.hwndFrom, nmia.iItem));
                }
            }
            break;
        }
    }

    return FALSE;
}
} // namespace

VOID
CreateSettingsDlg(HWND hwnd)
{
    SETTINGS_INFO NewSettingsInfo = SettingsInfo;
    g_pNewSettingsInfo = &NewSettingsInfo;

    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG), hwnd, SettingsDlgProc);
}
